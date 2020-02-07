#include "util.h"
#include "server.h"

EXPORT CDTPServer cdtp_server(size_t max_clients,
                              void (*on_recv      )(int, void *, void *),
                              void (*on_connect   )(int, void *, void *),
                              void (*on_disconnect)(int, void *, void *),
                              void *on_recv_arg, void *on_connect_arg, void *on_disconnect_arg,
                              int blocking, int event_blocking, int daemon,
                              int *err)
{
    CDTPServer server;

#if defined(_WIN32) && !defined(CDTP_WINSOCK_INIT)
#define CDTP_WINSOCK_INIT
    // Initialize winsock
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        *err = CDTP_SERVER_WINSOCK_INIT_FAILED;
        return server;
    }
#endif

    // Initialize the server object
    server.max_clients       = max_clients;
    server.on_recv           = on_recv;
    server.on_connect        = on_connect;
    server.on_disconnect     = on_disconnect;
    server.on_recv_arg       = on_recv_arg;
    server.on_connect_arg    = on_connect_arg;
    server.on_disconnect_arg = on_disconnect_arg;
    server.blocking          = blocking;
    server.event_blocking    = event_blocking;
    server.daemon            = daemon;
    server.serving           = CDTP_FALSE;

    // Initialize the socket info
    int opt = 1;
#ifdef _WIN32
    // Initialize the socket
    if ((server.sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        *err = CDTP_SERVER_SOCK_INIT_FAILED;
        return server;
    }
    if (setsockopt(server.sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) == SOCKET_ERROR)
    {
        *err = CDTP_SERVER_SETSOCKOPT_FAILED;
        return server;
    }

    // Initialize the client socket array
    server.clients = malloc(max_clients * sizeof(SOCKET));
#else
    // Initialize the socket
    if ((server.sock = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        *err = CDTP_SERVER_SOCK_INIT_FAILED;
        return server;
    }
    if (setsockopt(server.sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        *err = CDTP_SERVER_SETSOCKOPT_FAILED;
        return server;
    }

    // Initialize the client socket array
    server.clients = malloc(max_clients * sizeof(int));
#endif

    // Initialize the allocated clients array
    server.allocated_clients = malloc(max_clients * sizeof(int));
    for (int i = 0; i < max_clients; i++)
        server.allocated_clients[i] = CDTP_FALSE;
    
    *err = CDTP_SERVER_SUCCESS;
    return server;
}

EXPORT CDTPServer cdtp_server_default(size_t max_clients,
                                      void (*on_recv      )(int, void *, void *),
                                      void (*on_connect   )(int, void *, void *),
                                      void (*on_disconnect)(int, void *, void *),
                                      void *on_recv_arg, void *on_connect_arg, void *on_disconnect_arg,
                                      int *err)
{
    return cdtp_server(max_clients, on_recv, on_connect, on_disconnect,
                       on_recv_arg, on_connect_arg, on_disconnect_arg,
                       CDTP_FALSE, CDTP_FALSE, CDTP_TRUE, err);
}

EXPORT int cdtp_start(CDTPServer *server, char *host, int port)
{
    // Make sure the server is not already serving
    if (server->serving == CDTP_TRUE)
        return CDTP_SERVER_ALREADY_SERVING;
    server->serving = CDTP_TRUE;

    // Set the server address
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(host);
    address.sin_port = htons(port);
    
    // Bind the address to the server
    if (bind(server->sock, (struct sockaddr *)&address, sizeof(address)) < 0)
        return CDTP_SERVER_BIND_FAILED;

    // Listen for connections
    if (listen(server->sock, CDTP_LISTEN_BACKLOG) < 0)
        return CDTP_SERVER_LISTEN_FAILED;

    // Serve
    if (server->blocking)
    {
        cdtp_serve(server);
    }
    else
    {
        // TODO: call `cdtp_serve` using thread
    }
    
    return CDTP_SERVER_SUCCESS;
}

#ifdef _WIN32
EXPORT int cdtp_start_host(CDTPServer *server, ULONG host, int port)
#else
EXPORT int cdtp_start_host(CDTPServer *server, in_addr_t host, int port)
#endif
{
    // Make sure the server is not already serving
    if (server->serving == CDTP_TRUE)
        return CDTP_SERVER_ALREADY_SERVING;
    server->serving = CDTP_TRUE;

    // Set the server address
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = host;
    address.sin_port = htons(port);
    
    // Bind the address to the server
    if (bind(server->sock, (struct sockaddr *)&address, sizeof(address)) < 0)
        return CDTP_SERVER_BIND_FAILED;

    // Listen for connections
    if (listen(server->sock, CDTP_LISTEN_BACKLOG) < 0)
        return CDTP_SERVER_LISTEN_FAILED;

    // Serve
    if (server->blocking)
    {
        cdtp_serve(server);
    }
    else
    {
        // TODO: call `cdtp_serve` using thread
    }
    
    return CDTP_SERVER_SUCCESS;
}

EXPORT int cdtp_start_default_host(CDTPServer *server, int port)
{
    return cdtp_start(server, INADDR_ANY, port);
}

EXPORT int cdtp_start_default(CDTPServer *server)
{
    return cdtp_start(server, INADDR_ANY, 0);
}

EXPORT void cdtp_stop(CDTPServer *server)
{
    server->serving = CDTP_FALSE;
#ifdef _WIN32
    closesocket(server->sock);
    for (int i = 0; i < server->max_clients; i++)
        if (server->allocated_clients[i] == CDTP_TRUE)
            closesocket(server->clients[i]);
#else
    close(server->sock);
    for (int i = 0; i < server->max_clients; i++)
        if (server->allocated_clients[i] == CDTP_TRUE)
            close(server->clients[i]);
#endif
    free(server->clients);
    free(server->allocated_clients);
    // TODO: complete this function
}

EXPORT int cdtp_serving(CDTPServer *server)
{
    return server->serving;
}

void cdtp_serve(CDTPServer *server)
{
    // TODO: implement this function
}
