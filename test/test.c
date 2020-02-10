#include "../bin/include/cdtp.h"
#include <stdio.h>
#include <assert.h>

void tmp(int a, void *b, void *c)
{
    printf("tmp function called\n");
}

void check_err(void)
{
    if (cdtp_error())
    {
        printf("CDTP error:       %d\n", cdtp_get_error());
        printf("Underlying error: %d\n", cdtp_get_underlying_error());
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv)
{
    printf("Running tests...\n");

    CDTPServer *server = cdtp_server_default(16, tmp, tmp, tmp, NULL, NULL, NULL);
    check_err();
    char *host = "127.0.0.1";
    cdtp_server_start_default_port(server, host);
    check_err();
    char *ip_address = cdtp_server_host(server);
    check_err();
    int port = cdtp_server_port(server);
    printf("IP address: %s\n", ip_address);
    printf("Port:       %d\n", port);
    free(ip_address);
    cdtp_server_stop(server);
    check_err();

    printf("Successfully passed all tests\n");
    return 0;
}
