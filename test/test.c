#include "../bin/include/cdtp.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <time.h>
#endif

void wait(double seconds)
{
#ifdef _WIN32
    Sleep(seconds * 1000);
#else
    struct timespec ts;
    ts.tv_sec = seconds;
    ts.tv_nsec = ((seconds * 1000) % 1000) * 1000000;
    nanosleep(&ts, NULL);
#endif
}

void server_on_recv(int client_id, void *data, size_t data_size, void *arg)
{
    printf("Received data from client #%d: %s (size %ld)\n", client_id, (char *)data, data_size);
    free(data);
}

void server_on_connect(int client_id, void *arg)
{
    printf("Client #%d connected\n", client_id);
}

void server_on_disconnect(int client_id, void *arg)
{
    printf("Client #%d disconnected\n", client_id);
}

void client_on_recv(void *data, size_t data_size, void *arg)
{
    printf("Received from server: %s (size %ld)\n", (char *)data, data_size);
    free(data);
}

void client_on_disconnected(void *arg)
{
    printf("Unexpectedly disconnected from server\n");
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
    double wait_time = 0.1;

    printf("Running tests...\n");

    // Register error event
    cdtp_on_error(on_err, NULL);

    // Server initialization
    CDTPServer *server = cdtp_server(16,
                                     server_on_recv, server_on_connect, server_on_disconnect,
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

    // Register error event
    cdtp_on_error(on_err, NULL);

    wait(wait_time);

    // Client initialization
    CDTPClient *client = cdtp_client(client_on_recv, client_on_disconnected,
                                     NULL, NULL,
                                     CDTP_FALSE, CDTP_FALSE);

    // Client connect
    cdtp_client_connect_default_port(client, host);

    // Get IP address and port
    char *client_ip_address = cdtp_client_host(client);
    int client_port = cdtp_client_port(client);
    printf("IP address: %s\n", client_ip_address);
    printf("Port:       %d\n", client_port);
    free(client_ip_address);

    // Client send
    char *client_message = "Hello, server.";
    cdtp_client_send(client, client_message, strlen(client_message));

    wait(wait_time);

    // Client disconnect
    cdtp_client_disconnect(client);

    // Unregister error event
    cdtp_on_error_clear();

    wait(wait_time);

    // Server stop
    cdtp_server_stop(server);
    check_err();

    // Test that server cannot be restarted
    cdtp_server_start_default_port(server, host);
    assert(cdtp_get_error() == CDTP_SERVER_CANNOT_RESTART);

    printf("Successfully passed all tests\n");
    return 0;
}
