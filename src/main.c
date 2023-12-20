#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <ctype.h>
#include <unistd.h>
#include <dirent.h>
#include "include/cmark.h"

#define HTTP_OK "200 OK"
#define HTTP_NOT_FOUND "404 NOT FOUND"
#define HTTP_METHOD_NOT_IMPLEMENTED "501 Method Not Implemented"

char *url_docode(const char *encoded);
void exit_with_error(const char *error);
int get_line(int sock, char *buf, int size);

int startup(uint16_t *port);
void handle_request(void *arg);

//默认请求头
void headers(int client, const char *code);
//返回页面渲染
void unimplemented(int client);
void not_found(int client);
void render_dir(char *originalBasePath, char *currentPath, char *html, int depth);
void render_html(int client, const char *filename);
void render_markdown(int client, const char *filename);

/**
 * URL解码
 * @param encoded 编码过的URL字符串
 * @return 解码后的字符串
 */
char *url_docode(const char *encoded)
{
    char *decoded = malloc(strlen(encoded) + 1);
    int i, j = 0;
    for (i = 0; i < strlen(encoded); i++)
    {
        if (encoded[i] == '%')
        {
            // 判断是否是URL编码字符
            if (i + 2 < strlen(encoded))
            {
                char hex[3];
                hex[0] = encoded[i + 1];
                hex[1] = encoded[i + 2];
                hex[2] = '\0';
                // 将十六进制字符串转换为整数
                int value = strtol(hex, NULL, 16);
                decoded[j++] = (char)value;
                i += 2;
            }
        }
        else if (encoded[i] == '+')
        {
            // 将加号替换为空格
            decoded[j++] = ' ';
        }
        else
        {
            // 复制其他字符
            decoded[j++] = encoded[i];
        }
    }
    decoded[j] = '\0';
    return decoded;
}

/**
 * 发送HTTP响应头
 * @param client 客户端socket描述符
 * @param code HTTP状态码和描述
 */
void headers(int client, const char *code)
{
    char buf[1024];
    sprintf(buf, "HTTP/1.0 %s\r\n", code);
    write(client, buf, strlen(buf));
    sprintf(buf, "Server: webMD\r\n");
    write(client, buf, strlen(buf));
    sprintf(buf, "Content-Type: text/html; charset=UTF-8\r\n");
    write(client, buf, strlen(buf));
    sprintf(buf, "Connection: close\r\n");
    write(client, buf, strlen(buf));
    strcpy(buf, "\r\n");
    write(client, buf, strlen(buf));
}

/**
 * 发送404未找到响应
 * @param client 客户端socket描述符
 */
void not_found(int client)
{
    char buf[1024];
    headers(client, HTTP_NOT_FOUND);

    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n"
                 "<BODY><P>"
                 "请求的资源不存在于服务器\r\n"
                 "</BODY></HTML>\r\n");
    write(client, buf, strlen(buf));
}

/**
 * 发送501未实现方法响应
 * @param client 客户端socket描述符
 */
void unimplemented(int client)
{
    char buf[1024];
    headers(client, HTTP_METHOD_NOT_IMPLEMENTED);

    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented</TITLE></HEAD>"
                 "<BODY><P>HTTP request method not supported.</P></BODY></HTML>");
    write(client, buf, strlen(buf));
}

/**
 * 渲染HTML内容并发送给客户端
 * @param client 客户端socket描述符
 * @param filename 要渲染的HTML文件名
 */
void render_html(int client, const char *filename)
{
    FILE *resource = NULL;
    int numchars = 1;
    char buf[1024];

    buf[0] = 'A';
    buf[1] = '\0';
    while ((numchars > 0) && strcmp("\n", buf)) //抛去剩余的headers
        numchars = get_line(client, buf, sizeof(buf));

    resource = fopen(filename, "r");
    if (resource == NULL)
        not_found(client);
    else
    {
        headers(client, HTTP_OK);
        fgets(buf, sizeof(buf), resource);
        while (!feof(resource))
        {
            write(client, buf, strlen(buf));
            fgets(buf, sizeof(buf), resource);
        }
    }
    fclose(resource);
}

/**
 * 渲染Markdown文件并发送给客户端
 * @param client 客户端socket描述符
 * @param filename 要渲染的Markdown文件名
 */
void render_markdown(int client, const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror("Error opening file");
        return;
    }

    // 读取文件内容到字符串
    fseek(fp, 0, SEEK_END);
    size_t length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *markdown = malloc(length + 1);
    if (markdown == NULL)
    {
        perror("Memory allocation failed");
        fclose(fp);
        return;
    }
    fread(markdown, 1, length, fp);
    fclose(fp);
    markdown[length] = '\0';

    // 解析 Markdown 文本
    cmark_node *document = cmark_parse_document(markdown, length, CMARK_OPT_DEFAULT);

    // 将文档渲染为 HTML
    char *html = cmark_render_html(document, CMARK_OPT_DEFAULT);
    char *html_output = html;
    printf("%s", html);

    size_t html_length = strlen(html_output);
    size_t bytes_sent = 0;

    headers(client, HTTP_OK);
    while (bytes_sent < html_length)
    {
        size_t remaining_bytes = html_length - bytes_sent;
        size_t send_size = (remaining_bytes > 4096) ? 4096 : remaining_bytes;
        int sent = write(client, html_output + bytes_sent, send_size);
        if (sent == -1)
        {
            perror("Error sending data");
            free(html);
            return;
        }
        bytes_sent += sent;
    }

    // 清理
    free(html);
    cmark_node_free(document);
    free(markdown);
    return;
}

/**
 * 渲染目录内容为HTML
 * @param originalBasePath 原始基础路径
 * @param currentPath 当前路径
 * @param html 用于存储生成的HTML的字符串
 * @param depth 目录深度
 */
void render_dir(char *originalBasePath, char *currentPath, char *html, int depth)
{
    char path[1000];
    struct dirent *dp;
    DIR *dir = opendir(currentPath);

    if (!dir)
        return;

    while ((dp = readdir(dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
        {
            // 为当前路径构建新路径
            strcpy(path, currentPath);
            strcat(path, "/");
            strcat(path, dp->d_name);

            //添加缩进
            for (int i = 0; i < depth; i++)
            {
                strcat(html, "&nbsp;&nbsp;&nbsp;&nbsp;");
            }

            if (dp->d_type == DT_DIR)
            {
                strcat(html, "<b>");
                strcat(html, dp->d_name);
                strcat(html, "</b><br>\n");
                render_dir(originalBasePath, path, html, depth + 1);
            }
            else if (strstr(dp->d_name, ".md") != NULL)
            {
                //计算相对路径
                char *relativePath = path + strlen(originalBasePath) + 1; // +1 以跳过 '/'

                strcat(html, "<a href='");
                strcat(html, relativePath); //使用相对路径
                strcat(html, "'>");
                strcat(html, dp->d_name);
                strcat(html, "</a><br>\n");
            }
        }
    }

    closedir(dir);
}

/**
 * 出错时退出程序
 * @param error 错误信息
 */
void exit_with_error(const char *error)
{
    printf("Error: %s\n", error);
    exit(EXIT_FAILURE);
}

/**
 * 从客户端读取一行数据
 * @param sock 客户端socket描述符
 * @param buf 缓冲区
 * @param size 缓冲区大小
 * @return 读取的字符数
 */
int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);
        if (n > 0)
        {
            if (c == '\r')
            {
                n = recv(sock, &c, 1, MSG_PEEK);
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        }
        else
            c = '\n';
    }
    buf[i] = '\0';

    return (i);
}

/**
 * 初始化服务器并开始监听
 * @param port 指向要监听的端口号的指针
 * @return 服务器socket描述符
 */
int startup(uint16_t *port)
{
    int opt = 1;
    int server_socket = 0;
    struct sockaddr_in name;

    server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == -1)
    {
        exit_with_error("server_socket");
    }

    if ((setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) < 0)
    {
        exit_with_error("setsockopt");
    }

    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(*port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_socket, (struct sockaddr *)&name, sizeof(name)) < 0)
    {
        exit_with_error("bind");
    }

    if (*port == 0)
    {
        socklen_t namelen = sizeof(name);
        if (getsockname(server_socket, (struct sockaddr *)&name, &namelen) < 0)
        {
            exit_with_error("getsockname");
        }

        *port = name.sin_port;
    }

    if (listen(server_socket, 20) < 0)
    {
        exit_with_error("listen");
    }

    return server_socket;
}

/**
 * 处理客户端请求
 * @param arg 客户端socket描述符
 */
void handle_request(void *arg)
{
    int client = (intptr_t)arg;
    char buf[1024];
    size_t numchars;
    char method[255];
    char url[255];
    char path[512];
    size_t i;
    size_t buf_index;
    struct stat st;

    numchars = get_line(client, buf, sizeof(buf));

    printf("%s", buf);

    buf_index = 0;

    // 解析method
    i = 0;
    while (!isspace(buf[i]) && (i < sizeof(method) - 1))
    {
        method[i] = buf[i];
        i++;
    }
    buf_index = i;
    method[i] = '\0';
    if (strcasecmp(method, "GET") != 0)
    {
        unimplemented(client);
        shutdown(client, SHUT_RDWR);
        close(client);
        return;
    }

    i = 0;
    while (isspace(buf[buf_index]) && (buf_index < numchars))
    {
        buf_index++;
    }

    // 解析url
    while (!isspace(buf[buf_index]) && (i < sizeof(url) - 1) && (buf_index < numchars))
    {
        url[i] = buf[buf_index];
        i++;
        buf_index++;
    }
    url[i] = '\0';

    //根目录请求
    if (strcasecmp(url, "/") == 0)
    {
        headers(client, HTTP_OK);
        char html[10000] = "<html><body>\n";
        render_dir("bin/mds", "bin/mds", html, 0);
        strcat(html, "</body></html>");
        printf("%s", html);
        write(client, html, strlen(html));
        shutdown(client, SHUT_RDWR);
        close(client);
        return;
    }

    //md文件请求
    sprintf(path, "bin/mds%s", url_docode(url));
    printf("%s", path);

    if (stat(path, &st) == -1)
    {
        while ((numchars > 0) && strcmp("\n", buf)) /* read & discard headers */
            numchars = get_line(client, buf, sizeof(buf));
        not_found(client);
    }
    else
    {
        if ((st.st_mode & S_IFMT) == S_IFREG)
        {
            render_markdown(client, path);
        }
    }
    shutdown(client, SHUT_RDWR);
    close(client);
}

int main(int argc, char *argv[])
{
    uint16_t port = 0;
    if (argc > 1)
    {
        port = atoi(argv[1]);
    }

    int server_sock = startup(&port);
    int client_sock = -1;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pthread_t newthread;

    printf("Now listening on: %d\n", port);

    while (1)
    {
        client_sock = accept(server_sock, (struct sockaddr *restrict)&client_addr, &client_addr_len);

        if (client_sock == -1)
        {
            exit_with_error("accept");
        }

        if (pthread_create(&newthread, NULL, (void *)handle_request, (void *)(intptr_t)client_sock) != 0)
        {
            exit_with_error("pthread_create");
        }
    }

    close(server_sock);

    return (EXIT_SUCCESS);
}