#include "../bin/include/cdtp.h"
#include <stdio.h>
#include <assert.h>

void on_recv(int client_id, void *data, void *arg)
{
    printf("Received data from client #%d: %s\n", client_id, *(char **)data);
}

void on_connect(int client_id, void *arg)
{
    printf("Client #%d connected\n", client_id);
}

void on_disconnect(int client_id, void *arg)
{
    printf("Client #%d disconnected\n", client_id);
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

    // Server initialization
    CDTPServer *server = cdtp_server_default(16, on_recv, on_connect, on_disconnect, NULL, NULL, NULL);
    check_err();

    // Server start
    char *host = "127.0.0.1";
    cdtp_server_start_default_port(server, host);
    check_err();

    // Get IP address and port
    char *ip_address = cdtp_server_host(server);
    check_err();
    int port = cdtp_server_port(server);
    printf("IP address: %s\n", ip_address);
    printf("Port:       %d\n", port);
    free(ip_address);

    // Test that the client does not exist
    cdtp_server_remove_client(server, 0);
    assert(cdtp_get_error() == CDTP_CLIENT_DOES_NOT_EXIT);

    // Server stop
    cdtp_server_stop(server);
    check_err();

    // Test that server cannot be restarted
    cdtp_server_start_default_port(server, host);
    assert(cdtp_get_error() == CDTP_SERVER_CANNOT_RESTART);

    printf("Successfully passed all tests\n");
    return 0;
}
