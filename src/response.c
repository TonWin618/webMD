#include <dirent.h>
#include <cmark.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "response.h"

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
    char buf[1024];

    buf[0] = 'A';
    buf[1] = '\0';

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