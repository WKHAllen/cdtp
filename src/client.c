#include "client.h"

// Call the on_recv function
void _cdtp_client_call_on_recv(CDTPClient* client, void* data, size_t data_size)
{
    if (client->on_recv != NULL) {
        _cdtp_start_thread_on_recv_client(client->on_recv, data, data_size, client->on_recv_arg);
    }
}

// Call the on_connect function
void _cdtp_client_call_on_disconnected(CDTPClient* client)
{
    if (client->on_disconnected != NULL) {
        _cdtp_start_thread_on_disconnected(client->on_disconnected, client->on_disconnected_arg);
    }
}

// Exchange crypto keys with the server
void _cdtp_client_exchange_keys(CDTPClient *client)
{
    // TODO
    (void) client;
}

// Client handle function
void _cdtp_client_handle(CDTPClient* client)
{
#ifdef _WIN32
    // Set non-blocking
    unsigned long mode = 1;

    if (ioctlsocket(client->sock->sock, FIONBIO, &mode) != 0) {
        _cdtp_set_err(CDTP_CLIENT_SOCK_INIT_FAILED);
        return;
    }

    unsigned char size_buffer[CDTP_LENSIZE];
    int recv_code;

    while (client->connected == CDTP_TRUE) {
        recv_code = recv(client->sock->sock, (char *) size_buffer, CDTP_LENSIZE, 0);

        // Check if the client has disconnected
        if (client->connected != CDTP_TRUE) {
            return;
        }

        if (recv_code == SOCKET_ERROR) {
            int err_code = WSAGetLastError();

            if (err_code == WSAECONNRESET) {
                cdtp_client_disconnect(client);
                _cdtp_client_call_on_disconnected(client);
                return;
            }
            else if (err_code == WSAEWOULDBLOCK) {
                // Nothing happened on the socket, do nothing
            }
            else {
                _cdtp_set_error(CDTP_CLIENT_RECV_FAILED, err_code);
                return;
            }
        }
        else if (recv_code == 0) {
            cdtp_client_disconnect(client);
            _cdtp_client_call_on_disconnected(client);
            return;
        }
        else {
            size_t msg_size = _cdtp_decode_message_size(size_buffer);
            unsigned char *buffer = (unsigned char *) malloc(msg_size * sizeof(unsigned char));

            // Wait in case the message is sent in multiple chunks
            cdtp_sleep(CDTP_SLEEP_TIME);

            recv_code = recv(client->sock->sock, (char *) buffer, msg_size, 0);

            if (recv_code == SOCKET_ERROR) {
                int err_code = WSAGetLastError();

                if (err_code == WSAECONNRESET) {
                    cdtp_client_disconnect(client);
                    _cdtp_client_call_on_disconnected(client);
                    return;
                }
                else {
                    _cdtp_set_error(CDTP_CLIENT_RECV_FAILED, err_code);
                    return;
                }
            }
            else if (recv_code == 0) {
                cdtp_client_disconnect(client);
                _cdtp_client_call_on_disconnected(client);
                return;
            }
            else if (((size_t)recv_code) != msg_size) {
                _cdtp_set_err(CDTP_CLIENT_RECV_FAILED);
                return;
            }
            else {
                _cdtp_client_call_on_recv(client, (void *) buffer, msg_size);
            }
        }

        cdtp_sleep(CDTP_SLEEP_TIME);
    }
#else
    // Set non-blocking
    if (fcntl(client->sock->sock, F_SETFL, fcntl(client->sock->sock, F_GETFL, 0) | O_NONBLOCK) == -1) {
        _cdtp_set_err(CDTP_CLIENT_SOCK_INIT_FAILED);
        return;
    }

    unsigned char size_buffer[CDTP_LENSIZE];
    int recv_code;

    while (client->connected == CDTP_TRUE) {
        recv_code = read(client->sock->sock, (char *) size_buffer, CDTP_LENSIZE);

        // Check if the client has disconnected
        if (client->connected != CDTP_TRUE) {
            return;
        }

        if (recv_code == 0) {
            cdtp_client_disconnect(client);
            _cdtp_client_call_on_disconnected(client);
            return;
        }
        else if (recv_code == -1) {
            int err_code = errno;

            if (CDTP_EAGAIN_OR_WOULDBLOCK(err_code)) {
                // Nothing happened on the socket, do nothing
            }
            else {
                _cdtp_set_error(CDTP_CLIENT_RECV_FAILED, err_code);
                return;
            }
        }
        else {
            size_t msg_size = _cdtp_decode_message_size(size_buffer);
            unsigned char *buffer = (unsigned char *) malloc(msg_size * sizeof(unsigned char));

            // Wait in case the message is sent in multiple chunks
            cdtp_sleep(CDTP_SLEEP_TIME);

            recv_code = read(client->sock->sock, (char *) buffer, msg_size);

            if (recv_code == -1) {
                _cdtp_set_err(CDTP_CLIENT_RECV_FAILED);
                return;
            }
            else if (recv_code == 0) {
                cdtp_client_disconnect(client);
                _cdtp_client_call_on_disconnected(client);
                return;
            }
            else if (((size_t) recv_code) != msg_size) {
                _cdtp_set_err(CDTP_CLIENT_RECV_FAILED);
                return;
            }
            else {
                _cdtp_client_call_on_recv(client, (void *) buffer, msg_size);
            }
        }

        cdtp_sleep(CDTP_SLEEP_TIME);
    }
#endif
}

// Call the handle function
void _cdtp_client_call_handle(CDTPClient* client)
{
    client->handle_thread = _cdtp_start_handle_thread(_cdtp_client_handle, client);
}

CDTP_EXPORT CDTPClient* cdtp_client(
    ClientOnRecvCallback on_recv,
    ClientOnDisconnectedCallback on_disconnected,
    void* on_recv_arg,
    void* on_disconnected_arg
)
{
    CDTPClient* client = (CDTPClient*) malloc(sizeof(CDTPClient));

    // Initialize the client object
    client->on_recv = on_recv;
    client->on_disconnected = on_disconnected;
    client->on_recv_arg = on_recv_arg;
    client->on_disconnected_arg = on_disconnected_arg;
    client->connected = CDTP_FALSE;
    client->done = CDTP_FALSE;

    // Initialize the library
    if (CDTP_INIT != CDTP_TRUE) {
        int return_code = _cdtp_init();

        if (return_code != 0) {
            _cdtp_set_error(CDTP_WINSOCK_INIT_FAILED, return_code);
            return NULL;
        }
    }

    // Initialize the client socket
    client->sock = (CDTPSocket*) malloc(sizeof(CDTPSocket));

    // Initialize the socket info
#ifdef _WIN32
    // Initialize the socket
    if ((client->sock->sock = socket(CDTP_ADDRESS_FAMILY, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        _cdtp_set_err(CDTP_CLIENT_SOCK_INIT_FAILED);
        return NULL;
    }
#else
    // Initialize the socket
    if ((client->sock->sock = socket(CDTP_ADDRESS_FAMILY, SOCK_STREAM, 0)) == 0) {
        _cdtp_set_err(CDTP_CLIENT_SOCK_INIT_FAILED);
        return NULL;
    }
#endif

    return client;
}

CDTP_EXPORT void cdtp_client_connect(CDTPClient* client, char* host, unsigned short port)
{
    // Change 'localhost' to '127.0.0.1'
    if (strcmp(host, "localhost") == 0) {
        host = "127.0.0.1";
    }

    // Make sure the client has not connected before
    if (client->done == CDTP_TRUE) {
        _cdtp_set_error(CDTP_CLIENT_CANNOT_RECONNECT, 0);
        return;
    }

    // Make sure the client is not already connected
    if (client->connected == CDTP_TRUE) {
        _cdtp_set_error(CDTP_CLIENT_ALREADY_CONNECTED, 0);
        return;
    }

    // Set the client address
#ifdef _WIN32
    int addrlen = CDTP_ADDRSTRLEN;

    wchar_t *host_wc = _str_to_wchar(host);

    if (WSAStringToAddressW(host_wc, CDTP_ADDRESS_FAMILY, NULL, (LPSOCKADDR) & (client->sock->address), &addrlen) != 0) {
        _cdtp_set_err(CDTP_CLIENT_ADDRESS_FAILED);
        return;
    }

    free(host_wc);
#else
    if (inet_pton(CDTP_ADDRESS_FAMILY, host, &(client->sock->address)) != 1) {
        _cdtp_set_err(CDTP_CLIENT_ADDRESS_FAILED);
        return;
    }
#endif

    client->sock->address.sin_family = CDTP_ADDRESS_FAMILY;
    client->sock->address.sin_port = htons(port);

    if (connect(client->sock->sock, (struct sockaddr*)&(client->sock->address), sizeof(client->sock->address)) < 0) {
        _cdtp_set_err(CDTP_CLIENT_CONNECT_FAILED);
        return;
    }

    // Check the return code
    unsigned char size_buffer[CDTP_LENSIZE];
#ifdef _WIN32
    int recv_code = recv(client->sock->sock, (char*)size_buffer, CDTP_LENSIZE, 0);

    if (recv_code == SOCKET_ERROR) {
        int err_code = WSAGetLastError();

        if (err_code == WSAECONNRESET) {
            cdtp_client_disconnect(client);
            _cdtp_client_call_on_disconnected(client);
            return;
        }
        else {
            _cdtp_set_error(CDTP_CLIENT_RECV_FAILED, err_code);
            return;
        }
    }
    else if (recv_code == 0) {
        cdtp_client_disconnect(client);
        _cdtp_client_call_on_disconnected(client);
        return;
    }
    else {
        size_t msg_size = _cdtp_decode_message_size(size_buffer);
        char* buffer = (char*) malloc(msg_size * sizeof(char));
        recv_code = recv(client->sock->sock, buffer, msg_size, 0);

        if (recv_code == SOCKET_ERROR) {
            int err_code = WSAGetLastError();

            if (err_code == WSAECONNRESET) {
                cdtp_client_disconnect(client);
                _cdtp_client_call_on_disconnected(client);
                return;
            }
            else {
                _cdtp_set_error(CDTP_CLIENT_RECV_FAILED, err_code);
                return;
            }
        }
        else if (recv_code == 0) {
            cdtp_client_disconnect(client);
            _cdtp_client_call_on_disconnected(client);
            return;
        }
        else {
            int connect_code = *(int*)buffer;
            if (connect_code == CDTP_SERVER_FULL) {
                _cdtp_set_error(CDTP_SERVER_FULL, 0);
                cdtp_client_disconnect(client);
                _cdtp_client_call_on_disconnected(client);
                return;
            }
        }
    }
#else
    int recv_code = read(client->sock->sock, size_buffer, CDTP_LENSIZE);

    if (recv_code == 0) {
        cdtp_client_disconnect(client);
        _cdtp_client_call_on_disconnected(client);
        return;
    }
    else {
        size_t msg_size = _cdtp_decode_message_size(size_buffer);
        char* buffer = (char*) malloc(msg_size * sizeof(char));
        recv_code = read(client->sock->sock, buffer, msg_size);

        if (recv_code == 0) {
            cdtp_client_disconnect(client);
            _cdtp_client_call_on_disconnected(client);
            return;
        }
        else {
            int connect_code = *(int*)buffer;

            if (connect_code == CDTP_SERVER_FULL) {
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

    // Set blocking for key exchange
#ifdef _WIN32
    unsigned long mode = 0;

    if (ioctlsocket(client->sock->sock, FIONBIO, &mode) != 0) {
        _cdtp_set_err(CDTP_CLIENT_SOCK_INIT_FAILED);
        return;
    }
#else
    if (fcntl(client->sock->sock, F_SETFL, fcntl(client->sock->sock, F_GETFL, 0) & ~O_NONBLOCK) == -1) {
        _cdtp_set_err(CDTP_CLIENT_SOCK_INIT_FAILED);
        return;
    }
#endif

    _cdtp_client_exchange_keys(client);

    _cdtp_client_call_handle(client);
}

CDTP_EXPORT void cdtp_client_disconnect(CDTPClient* client)
{
    // Make sure the client is connected
    if (client->connected != CDTP_TRUE) {
        _cdtp_set_error(CDTP_CLIENT_NOT_CONNECTED, 0);
        return;
    }

    client->connected = CDTP_FALSE;
    client->done = CDTP_TRUE;

#ifdef _WIN32
    // Close the socket
    if (closesocket(client->sock->sock) != 0) {
        _cdtp_set_err(CDTP_CLIENT_DISCONNECT_FAILED);
        return;
    }

    // Wait for threads to exit
    if (GetThreadId(client->handle_thread) == GetCurrentThreadId()) {
        if (WaitForSingleObject(client->handle_thread, INFINITE) == WAIT_FAILED) {
            _cdtp_set_error(CDTP_HANDLE_THREAD_NOT_CLOSING, GetLastError());
            return;
        }
    } else {
        if (CloseHandle(client->handle_thread) == 0) {
            _cdtp_set_error(CDTP_HANDLE_THREAD_NOT_CLOSING, GetLastError());
            return;
        }
    }
#else
    // Close the socket
    if (close(client->sock->sock) != 0) {
        _cdtp_set_err(CDTP_CLIENT_DISCONNECT_FAILED);
        return;
    }

    // Wait for threads to exit
    if (pthread_equal(client->handle_thread, pthread_self()) == 0) {
        int err_code = pthread_join(client->handle_thread, NULL);

        if (err_code != 0) {
            _cdtp_set_error(CDTP_HANDLE_THREAD_NOT_CLOSING, err_code);
            return;
        }
    } else {
        int err_code = pthread_detach(client->handle_thread);

        if (err_code != 0) {
            _cdtp_set_error(CDTP_HANDLE_THREAD_NOT_CLOSING, err_code);
            return;
        }
    }
#endif
}

CDTP_EXPORT int cdtp_client_is_connected(CDTPClient* client)
{
    return client->connected;
}

CDTP_EXPORT char* cdtp_client_get_host(CDTPClient* client)
{
    // Make sure the client is connected
    if (client->connected != CDTP_TRUE) {
        _cdtp_set_error(CDTP_CLIENT_NOT_CONNECTED, 0);
        return NULL;
    }

    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);

    if (getsockname(client->sock->sock, (struct sockaddr *) (&addr), &len) != 0) {
        _cdtp_set_err(CDTP_CLIENT_ADDRESS_FAILED);
        return NULL;
    }

    struct sockaddr_in *s = (struct sockaddr_in *) (&addr);

#ifdef _WIN32
    int addrlen = CDTP_ADDRSTRLEN;

    wchar_t addr_wc[CDTP_ADDRSTRLEN];

    if (WSAAddressToStringW((LPSOCKADDR) s, sizeof(*s), NULL, addr_wc, (LPDWORD) (&addrlen)) != 0) {
        _cdtp_set_err(CDTP_CLIENT_ADDRESS_FAILED);
        return NULL;
    }

    // Remove the port
    for (int i = 0; i < CDTP_ADDRSTRLEN && addr_wc[i] != '\0'; i++) {
        if (addr_wc[i] == ':') {
            addr_wc[i] = '\0';
            break;
        }
    }

    char *addr_str = _wchar_to_str(addr_wc);
#else
    char *addr_str = (char *) malloc(CDTP_ADDRSTRLEN * sizeof(char));

    if (inet_ntop(CDTP_ADDRESS_FAMILY, &s->sin_addr, addr_str, CDTP_ADDRSTRLEN) == NULL) {
        _cdtp_set_err(CDTP_CLIENT_ADDRESS_FAILED);
        return NULL;
    }
#endif

    return addr_str;
}

CDTP_EXPORT unsigned short cdtp_client_get_port(CDTPClient* client)
{
    // Make sure the server is running
    if (client->connected != CDTP_TRUE) {
        _cdtp_set_error(CDTP_CLIENT_NOT_CONNECTED, 0);
        return 0;
    }

    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);

    if (getsockname(client->sock->sock, (struct sockaddr *) (&addr), &len) != 0) {
        _cdtp_set_err(CDTP_CLIENT_ADDRESS_FAILED);
        return 0;
    }

    struct sockaddr_in *s = (struct sockaddr_in *) (&addr);
    unsigned short port = ntohs(s->sin_port);

    return port;
}

CDTP_EXPORT char* cdtp_client_get_server_host(CDTPClient* client)
{
    // Make sure the client is connected
    if (client->connected != CDTP_TRUE) {
        _cdtp_set_error(CDTP_CLIENT_NOT_CONNECTED, 0);
        return NULL;
    }

    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);

    if (getpeername(client->sock->sock, (struct sockaddr *) (&addr), &len) != 0) {
        _cdtp_set_err(CDTP_SERVER_ADDRESS_FAILED);
        return NULL;
    }

    struct sockaddr_in *s = (struct sockaddr_in *) (&addr);

#ifdef _WIN32
    int addrlen = CDTP_ADDRSTRLEN;

    wchar_t addr_wc[CDTP_ADDRSTRLEN];

    if (WSAAddressToStringW((LPSOCKADDR) s, sizeof(*s), NULL, addr_wc, (LPDWORD) (&addrlen)) != 0) {
        _cdtp_set_err(CDTP_SERVER_ADDRESS_FAILED);
        return NULL;
    }

    // Remove the port
    for (int i = 0; i < CDTP_ADDRSTRLEN && addr_wc[i] != '\0'; i++) {
        if (addr_wc[i] == ':') {
            addr_wc[i] = '\0';
            break;
        }
    }

    char *addr_str = _wchar_to_str(addr_wc);
#else
    char *addr_str = (char *) malloc(CDTP_ADDRSTRLEN * sizeof(char));

    if (inet_ntop(CDTP_ADDRESS_FAMILY, &s->sin_addr, addr_str, CDTP_ADDRSTRLEN) == NULL) {
        _cdtp_set_err(CDTP_SERVER_ADDRESS_FAILED);
        return NULL;
    }
#endif

    return addr_str;
}

CDTP_EXPORT unsigned short cdtp_client_get_server_port(CDTPClient* client)
{
    // Make sure the server is running
    if (client->connected != CDTP_TRUE) {
        _cdtp_set_error(CDTP_CLIENT_NOT_CONNECTED, 0);
        return 0;
    }

    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);

    if (getpeername(client->sock->sock, (struct sockaddr *) (&addr), &len) != 0) {
        _cdtp_set_err(CDTP_SERVER_ADDRESS_FAILED);
        return 0;
    }

    struct sockaddr_in *s = (struct sockaddr_in *) (&addr);
    unsigned short port = ntohs(s->sin_port);

    return port;
}

CDTP_EXPORT void cdtp_client_send(CDTPClient* client, void* data, size_t data_size)
{
    // Make sure the client is connected
    if (client->connected != CDTP_TRUE) {
        _cdtp_set_error(CDTP_CLIENT_NOT_CONNECTED, 0);
        return;
    }

    char* message = _cdtp_construct_message(data, data_size);

    if (send(client->sock->sock, message, CDTP_LENSIZE + data_size, 0) < 0) {
        _cdtp_set_err(CDTP_CLIENT_SEND_FAILED);
    }

    free(message);
}

CDTP_EXPORT void cdtp_client_free(CDTPClient *client)
{
    free(client->sock);
    free(client);
}
