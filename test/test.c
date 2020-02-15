#include "../bin/include/cdtp.h"
#include <stdio.h>
#include <assert.h>

void on_recv(int client_id, void *data, size_t data_len, void *arg)
{
    printf("Received data from client #%d: %s (size %ld)\n", client_id, (char *)data, data_len);
}

void on_connect(int client_id, void *arg)
{
    printf("Client #%d connected\n", client_id);
}

void on_disconnect(int client_id, void *arg)
{
    printf("Client #%d disconnected\n", client_id);
}

void on_err(int cdtp_err, int underlying_err, void *arg)
{
    printf("CDTP error:       %d\n", cdtp_err);
    printf("Underlying error: %d\n", underlying_err);
    exit(EXIT_FAILURE);
}

void check_err(void)
{
    if (cdtp_error())
        on_err(cdtp_get_error(), cdtp_get_underlying_error(), NULL);
}

int main(int argc, char **argv)
{
    printf("Running tests...\n");

    // Register error event
    cdtp_on_error(on_err, NULL);

    // Server initialization
    CDTPServer *server = cdtp_server(16,
                                     on_recv, on_connect, on_disconnect,
                                     NULL, NULL, NULL,
                                     CDTP_FALSE, CDTP_FALSE);

    // Server start
    char *host = "127.0.0.1";
    cdtp_server_start_default_port(server, host);

    // Get IP address and port
    char *ip_address = cdtp_server_host(server);
    int port = cdtp_server_port(server);
    printf("IP address: %s\n", ip_address);
    printf("Port:       %d\n", port);
    free(ip_address);

    // Unregister error event
    cdtp_on_error_clear();

    // Test that the client does not exist
    cdtp_server_remove_client(server, 0);
    assert(cdtp_get_error() == CDTP_CLIENT_DOES_NOT_EXIST);

    // Server stop
    cdtp_server_stop(server);
    check_err();

    // Test that server cannot be restarted
    cdtp_server_start_default_port(server, host);
    assert(cdtp_get_error() == CDTP_SERVER_CANNOT_RESTART);

    printf("Successfully passed all tests\n");
    return 0;
}
