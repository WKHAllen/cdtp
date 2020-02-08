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
    server.num_clients       = 0;

    // Initialize the library
    if (CDTP_INIT != CDTP_TRUE)
    {
        int return_code = cdtp_init();
        if (return_code != 0)
        {
            *err = CDTP_SERVER_WINSOCK_INIT_FAILED;
            return server;
        }
    }

    // Initialize the server socket
    server.sock = malloc(sizeof(*server.sock));

    // Initialize the socket info
    int opt = 1;
#ifdef _WIN32
    // Initialize the socket
    if ((server.sock->sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        *err = CDTP_SERVER_SOCK_INIT_FAILED;
        return server;
    }
    if (setsockopt(server.sock->sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) == SOCKET_ERROR)
    {
        *err = CDTP_SERVER_SETSOCKOPT_FAILED;
        return server;
    }
#else
    // Initialize the socket
    if ((server.sock->sock = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        *err = CDTP_SERVER_SOCK_INIT_FAILED;
        return server;
    }
    if (setsockopt(server.sock->sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        *err = CDTP_SERVER_SETSOCKOPT_FAILED;
        return server;
    }
#endif

    // Initialize the client socket array
    server.clients = malloc(max_clients * sizeof(*server.clients));

    // Initialize the allocated clients array
    server.allocated_clients = malloc(max_clients * sizeof(*server.allocated_clients));
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

EXPORT int cdtp_server_start(CDTPServer *server, char *host, int port)
{
    // Make sure the server is not already serving
    if (server->serving == CDTP_TRUE)
        return CDTP_SERVER_ALREADY_SERVING;
    server->serving = CDTP_TRUE;

    // Set the server address
    server->sock->address.sin_family = AF_INET;
    server->sock->address.sin_addr.s_addr = inet_addr(host);
    server->sock->address.sin_port = htons(port);

    // Bind the address to the server
    if (bind(server->sock->sock, (struct sockaddr *)&server->sock->address, sizeof(server->sock->address)) < 0)
        return CDTP_SERVER_BIND_FAILED;

    // Listen for connections
    if (listen(server->sock->sock, CDTP_LISTEN_BACKLOG) < 0)
        return CDTP_SERVER_LISTEN_FAILED;

    // Serve
    if (server->blocking)
    {
        _cdtp_server_serve(server);
    }
    else
    {
        // TODO: call `cdtp_serve` using thread
    }
    
    return CDTP_SERVER_SUCCESS;
}

#ifdef _WIN32
EXPORT int cdtp_server_start_host(CDTPServer *server, ULONG host, int port)
#else
EXPORT int cdtp_server_start_host(CDTPServer *server, in_addr_t host, int port)
#endif
{
    // Make sure the server is not already serving
    if (server->serving == CDTP_TRUE)
        return CDTP_SERVER_ALREADY_SERVING;
    server->serving = CDTP_TRUE;

    // Set the server address
    server->sock->address.sin_family = AF_INET;
    server->sock->address.sin_addr.s_addr = host;
    server->sock->address.sin_port = htons(port);

    // Bind the address to the server
    if (bind(server->sock->sock, (struct sockaddr *)&server->sock->address, sizeof(server->sock->address)) < 0)
        return CDTP_SERVER_BIND_FAILED;

    // Listen for connections
    if (listen(server->sock->sock, CDTP_LISTEN_BACKLOG) < 0)
        return CDTP_SERVER_LISTEN_FAILED;

    // Serve
    if (server->blocking)
    {
        _cdtp_server_serve(server);
    }
    else
    {
        // TODO: call `cdtp_serve` using thread
    }
    
    return CDTP_SERVER_SUCCESS;
}

EXPORT int cdtp_server_start_default_host(CDTPServer *server, int port)
{
    return cdtp_server_start_host(server, INADDR_ANY, port);
}

EXPORT int cdtp_server_start_default(CDTPServer *server)
{
    return cdtp_server_start_host(server, INADDR_ANY, 0);
}

EXPORT void cdtp_server_stop(CDTPServer *server)
{
    server->serving = CDTP_FALSE;
#ifdef _WIN32
    closesocket(server->sock->sock);
    for (int i = 0; i < server->max_clients; i++)
        if (server->allocated_clients[i] == CDTP_TRUE)
            closesocket(server->clients[i]->sock);
#else
    close(server->sock->sock);
    for (int i = 0; i < server->max_clients; i++)
        if (server->allocated_clients[i] == CDTP_TRUE)
            close(server->clients[i]->sock);
#endif
    free(server->sock);
    free(server->clients);
    free(server->allocated_clients);
    // TODO: complete this function
}

EXPORT int cdtp_server_serving(CDTPServer *server)
{
    return server->serving;
}

void _cdtp_server_serve(CDTPServer *server)
{
    // TODO: implement this function
}
