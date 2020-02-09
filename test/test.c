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

    int err;
    CDTPServer *server = cdtp_server_default(16, tmp, tmp, tmp, NULL, NULL, NULL, &err);
    assert(err == CDTP_SERVER_SUCCESS);
    char *host = "127.0.0.1";
    assert(cdtp_server_start_default_port(server, host) == CDTP_SERVER_SUCCESS);
    char *ip_address = cdtp_server_ip(server);
    printf("IP address: %s\n", ip_address);
    free(ip_address);
    cdtp_server_stop(server);

    printf("Successfully passed all tests\n");
    return 0;
}
