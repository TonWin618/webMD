#include <stdio.h>

#define HTTP_OK "200 OK"
#define HTTP_NOT_FOUND "404 NOT FOUND"
#define HTTP_METHOD_NOT_IMPLEMENTED "501 Method Not Implemented"

void headers(int client, const char *code);
void unimplemented(int client);
void not_found(int client);

void render_dir(char *originalBasePath, char *currentPath, char *html, int depth);
void render_html(int client, const char *filename);
void render_markdown(int client, const char *filename);