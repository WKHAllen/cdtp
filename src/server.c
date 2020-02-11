#include "server.h"

// Socket type
struct CDTPSocket
{
#ifdef _WIN32
    SOCKET sock;
#else
    int sock;
#endif
    struct sockaddr_in address;
};

// Socket server type
struct CDTPServer
{
    size_t max_clients;
    void (*on_recv      )(int, void *, void *);
    void (*on_connect   )(int, void *);
    void (*on_disconnect)(int, void *);
    void *on_recv_arg;
    void *on_connect_arg;
    void *on_disconnect_arg;
    int blocking;
    int event_blocking;
    int daemon;
    int serving;
    int done;
    int num_clients;
    CDTPSocket *sock;
    CDTPSocket **clients;
    int *allocated_clients;
};

EXPORT CDTPServer *cdtp_server(size_t max_clients,
                              void (*on_recv      )(int, void *, void *),
                              void (*on_connect   )(int, void *),
                              void (*on_disconnect)(int, void *),
                              void *on_recv_arg, void *on_connect_arg, void *on_disconnect_arg,
                              int blocking, int event_blocking, int daemon)
{
    CDTPServer *server = malloc(sizeof(*server));

    // Initialize the server object
    server->max_clients       = max_clients;
    server->on_recv           = on_recv;
    server->on_connect        = on_connect;
    server->on_disconnect     = on_disconnect;
    server->on_recv_arg       = on_recv_arg;
    server->on_connect_arg    = on_connect_arg;
    server->on_disconnect_arg = on_disconnect_arg;
    server->blocking          = blocking;
    server->event_blocking    = event_blocking;
    server->daemon            = daemon;
    server->serving           = CDTP_FALSE;
    server->done              = CDTP_FALSE;
    server->num_clients       = 0;

    // Initialize the library
    if (CDTP_INIT != CDTP_TRUE)
    {
        int return_code = _cdtp_init();
        if (return_code != 0)
        {
            _cdtp_set_error(CDTP_SERVER_WINSOCK_INIT_FAILED, return_code);
            return server;
        }
    }

    // Initialize the server socket
    server->sock = malloc(sizeof(*(server->sock)));

    // Initialize the socket info
    int opt = 1;
#ifdef _WIN32
    // Initialize the socket
    if ((server->sock->sock = socket(CDTP_ADDRESS_FAMILY, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        _cdtp_set_err(CDTP_SERVER_SOCK_INIT_FAILED);
        return server;
    }
    if (setsockopt(server->sock->sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) == SOCKET_ERROR)
    {
        _cdtp_set_err(CDTP_SERVER_SETSOCKOPT_FAILED);
        return server;
    }
#else
    // Initialize the socket
    if ((server->sock->sock = socket(CDTP_ADDRESS_FAMILY, SOCK_STREAM, 0)) == 0)
    {
        _cdtp_set_err(CDTP_SERVER_SOCK_INIT_FAILED);
        return server;
    }
    if (setsockopt(server->sock->sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        _cdtp_set_err(CDTP_SERVER_SETSOCKOPT_FAILED);
        return server;
    }
#endif

    // Initialize the client socket array
    server->clients = malloc(max_clients * sizeof(*(server->clients)));

    // Initialize the allocated clients array
    server->allocated_clients = malloc(max_clients * sizeof(*(server->allocated_clients)));
    for (int i = 0; i < max_clients; i++)
        server->allocated_clients[i] = CDTP_FALSE;

    return server;
}

EXPORT CDTPServer *cdtp_server_default(size_t max_clients,
                                      void (*on_recv      )(int, void *, void *),
                                      void (*on_connect   )(int, void *),
                                      void (*on_disconnect)(int, void *),
                                      void *on_recv_arg, void *on_connect_arg, void *on_disconnect_arg)
{
    return cdtp_server(max_clients, on_recv, on_connect, on_disconnect,
                       on_recv_arg, on_connect_arg, on_disconnect_arg,
                       CDTP_FALSE, CDTP_FALSE, CDTP_TRUE);
}

EXPORT void cdtp_server_start(CDTPServer *server, char *host, int port)
{
    // Change 'localhost' to '127.0.0.1'
    if (strcmp(host, "localhost") == 0)
        host = "127.0.0.1";

    // Make sure the server has not been run before
    if (server->done == CDTP_TRUE)
    {
        _cdtp_set_error(CDTP_SERVER_CANNOT_RESTART, 0);
        return;
    }

    // Make sure the server is not already serving
    if (server->serving == CDTP_TRUE)
    {
        _cdtp_set_error(CDTP_SERVER_ALREADY_SERVING, 0);
        return;
    }
    server->serving = CDTP_TRUE;

    // Set the server address
#ifdef _WIN32
    int addrlen = CDTP_ADDRSTRLEN;
    if (WSAStringToAddress(host, CDTP_ADDRESS_FAMILY, NULL, (LPSOCKADDR)&(server->sock->address), &addrlen) != 0)
    {
        _cdtp_set_err(CDTP_SERVER_ADDRESS_FAILED);
        return;
    }
#else
    if (inet_pton(CDTP_ADDRESS_FAMILY, host, &(server->sock->address)) != 1)
    {
        _cdtp_set_err(CDTP_SERVER_ADDRESS_FAILED);
        return;
    }
#endif
    server->sock->address.sin_family = CDTP_ADDRESS_FAMILY;
    server->sock->address.sin_port = htons(port);

    // Bind the address to the server
    if (bind(server->sock->sock, (struct sockaddr *)&(server->sock->address), sizeof(server->sock->address)) < 0)
    {
        _cdtp_set_err(CDTP_SERVER_BIND_FAILED);
        return;
    }

    // Listen for connections
    if (listen(server->sock->sock, CDTP_LISTEN_BACKLOG) < 0)
    {
        _cdtp_set_err(CDTP_SERVER_LISTEN_FAILED);
        return;
    }

    // Serve
    _cdtp_server_call_serve(server);
}

#ifdef _WIN32
EXPORT void cdtp_server_start_host(CDTPServer *server, ULONG host, int port)
#else
EXPORT void cdtp_server_start_host(CDTPServer *server, in_addr_t host, int port)
#endif
{
    // Make sure the server has not been run before
    if (server->done == CDTP_TRUE)
    {
        _cdtp_set_error(CDTP_SERVER_CANNOT_RESTART, 0);
        return;
    }

    // Make sure the server is not already serving
    if (server->serving == CDTP_TRUE)
    {
        _cdtp_set_error(CDTP_SERVER_ALREADY_SERVING, 0);
        return;
    }
    server->serving = CDTP_TRUE;

    // Set the server address
    server->sock->address.sin_family = CDTP_ADDRESS_FAMILY;
    server->sock->address.sin_addr.s_addr = host;
    server->sock->address.sin_port = htons(port);

    // Bind the address to the server
    if (bind(server->sock->sock, (struct sockaddr *)&(server->sock->address), sizeof(server->sock->address)) < 0)
    {
        _cdtp_set_err(CDTP_SERVER_BIND_FAILED);
        return;
    }

    // Listen for connections
    if (listen(server->sock->sock, CDTP_LISTEN_BACKLOG) < 0)
    {
        _cdtp_set_err(CDTP_SERVER_LISTEN_FAILED);
        return;
    }

    // Serve
    _cdtp_server_call_serve(server);
}

EXPORT void cdtp_server_start_default_host(CDTPServer *server, int port)
{
    cdtp_server_start_host(server, INADDR_ANY, port);
}

EXPORT void cdtp_server_start_default_port(CDTPServer *server, char *host)
{
    cdtp_server_start(server, host, CDTP_PORT);
}

#ifdef _WIN32
EXPORT void cdtp_server_start_host_default_port(CDTPServer *server, ULONG host)
#else
EXPORT void cdtp_server_start_host_default_port(CDTPServer *server, in_addr_t host)
#endif
{
    cdtp_server_start_host(server, host, CDTP_PORT);
}

EXPORT void cdtp_server_start_default(CDTPServer *server)
{
    cdtp_server_start_host(server, INADDR_ANY, CDTP_PORT);
}

EXPORT void cdtp_server_stop(CDTPServer *server)
{
    server->serving = CDTP_FALSE;
#ifdef _WIN32
    for (int i = 0; i < server->max_clients; i++)
        if (server->allocated_clients[i] == CDTP_TRUE)
            if (closesocket(server->clients[i]->sock) != 0)
            {
                _cdtp_set_err(CDTP_SERVER_STOP_FAILED);
                return;
            }
    if (closesocket(server->sock->sock) != 0)
    {
        _cdtp_set_err(CDTP_SERVER_STOP_FAILED);
        return;
    }
#else
    for (int i = 0; i < server->max_clients; i++)
        if (server->allocated_clients[i] == CDTP_TRUE)
            if (close(server->clients[i]->sock) != 0)
            {
                _cdtp_set_err(CDTP_SERVER_STOP_FAILED);
                return;
            }
    if (close(server->sock->sock) != 0)
    {
        _cdtp_set_err(CDTP_SERVER_STOP_FAILED);
        return;
    }
#endif
    free(server->sock);
    free(server->clients);
    free(server->allocated_clients);
    free(server);
    server->done = CDTP_TRUE;
}

EXPORT int cdtp_server_serving(CDTPServer *server)
{
    return server->serving;
}

EXPORT struct sockaddr_in cdtp_server_addr(CDTPServer *server)
{
    return server->sock->address;
}

EXPORT char *cdtp_server_host(CDTPServer *server)
{
    // Make sure the server is running
    if (server->serving != CDTP_TRUE)
    {
        _cdtp_set_error(CDTP_SERVER_NOT_SERVING, 0);
        return "";
    }

    char *addr = malloc(CDTP_ADDRSTRLEN * sizeof(char));
#ifdef _WIN32
    int addrlen = CDTP_ADDRSTRLEN;
    if (WSAAddressToString((LPSOCKADDR)&(server->sock->address), sizeof(server->sock->address), NULL, addr, (LPDWORD)&addrlen) != 0)
    {
        _cdtp_set_err(CDTP_SERVER_ADDRESS_FAILED);
        return "";
    }
    // Remove the port
    for (int i = 0; i < CDTP_ADDRSTRLEN && addr[i] != '\0'; i++)
    {
        if (addr[i] == ':')
        {
            addr[i] = '\0';
            break;
        }
    }
#else
    if (inet_ntop(CDTP_ADDRESS_FAMILY, &(server->sock->address), addr, CDTP_ADDRSTRLEN) == NULL)
    {
        _cdtp_set_err(CDTP_SERVER_ADDRESS_FAILED);
        return "";
    }
#endif
    return addr;
}

EXPORT int cdtp_server_port(CDTPServer *server)
{
    // Make sure the server is running
    if (server->serving != CDTP_TRUE)
    {
        _cdtp_set_error(CDTP_SERVER_NOT_SERVING, 0);
        return 0;
    }

    return ntohs(server->sock->address.sin_port);
}

EXPORT void cdtp_server_remove_client(CDTPServer *server, int client_id)
{
    // Make sure the server is running
    if (server->serving != CDTP_TRUE)
    {
        _cdtp_set_error(CDTP_SERVER_NOT_SERVING, 0);
        return;
    }

    if (client_id < 0 || client_id >= server->max_clients || server->allocated_clients[client_id] != CDTP_TRUE)
    {
        _cdtp_set_error(CDTP_CLIENT_DOES_NOT_EXIT, 0);
        return;
    }
#ifdef _WIN32
    if (closesocket(server->clients[client_id]->sock) != 0)
#else
    if (close(server->clients[client_id]->sock) != 0)
#endif
    {
        _cdtp_set_err(CDTP_CLIENT_REMOVE_FAILED);
        return;
    }
    server->allocated_clients[client_id] = CDTP_FALSE;
}

EXPORT void cdtp_server_send(CDTPServer *server, int client_id, void *data)
{
    // Make sure the server is running
    if (server->serving != CDTP_TRUE)
    {
        _cdtp_set_error(CDTP_SERVER_NOT_SERVING, 0);
        return;
    }

    // TODO: implement this function
}

EXPORT void cdtp_server_send_all(CDTPServer *server, void *data)
{
    // Make sure the server is running
    if (server->serving != CDTP_TRUE)
    {
        _cdtp_set_error(CDTP_SERVER_NOT_SERVING, 0);
        return;
    }

    for (int i = 0; i < server->max_clients; i++)
        if (server->allocated_clients[i] == CDTP_TRUE)
            cdtp_server_send(server, i, data);
}

void _cdtp_server_call_serve(CDTPServer *server)
{
    if (server->blocking == CDTP_TRUE)
        _cdtp_server_serve(server);
    else
    {
        // TODO: call `_cdtp_server_serve` using thread
    }
}

void _cdtp_server_serve(CDTPServer *server)
{
    // TODO: implement this function
}

void _cdtp_server_call_on_recv(CDTPServer *server, int client_id, void *data)
{
    if (server->event_blocking == CDTP_TRUE)
        (*(server->on_recv))(client_id, data, server->on_recv_arg);
    else
    {
        // TODO: call `server->on_recv` using thread
    }
}

void _cdtp_server_call_on_connect(CDTPServer *server, int client_id)
{
    if (server->event_blocking == CDTP_TRUE)
        (*(server->on_connect))(client_id, server->on_connect_arg);
    else
    {
        // TODO: call `server->on_connect` using thread
    }
}

void _cdtp_server_call_on_disconnect(CDTPServer *server, int client_id)
{
    if (server->event_blocking == CDTP_TRUE)
        (*(server->on_disconnect))(client_id, server->on_disconnect_arg);
    else
    {
        // TODO: call `server->on_disconnect` using thread
    }
}
