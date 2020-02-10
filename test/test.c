#include "../bin/include/cdtp.h"
#include <stdio.h>
#include <assert.h>

void tmp(int a, void *b, void *c)
{
    printf("tmp function called\n");
}

int main(int argc, char **argv)
{
    printf("Running tests...\n");

    int ret;
    int err;
    CDTPServer *server = cdtp_server_default(16, tmp, tmp, tmp, NULL, NULL, NULL, &err);
    if (err != CDTP_SERVER_SUCCESS)
    {
        printf("CDTP Error: %d\n", err);
        exit(EXIT_FAILURE);
    }
    char *host = "127.0.0.1";
    ret = cdtp_server_start_default_port(server, host, &err);
    if (ret != CDTP_SERVER_SUCCESS)
    {
        printf("CDTP Error: %d\n", ret);
        printf("OS Error:   %d\n", err);
        exit(EXIT_FAILURE);
    }
    char *ip_address = cdtp_server_host(server);
    int port = cdtp_server_port(server);
    printf("IP address: %s\n", ip_address);
    printf("Port:       %d\n", port);
    free(ip_address);
    cdtp_server_stop(server);

    printf("Successfully passed all tests\n");
    return 0;
}
