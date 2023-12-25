#include <server.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    uint16_t port = 0;
    if (argc > 1)
    {
        port = atoi(argv[1]);
    }
    server_startup(&port);

    printf("Now listening on: %d\n", port);

    while (1)
    {
        server_accept_connection();
    }
    
    server_stop();
    
    return (0);
}