#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>

#include "server.h"
#include "utils.h"
#include "response.h"

void exit_with_error(const char *error);
int get_line(int sock, char *buf, int size);
void handle_request(void *arg);

int server_socket = 0;

/**
 * 初始化服务器并开始监听
 * @param port 指向要监听的端口号的指针
 * @return 服务器socket描述符
 */
int server_startup(uint16_t *port)
{
    int opt = 1;
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
 * 接受客户端连接请求
 */
void server_accept_connection()
{
    int client_socket;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pthread_t newthread;

    client_socket = accept(server_socket, (struct sockaddr *restrict)&client_addr, &client_addr_len);

    if (client_socket == -1)
    {
        exit_with_error("accept");
    }

    if (pthread_create(&newthread, NULL, (void *)handle_request, (void *)(intptr_t)client_socket) != 0)
    {
        exit_with_error("pthread_create");
    }
}

/**
 * 停止服务器
 * 关闭服务器的主socket，从而停止接受新的连接请求
 * @return 如果成功关闭则返回0，否则返回错误代码
 */
int server_stop()
{
    return close(server_socket);
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

/**
 * 出错时退出程序
 * @param error 错误信息
 */
void exit_with_error(const char *error)
{
    printf("Error: %s\n", error);
    exit(1);
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
