#include "client.h"

EXPORT CDTPClient *cdtp_client(void (*on_recv        )(void *, size_t, void *),
                               void (*on_disconnected)(void *),
                               void *on_recv_arg, void *on_disconnected_arg,
                               int blocking, int event_blocking)
{
    CDTPClient *client = malloc(sizeof(*client));

    // Initialize the client object
    client->on_recv             = on_recv;
    client->on_disconnected     = on_disconnected;
    client->on_recv_arg         = on_recv_arg;
    client->on_disconnected_arg = on_disconnected_arg;
    client->blocking            = blocking;
    client->event_blocking      = event_blocking;
    client->connected           = CDTP_FALSE;
    client->done                = CDTP_FALSE;

    // Initialize the library
    if (CDTP_INIT != CDTP_TRUE)
    {
        int return_code = _cdtp_init();
        if (return_code != 0)
        {
            _cdtp_set_error(CDTP_WINSOCK_INIT_FAILED, return_code);
            return client;
        }
    }

    // Initialize the client socket
    client->sock = malloc(sizeof(*(client->sock)));

    // Initialize the socket info
#ifdef _WIN32
    // Initialize the socket
    if ((client->sock->sock = socket(CDTP_ADDRESS_FAMILY, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        _cdtp_set_err(CDTP_CLIENT_SOCK_INIT_FAILED);
        return client;
    }
#else
    // Initialize the socket
    if ((client->sock->sock = socket(CDTP_ADDRESS_FAMILY, SOCK_STREAM, 0)) == 0)
    {
        _cdtp_set_err(CDTP_CLIENT_SOCK_INIT_FAILED);
        return client;
    }
#endif

    return client;
}

EXPORT CDTPClient *cdtp_client_default(void (*on_recv        )(void *, size_t, void *),
                                       void (*on_disconnected)(void *),
                                       void *on_recv_arg, void *on_disconnected_arg)
{
    return cdtp_client(on_recv, on_disconnected, on_recv_arg, on_disconnected_arg,
                       CDTP_FALSE, CDTP_FALSE);
}

EXPORT void cdtp_client_connect(CDTPClient *client, char *host, int port)
{
    // Change 'localhost' to '127.0.0.1'
    if (strcmp(host, "localhost") == 0)
        host = "127.0.0.1";

    // Make sure the client has not connected before
    if (client->done == CDTP_TRUE)
    {
        _cdtp_set_error(CDTP_CLIENT_CANNOT_RECONNECT, 0);
        return;
    }

    // Make sure the client is not already connected
    if (client->connected == CDTP_TRUE)
    {
        _cdtp_set_error(CDTP_CLIENT_ALREADY_CONNECTED, 0);
        return;
    }

    // Set the client address
#ifdef _WIN32
    int addrlen = CDTP_ADDRSTRLEN;
    if (WSAStringToAddress(host, CDTP_ADDRESS_FAMILY, NULL, (LPSOCKADDR)&(client->sock->address), &addrlen) != 0)
    {
        _cdtp_set_err(CDTP_CLIENT_ADDRESS_FAILED);
        return;
    }
#else
    if (inet_pton(CDTP_ADDRESS_FAMILY, host, &(client->sock->address)) != 1)
    {
        _cdtp_set_err(CDTP_CLIENT_ADDRESS_FAILED);
        return;
    }
#endif
    client->sock->address.sin_family = CDTP_ADDRESS_FAMILY;
    client->sock->address.sin_port = htons(port);

    if (connect(client->sock->sock, (struct sockaddr *)&(client->sock->address), sizeof(client->sock->address)) < 0)
    {
        _cdtp_set_err(CDTP_CLIENT_CONNECT_FAILED);
        return;
    }

    // Check the return code
    char size_buffer[CDTP_LENSIZE];
#ifdef _WIN32
    int recv_code = recv(client->sock->sock, size_buffer, CDTP_LENSIZE, 0);

    if (recv_code == SOCKET_ERROR)
    {
        int err_code = WSAGetLastError();
        if (err_code == WSAECONNRESET)
        {
            cdtp_client_disconnect(client);
            _cdtp_client_call_on_disconnected(client);
            return;
        }
        else
        {
            _cdtp_set_error(CDTP_CLIENT_RECV_FAILED, err_code);
            return;
        }
    }
    else if (recv_code == 0)
    {
        cdtp_client_disconnect(client);
        _cdtp_client_call_on_disconnected(client);
        return;
    }
    else
    {
        size_t msg_size = _cdtp_ascii_to_dec(size_buffer);
        char *buffer = malloc(msg_size * sizeof(char));
        recv_code = recv(client->sock->sock, buffer, msg_size, 0);
        if (recv_code == SOCKET_ERROR)
        {
            int err_code = WSAGetLastError();
            if (err_code == WSAECONNRESET)
            {
                cdtp_client_disconnect(client);
                _cdtp_client_call_on_disconnected(client);
                return;
            }
            else
            {
                _cdtp_set_error(CDTP_CLIENT_RECV_FAILED, err_code);
                return;
            }
        }
        else if (recv_code == 0)
        {
            cdtp_client_disconnect(client);
            _cdtp_client_call_on_disconnected(client);
            return;
        }
        else
        {
            int connect_code = *(int *)buffer;
            if (connect_code == CDTP_SERVER_FULL)
            {
                _cdtp_set_error(CDTP_SERVER_FULL, 0);
                cdtp_client_disconnect(client);
                _cdtp_client_call_on_disconnected(client);
                return;
            }
        }
    }
#else
    int recv_code = read(client->sock->sock, size_buffer, CDTP_LENSIZE);
    if (recv_code == 0)
    {
        cdtp_client_disconnect(client);
        _cdtp_client_call_on_disconnected(client);
        return;
    }
    else
    {
        size_t msg_size = _cdtp_ascii_to_dec(size_buffer);
        char *buffer = malloc(msg_size * sizeof(char));
        recv_code = read(client->sock->sock, buffer, msg_size);
        if (recv_code == 0)
        {
            cdtp_client_disconnect(client);
            _cdtp_client_call_on_disconnected(client);
            return;
        }
        else
        {
            int connect_code = *(int *)buffer;
            if (connect_code == CDTP_SERVER_FULL)
            {
                _cdtp_set_error(CDTP_SERVER_FULL, 0);
                cdtp_client_disconnect(client);
                _cdtp_client_call_on_disconnected(client);
                return;
            }
        }
    }
#endif

    // Handle received data
    client->connected = CDTP_TRUE;
    _cdtp_client_call_handle(client);
}

#ifdef _WIN32
EXPORT void cdtp_client_connect_host(CDTPClient *client, ULONG host, int port)
#else
EXPORT void cdtp_client_connect_host(CDTPClient *client, in_addr_t host, int port)
#endif
{
    // Make sure the client has not connected before
    if (client->done == CDTP_TRUE)
    {
        _cdtp_set_error(CDTP_CLIENT_CANNOT_RECONNECT, 0);
        return;
    }

    // Make sure the client is not already connected
    if (client->connected == CDTP_TRUE)
    {
        _cdtp_set_error(CDTP_CLIENT_ALREADY_CONNECTED, 0);
        return;
    }

    // Set the client address
    client->sock->address.sin_family = CDTP_ADDRESS_FAMILY;
    client->sock->address.sin_addr.s_addr = host;
    client->sock->address.sin_port = htons(port);

    if (connect(client->sock->sock, (struct sockaddr *)&(client->sock->address), sizeof(client->sock->address)) < 0)
    {
        _cdtp_set_err(CDTP_CLIENT_CONNECT_FAILED);
        return;
    }

    // Check the return code
    char size_buffer[CDTP_LENSIZE];
#ifdef _WIN32
    int recv_code = recv(client->sock->sock, size_buffer, CDTP_LENSIZE, 0);

    if (recv_code == SOCKET_ERROR)
    {
        int err_code = WSAGetLastError();
        if (err_code == WSAECONNRESET)
        {
            cdtp_client_disconnect(client);
            _cdtp_client_call_on_disconnected(client);
            return;
        }
        else
        {
            _cdtp_set_error(CDTP_CLIENT_RECV_FAILED, err_code);
            return;
        }
    }
    else if (recv_code == 0)
    {
        cdtp_client_disconnect(client);
        _cdtp_client_call_on_disconnected(client);
        return;
    }
    else
    {
        size_t msg_size = _cdtp_ascii_to_dec(size_buffer);
        char *buffer = malloc(msg_size * sizeof(char));
        recv_code = recv(client->sock->sock, buffer, msg_size, 0);
        if (recv_code == SOCKET_ERROR)
        {
            int err_code = WSAGetLastError();
            if (err_code == WSAECONNRESET)
            {
                cdtp_client_disconnect(client);
                _cdtp_client_call_on_disconnected(client);
                return;
            }
            else
            {
                _cdtp_set_error(CDTP_CLIENT_RECV_FAILED, err_code);
                return;
            }
        }
        else if (recv_code == 0)
        {
            cdtp_client_disconnect(client);
            _cdtp_client_call_on_disconnected(client);
            return;
        }
        else
        {
            int connect_code = *(int *)buffer;
            if (connect_code == CDTP_SERVER_FULL)
            {
                _cdtp_set_error(CDTP_SERVER_FULL, 0);
                cdtp_client_disconnect(client);
                _cdtp_client_call_on_disconnected(client);
                return;
            }
        }
    }
#else
    int recv_code = read(client->sock->sock, size_buffer, CDTP_LENSIZE);
    if (recv_code == 0)
    {
        cdtp_client_disconnect(client);
        _cdtp_client_call_on_disconnected(client);
        return;
    }
    else
    {
        size_t msg_size = _cdtp_ascii_to_dec(size_buffer);
        char *buffer = malloc(msg_size * sizeof(char));
        recv_code = read(client->sock->sock, buffer, msg_size);
        if (recv_code == 0)
        {
            cdtp_client_disconnect(client);
            _cdtp_client_call_on_disconnected(client);
            return;
        }
        else
        {
            int connect_code = *(int *)buffer;
            if (connect_code == CDTP_SERVER_FULL)
            {
                _cdtp_set_error(CDTP_SERVER_FULL, 0);
                cdtp_client_disconnect(client);
                _cdtp_client_call_on_disconnected(client);
                return;
            }
        }
    }
#endif

    // Handle received data
    client->connected = CDTP_TRUE;
    _cdtp_client_call_handle(client);
}

EXPORT void cdtp_client_connect_default_host(CDTPClient *client, int port)
{
    cdtp_client_connect_host(client, INADDR_ANY, port);
}

EXPORT void cdtp_client_connect_default_port(CDTPClient *client, char *host)
{
    cdtp_client_connect(client, host, CDTP_PORT);
}

#ifdef _WIN32
EXPORT void cdtp_client_connect_host_default_port(CDTPClient *client, ULONG host)
#else
EXPORT void cdtp_client_connect_host_default_port(CDTPClient *client, in_addr_t host)
#endif
{
    cdtp_client_connect_host(client, host, CDTP_PORT);
}

EXPORT void cdtp_client_connect_default(CDTPClient *client)
{
    cdtp_client_connect_host(client, INADDR_ANY, CDTP_PORT);
}

EXPORT void cdtp_client_disconnect(CDTPClient *client)
{
    client->connected = CDTP_FALSE;

#ifdef _WIN32
    // Close the socket
    if (closesocket(client->sock->sock) != 0)
    {
        _cdtp_set_err(CDTP_CLIENT_DISCONNECT_FAILED);
        return;
    }

    // Wait for threads to exit
    if (client->blocking != CDTP_TRUE && WaitForSingleObject(client->handle_thread, INFINITE) == WAIT_FAILED)
    {
        _cdtp_set_error(CDTP_HANDLE_THREAD_NOT_CLOSING, GetLastError());
        return;
    }
#else
    // Close the socket
    if (close(client->sock->sock) != 0)
    {
        _cdtp_set_err(CDTP_CLIENT_DISCONNECT_FAILED);
        return;
    }

    // Wait for threads to exit
    if (client->blocking != CDTP_TRUE)
    {
        int err_code = pthread_join(client->handle_thread, NULL);
        if (err_code != 0)
        {
            _cdtp_set_error(CDTP_HANDLE_THREAD_NOT_CLOSING, err_code);
            return;
        }
    }
#endif

    // Free memory
    free(client->sock);
    free(client);
    client->done = CDTP_TRUE;
}

EXPORT int cdtp_client_connected(CDTPClient *client)
{
    return client->connected;
}

EXPORT struct sockaddr_in cdtp_client_addr(CDTPClient *client)
{
    return client->sock->address;
}

EXPORT char *cdtp_client_host(CDTPClient *client)
{
    // Make sure the client is connected
    if (client->connected != CDTP_TRUE)
    {
        _cdtp_set_error(CDTP_CLIENT_NOT_CONNECTED, 0);
        return "";
    }

    char *addr = malloc(CDTP_ADDRSTRLEN * sizeof(char));
#ifdef _WIN32
    int addrlen = CDTP_ADDRSTRLEN;
    if (WSAAddressToString((LPSOCKADDR)&(client->sock->address), sizeof(client->sock->address), NULL, addr, (LPDWORD)&addrlen) != 0)
    {
        _cdtp_set_err(CDTP_CLIENT_ADDRESS_FAILED);
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
    if (inet_ntop(CDTP_ADDRESS_FAMILY, &(client->sock->address), addr, CDTP_ADDRSTRLEN) == NULL)
    {
        _cdtp_set_err(CDTP_CLIENT_ADDRESS_FAILED);
        return "";
    }
#endif
    return addr;
}

EXPORT int cdtp_client_port(CDTPClient *client)
{
    // Make sure the server is running
    if (client->connected != CDTP_TRUE)
    {
        _cdtp_set_error(CDTP_CLIENT_NOT_CONNECTED, 0);
        return 0;
    }

    return ntohs(client->sock->address.sin_port);
}

EXPORT void cdtp_client_send(CDTPClient *client, void *data, size_t data_size)
{
    // Make sure the client is connected
    if (client->connected != CDTP_TRUE)
    {
        _cdtp_set_error(CDTP_CLIENT_NOT_CONNECTED, 0);
        return;
    }

    char *message = _cdtp_construct_message(data, data_size);
    if (send(client->sock->sock, message, CDTP_LENSIZE + data_size, 0) < 0)
        _cdtp_set_err(CDTP_CLIENT_SEND_FAILED);
    free(message);
}

void _cdtp_client_call_handle(CDTPClient *client)
{
    if (client->blocking == CDTP_TRUE)
        _cdtp_client_handle(client);
    else
        client->handle_thread = _cdtp_start_handle_thread(_cdtp_client_handle, client);
}

void _cdtp_client_handle(CDTPClient *client)
{
    char size_buffer[CDTP_LENSIZE];

    while (client->connected == CDTP_TRUE)
    {
#ifdef _WIN32
        int recv_code = recv(client->sock->sock, size_buffer, CDTP_LENSIZE, 0);

        // Check if the client has disconnected
        if (client->connected != CDTP_TRUE)
            return;

        if (recv_code == SOCKET_ERROR)
        {
            int err_code = WSAGetLastError();
            if (err_code == WSAECONNRESET)
            {
                cdtp_client_disconnect(client);
                _cdtp_client_call_on_disconnected(client);
                return;
            }
            else
            {
                _cdtp_set_error(CDTP_CLIENT_RECV_FAILED, err_code);
                return;
            }
        }
        else if (recv_code == 0)
        {
            cdtp_client_disconnect(client);
            _cdtp_client_call_on_disconnected(client);
            return;
        }
        else
        {
            size_t msg_size = _cdtp_ascii_to_dec(size_buffer);
            char *buffer = malloc(msg_size * sizeof(char));
            recv_code = recv(client->sock->sock, buffer, msg_size, 0);
            if (recv_code == SOCKET_ERROR)
            {
                int err_code = WSAGetLastError();
                if (err_code == WSAECONNRESET)
                {
                    cdtp_client_disconnect(client);
                    _cdtp_client_call_on_disconnected(client);
                    return;
                }
                else
                {
                    _cdtp_set_error(CDTP_CLIENT_RECV_FAILED, err_code);
                    return;
                }
            }
            else if (recv_code == 0)
            {
                cdtp_client_disconnect(client);
                _cdtp_client_call_on_disconnected(client);
                return;
            }
            else
                _cdtp_client_call_on_recv(client, (void *)buffer, msg_size);
        }
#else
        int recv_code = read(client->sock->sock, size_buffer, CDTP_LENSIZE);
        if (recv_code == 0)
        {
            cdtp_client_disconnect(client);
            _cdtp_client_call_on_disconnected(client);
            return;
        }
        else
        {
            size_t msg_size = _cdtp_ascii_to_dec(size_buffer);
            char *buffer = malloc(msg_size * sizeof(char));
            recv_code = read(client->sock->sock, buffer, msg_size);
            if (recv_code == 0)
            {
                cdtp_client_disconnect(client);
                _cdtp_client_call_on_disconnected(client);
                return;
            }
            else
                _cdtp_client_call_on_recv(client, (void *)buffer, msg_size);
        }
#endif
    }
}

void _cdtp_client_call_on_recv(CDTPClient *client, void *data, size_t data_size)
{
    if (client->event_blocking == CDTP_TRUE)
        (*(client->on_recv))(data, data_size, client->on_recv_arg);
    else
        _cdtp_start_thread_on_recv_client(client->on_recv, data, data_size, client->on_recv_arg);
}

void _cdtp_client_call_on_disconnected(CDTPClient *client)
{
    if (client->event_blocking == CDTP_TRUE)
        (*(client->on_disconnected))(client->on_disconnected_arg);
    else
        _cdtp_start_thread_on_disconnected(client->on_disconnected, client->on_disconnected_arg);
}
