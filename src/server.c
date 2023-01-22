#include "server.h"

/**
 * Get a new client ID.
 *
 * @param server The socket server.
 * @return The next available client ID.
 */
size_t _cdtp_server_new_client_id(CDTPServer *server)
{
    return server->next_client_id++;
}

/**
 * Disconnect a client from the server.
 *
 * @param server The socket server.
 * @param client_id The ID of the client to disconnect.
 */
void _cdtp_server_disconnect_sock(CDTPServer *server, size_t client_id)
{
    CDTPSocket *client = _cdtp_client_map_pop(server->clients, client_id);

    if (client != NULL) {
#ifdef _WIN32
        closesocket(client->sock);
#else
        close(client->sock);
#endif

        _cdtp_crypto_aes_key_free(client->key);
        free(client);
    }
}

/**
 * Call the `on_recv` event function.
 *
 * @param server The socket server.
 * @param client_id The ID of the client who sent the data.
 * @param data The received data.
 * @param data_size The size of the received data, in bytes.
 */
void _cdtp_server_call_on_recv(CDTPServer *server, size_t client_id, void *data, size_t data_size)
{
    if (server->on_recv != NULL) {
        CDTPSocket *client = _cdtp_client_map_get(server->clients, client_id);
        CDTPCryptoData *data_decrypted = _cdtp_crypto_aes_decrypt(client->key, data, data_size);
        size_t decrypted_data_size = data_decrypted->data_size;
        void *decrypted_data = _cdtp_crypto_data_unwrap(data_decrypted);

        _cdtp_start_thread_on_recv_server(server->on_recv,
                                          server,
                                          client_id,
                                          decrypted_data,
                                          decrypted_data_size,
                                          server->on_recv_arg);
    }

    free(data);
}

/**
 * Call the `on_connect` event function.
 *
 * @param server The socket server.
 * @param client_id The ID of the connecting client.
 */
void _cdtp_server_call_on_connect(CDTPServer *server, size_t client_id)
{
    if (server->on_connect != NULL) {
        _cdtp_start_thread_on_connect(server->on_connect,
                                      server,
                                      client_id,
                                      server->on_connect_arg);
    }
}

/**
 * Call the `on_disconnect` event function.
 *
 * @param server The socket server.
 * @param client_id The ID of the disconnecting client.
 */
void _cdtp_server_call_on_disconnect(CDTPServer *server, size_t client_id)
{
    if (server->on_disconnect != NULL) {
        _cdtp_start_thread_on_disconnect(server->on_disconnect,
                                         server,
                                         client_id,
                                         server->on_disconnect_arg);
    }
}

/**
 * Exchange crypto keys with a client.
 *
 * @param client The client socket.
 * @return If the exchange succeeded.
 */
bool _cdtp_server_exchange_keys(CDTPSocket *client)
{
    CDTPRSAKeyPair *rsa_keys = _cdtp_crypto_rsa_key_pair();
    CDTPRSAPublicKey *public_key = rsa_keys->public_key;
    CDTPRSAPrivateKey *private_key = rsa_keys->private_key;
    CDTPCryptoData *public_key_data = _cdtp_crypto_rsa_public_key_to_bytes(public_key);
    char *public_key_encoded = _cdtp_construct_message(public_key_data->data, public_key_data->data_size);

    if (send(client->sock, public_key_encoded, CDTP_LENSIZE + public_key_data->data_size, 0) < 0) {
        _cdtp_set_err(CDTP_SERVER_KEY_EXCHANGE_FAILED);
        return false;
    }

    char size_buffer[CDTP_LENSIZE];
    size_t msg_size;
    char *buffer;
    int recv_code;

#ifdef _WIN32
    recv_code = recv(client->sock, size_buffer, CDTP_LENSIZE, 0);

    if (recv_code == SOCKET_ERROR || recv_code == 0) {
        _cdtp_set_err(CDTP_SERVER_KEY_EXCHANGE_FAILED);
        return false;
    }
    else {
        msg_size = _cdtp_decode_message_size((unsigned char *) size_buffer);
        buffer = (char *) malloc(msg_size * sizeof(char));

        recv_code = recv(client->sock, buffer, msg_size, 0);

        if (recv_code == SOCKET_ERROR || recv_code == 0 || (size_t) recv_code != msg_size) {
            _cdtp_set_err(CDTP_SERVER_KEY_EXCHANGE_FAILED);
            return false;
        }
    }
#else
    recv_code = read(client->sock, size_buffer, CDTP_LENSIZE);

    if (recv_code == 0 || recv_code == -1) {
        _cdtp_set_err(CDTP_SERVER_KEY_EXCHANGE_FAILED);
        return false;
    }
    else {
        msg_size = _cdtp_decode_message_size((unsigned char *) size_buffer);
        buffer = (char *) malloc(msg_size * sizeof(char));

        recv_code = read(client->sock, buffer, msg_size);

        if (recv_code == 0 || recv_code == -1 || (size_t) recv_code != msg_size) {
            _cdtp_set_err(CDTP_SERVER_KEY_EXCHANGE_FAILED);
            return false;
        }
    }
#endif

    CDTPCryptoData *key_data = _cdtp_crypto_rsa_decrypt(private_key, buffer, msg_size);
    client->key = _cdtp_crypto_aes_key_from(key_data->data, key_data->data_size);

    _cdtp_crypto_rsa_key_pair_free(rsa_keys);
    _cdtp_crypto_data_free(public_key_data);
    free(public_key_encoded);
    free(buffer);
    _cdtp_crypto_data_free(key_data);

    return true;
}

/**
 * Serve clients.
 *
 * @param server The socket server.
 */
void _cdtp_server_serve(CDTPServer *server)
{
    struct sockaddr_in address;
    int addrlen = sizeof(address);

#ifdef _WIN32
    // Set non-blocking
    unsigned long mode = 1;

    if (ioctlsocket(server->sock->sock, FIONBIO, &mode) != 0) {
        _cdtp_set_err(CDTP_SERVER_SOCK_INIT_FAILED);
        return;
    }

    SOCKET new_sock;
#else
    // Set non-blocking
    if (fcntl(server->sock->sock, F_SETFL, fcntl(server->sock->sock, F_GETFL, 0) | O_NONBLOCK) == -1) {
        _cdtp_set_err(CDTP_SERVER_SOCK_INIT_FAILED);
        return;
    }

    int new_sock;
#endif

    unsigned char size_buffer[CDTP_LENSIZE];
    int recv_code;

    while (server->serving) {
        // Accept incoming connections
#ifdef _WIN32
        new_sock = accept(server->sock->sock, (struct sockaddr *) (&address), (int *) (&addrlen));
#else
        new_sock = accept(server->sock->sock, (struct sockaddr *) (&address), (socklen_t *) (&addrlen));
#endif

        // Check if the server has been stopped
        if (!server->serving) {
            return;
        }

#ifdef _WIN32
        if (new_sock == INVALID_SOCKET) {
            int err_code = WSAGetLastError();

            if (err_code == WSAEWOULDBLOCK) {
                // No new connections, do nothing
            }
            else if (err_code != WSAENOTSOCK || server->serving) {
                _cdtp_set_error(CDTP_SOCKET_ACCEPT_FAILED, err_code);
                return;
            }
            else {
                return;
            }
        }
        else {
            size_t client_id = _cdtp_server_new_client_id(server);

            // Set blocking for key exchange
            mode = 0;

            if (ioctlsocket(new_sock, FIONBIO, &mode) != 0) {
                _cdtp_set_err(CDTP_SOCKET_ACCEPT_FAILED);
                return;
            }

            // Create the new client object
            CDTPSocket *new_client = (CDTPSocket *) malloc(sizeof(CDTPSocket));
            new_client->sock = new_sock;
            memcpy(&(new_client->address), &address, sizeof(address));

            // Exchange keys
            if (!_cdtp_server_exchange_keys(new_client)) {
                return;
            }

            // Set non-blocking
            mode = 1;

            if (ioctlsocket(new_sock, FIONBIO, &mode) != 0) {
                _cdtp_set_err(CDTP_SOCKET_ACCEPT_FAILED);
                return;
            }

            // Add the new socket to the client map
            _cdtp_client_map_set(server->clients, client_id, new_client);
            _cdtp_server_call_on_connect(server, client_id);
        }
#else
        if (new_sock < 0) {
            int err_code = errno;

            if (CDTP_EAGAIN_OR_WOULDBLOCK(err_code)) {
                // No new connections, do nothing
            }
            else if (err_code != ENOTSOCK || server->serving) {
                _cdtp_set_error(CDTP_SOCKET_ACCEPT_FAILED, err_code);
                return;
            }
            else {
                return;
            }
        }
        else {
            size_t client_id = _cdtp_server_new_client_id(server);

            // Set blocking for key exchange
            if (fcntl(new_sock, F_SETFL, fcntl(new_sock, F_GETFL, 0) & ~O_NONBLOCK) == -1) {
                _cdtp_set_err(CDTP_SOCKET_ACCEPT_FAILED);
                return;
            }

            // Create the new client object
            CDTPSocket *new_client = (CDTPSocket *) malloc(sizeof(CDTPSocket));
            new_client->sock = new_sock;
            memcpy(&(new_client->address), &address, sizeof(address));

            // Exchange keys
            if (!_cdtp_server_exchange_keys(new_client)) {
                return;
            }

            // Set non-blocking
            if (fcntl(new_sock, F_SETFL, fcntl(new_sock, F_GETFL, 0) | O_NONBLOCK) == -1) {
                _cdtp_set_err(CDTP_SOCKET_ACCEPT_FAILED);
                return;
            }

            // Add the new socket to the client map
            _cdtp_client_map_set(server->clients, client_id, new_client);
            _cdtp_server_call_on_connect(server, client_id);
        }
#endif

        // Check for messages from client sockets
        CDTPClientMapIter *iter = _cdtp_client_map_iter(server->clients);

        for (size_t i = 0; i < iter->size; i++) {
            size_t client_id = iter->clients[i]->client_id;
            CDTPSocket *client_sock = iter->clients[i]->sock;

#ifdef _WIN32
            recv_code = recv(client_sock->sock, (char *) size_buffer, CDTP_LENSIZE, 0);

            if (recv_code == SOCKET_ERROR) {
                int err_code = WSAGetLastError();

                if (err_code == WSAECONNRESET || err_code == WSAENOTSOCK) {
                    if (_cdtp_client_map_contains(server->clients, client_id)) {
                        _cdtp_server_disconnect_sock(server, client_id);
                        _cdtp_server_call_on_disconnect(server, client_id);
                    }
                }
                else if (err_code == WSAEWOULDBLOCK) {
                    // Nothing happened on the socket, do nothing
                }
                else {
                    _cdtp_set_error(CDTP_SERVER_RECV_FAILED, err_code);
                    return;
                }
            }
            else if (recv_code == 0) {
                if (_cdtp_client_map_contains(server->clients, client_id)) {
                    _cdtp_server_disconnect_sock(server, client_id);
                    _cdtp_server_call_on_disconnect(server, client_id);
                }
            }
            else {
                size_t msg_size = _cdtp_decode_message_size(size_buffer);
                unsigned char *buffer = (unsigned char *) malloc(msg_size * sizeof(unsigned char));

                // Wait in case the message is sent in multiple chunks
                cdtp_sleep(CDTP_SLEEP_TIME);

                recv_code = recv(client_sock->sock, (char *) buffer, msg_size, 0);

                if (recv_code == SOCKET_ERROR) {
                    int err_code = WSAGetLastError();

                    if (err_code == WSAECONNRESET || err_code == WSAENOTSOCK) {
                        if (_cdtp_client_map_contains(server->clients, client_id)) {
                            _cdtp_server_disconnect_sock(server, client_id);
                            _cdtp_server_call_on_disconnect(server, client_id);
                        }
                    }
                    else {
                        _cdtp_set_error(CDTP_SERVER_RECV_FAILED, err_code);
                        return;
                    }
                }
                else if (recv_code == 0) {
                    if (_cdtp_client_map_contains(server->clients, client_id)) {
                        _cdtp_server_disconnect_sock(server, client_id);
                        _cdtp_server_call_on_disconnect(server, client_id);
                    }
                }
                else if (((size_t) recv_code) != msg_size) {
                    _cdtp_set_err(CDTP_SERVER_RECV_FAILED);
                    return;
                }
                else {
                    _cdtp_server_call_on_recv(server, client_id, (void *) buffer, msg_size);
                }
            }
#else
            recv_code = read(client_sock->sock, (char *) size_buffer, CDTP_LENSIZE);

            if (recv_code == 0) {
                if (_cdtp_client_map_contains(server->clients, client_id)) {
                    _cdtp_server_disconnect_sock(server, client_id);
                    _cdtp_server_call_on_disconnect(server, client_id);
                }
            }
            else if (recv_code == -1) {
                int err_code = errno;

                if (err_code == EBADF) {
                    if (_cdtp_client_map_contains(server->clients, client_id)) {
                        _cdtp_server_disconnect_sock(server, client_id);
                        _cdtp_server_call_on_disconnect(server, client_id);
                    }
                }
                else if (CDTP_EAGAIN_OR_WOULDBLOCK(err_code)) {
                    // Nothing happened on the socket, do nothing
                }
                else {
                    _cdtp_set_error(CDTP_SERVER_RECV_FAILED, err_code);
                    return;
                }
            }
            else {
                size_t msg_size = _cdtp_decode_message_size(size_buffer);
                unsigned char *buffer = (unsigned char *) malloc(msg_size * sizeof(unsigned char));

                // Wait in case the message is sent in multiple chunks
                cdtp_sleep(CDTP_SLEEP_TIME);

                recv_code = read(client_sock->sock, (char *) buffer, msg_size);

                if (recv_code == -1) {
                    int err_code = errno;

                    if (err_code == EBADF) {
                        if (_cdtp_client_map_contains(server->clients, client_id)) {
                            _cdtp_server_disconnect_sock(server, client_id);
                            _cdtp_server_call_on_disconnect(server, client_id);
                        }
                    } else {
                        _cdtp_set_error(CDTP_SERVER_RECV_FAILED, err_code);
                        return;
                    }
                }
                else if (recv_code == 0) {
                    if (_cdtp_client_map_contains(server->clients, client_id)) {
                        _cdtp_server_disconnect_sock(server, client_id);
                        _cdtp_server_call_on_disconnect(server, client_id);
                    }
                }
                else if (((size_t) recv_code) != msg_size) {
                    _cdtp_set_err(CDTP_SERVER_RECV_FAILED);
                    return;
                }
                else {
                    _cdtp_server_call_on_recv(server, client_id, (void *) buffer, msg_size);
                }
            }
#endif
        }

        _cdtp_client_map_iter_free(iter);

        cdtp_sleep(CDTP_SLEEP_TIME);
    }
}

/**
 * Call the serve function.
 *
 * @param server The socket server.
 */
void _cdtp_server_call_serve(CDTPServer *server)
{
    server->serve_thread = _cdtp_start_serve_thread(_cdtp_server_serve, server);
}

CDTP_EXPORT CDTPServer *cdtp_server(
    ServerOnRecvCallback on_recv,
    ServerOnConnectCallback on_connect,
    ServerOnDisconnectCallback on_disconnect,
    void *on_recv_arg,
    void *on_connect_arg,
    void *on_disconnect_arg
)
{
    CDTPServer *server = (CDTPServer *) malloc(sizeof(CDTPServer));

    // Initialize the server object
    server->on_recv = on_recv;
    server->on_connect = on_connect;
    server->on_disconnect = on_disconnect;
    server->on_recv_arg = on_recv_arg;
    server->on_connect_arg = on_connect_arg;
    server->on_disconnect_arg = on_disconnect_arg;
    server->serving = false;
    server->done = false;
    server->clients = _cdtp_client_map();
    server->next_client_id = 0;

    // Initialize the library
    if (!CDTP_INIT) {
        int return_code = _cdtp_init();

        if (return_code != 0) {
            _cdtp_set_error(CDTP_WINSOCK_INIT_FAILED, return_code);
            return NULL;
        }
    }

    // Initialize the server socket
    server->sock = (CDTPSocket *) malloc(sizeof(CDTPSocket));

    // Initialize the socket info
    int opt = 1;

#ifdef _WIN32
    // Initialize the socket
    if ((server->sock->sock = socket(CDTP_ADDRESS_FAMILY, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        _cdtp_set_err(CDTP_SERVER_SOCK_INIT_FAILED);
        return NULL;
    }
    if (setsockopt(server->sock->sock, SOL_SOCKET, SO_REUSEADDR, (char *) (&opt), sizeof(opt)) == SOCKET_ERROR) {
        _cdtp_set_err(CDTP_SERVER_SETSOCKOPT_FAILED);
        return NULL;
    }
#else
    // Initialize the socket
    if ((server->sock->sock = socket(CDTP_ADDRESS_FAMILY, SOCK_STREAM, 0)) == 0) {
        _cdtp_set_err(CDTP_SERVER_SOCK_INIT_FAILED);
        return NULL;
    }
    if (setsockopt(server->sock->sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        _cdtp_set_err(CDTP_SERVER_SETSOCKOPT_FAILED);
        return NULL;
    }
#endif

    server->sock->key = NULL;

    return server;
}

CDTP_EXPORT void cdtp_server_start(CDTPServer *server, char *host, unsigned short port)
{
    // Make sure the server has not been run before
    if (server->done) {
        _cdtp_set_error(CDTP_SERVER_CANNOT_RESTART, 0);
        return;
    }

    // Make sure the server is not already serving
    if (server->serving) {
        _cdtp_set_error(CDTP_SERVER_ALREADY_SERVING, 0);
        return;
    }

    // Change 'localhost' to '127.0.0.1'
    if (strcmp(host, "localhost") == 0) {
        host = "127.0.0.1";
    }

    // Set the server address
#ifdef _WIN32
    int addrlen = CDTP_ADDRSTRLEN;

    wchar_t *host_wc = _str_to_wchar(host);

    if (WSAStringToAddressW(host_wc, CDTP_ADDRESS_FAMILY, NULL, (LPSOCKADDR) (&(server->sock->address)), &addrlen) != 0) {
        _cdtp_set_err(CDTP_SERVER_ADDRESS_FAILED);
        return;
    }

    free(host_wc);
#else
    if (inet_pton(CDTP_ADDRESS_FAMILY, host, &(server->sock->address)) != 1) {
        _cdtp_set_err(CDTP_SERVER_ADDRESS_FAILED);
        return;
    }
#endif

    server->sock->address.sin_family = CDTP_ADDRESS_FAMILY;
    server->sock->address.sin_port = htons(port);

    // Bind the address to the server
    if (bind(server->sock->sock, (struct sockaddr *) (&(server->sock->address)), sizeof(server->sock->address)) < 0) {
        _cdtp_set_err(CDTP_SERVER_BIND_FAILED);
        return;
    }

    // Listen for connections
    if (listen(server->sock->sock, CDTP_SERVER_LISTEN_BACKLOG) < 0) {
        _cdtp_set_err(CDTP_SERVER_LISTEN_FAILED);
        return;
    }

    // Serve
    server->serving = true;
    _cdtp_server_call_serve(server);
}

CDTP_EXPORT void cdtp_server_stop(CDTPServer *server)
{
    // Make sure the server is running
    if (!server->serving) {
        _cdtp_set_error(CDTP_SERVER_NOT_SERVING, 0);
        return;
    }

    server->serving = false;
    server->done = true;

#ifdef _WIN32
    // Close sockets
    CDTPClientMapIter *iter = _cdtp_client_map_iter(server->clients);

    for (size_t i = 0; i < iter->size; i++) {
        if (closesocket(iter->clients[i]->sock->sock) != 0) {
            _cdtp_set_err(CDTP_SERVER_STOP_FAILED);
            return;
        }

        CDTPSocket *client = _cdtp_client_map_pop(server->clients, iter->clients[i]->client_id);
        _cdtp_crypto_aes_key_free(client->key);
        free(client);
    }

    _cdtp_client_map_iter_free(iter);

    if (closesocket(server->sock->sock) != 0) {
        _cdtp_set_err(CDTP_SERVER_STOP_FAILED);
        return;
    }

    // Wait for threads to exit
    if (GetThreadId(server->serve_thread) == GetCurrentThreadId()) {
        if (WaitForSingleObject(server->serve_thread, INFINITE) == WAIT_FAILED) {
            _cdtp_set_error(CDTP_SERVE_THREAD_NOT_CLOSING, GetLastError());
            return;
        }
    } else {
        if (CloseHandle(server->serve_thread) == 0) {
            _cdtp_set_error(CDTP_SERVE_THREAD_NOT_CLOSING, GetLastError());
            return;
        }
    }
#else
    // Close sockets
    CDTPClientMapIter *iter = _cdtp_client_map_iter(server->clients);

    for (size_t i = 0; i < iter->size; i++) {
        if (close(iter->clients[i]->sock->sock) != 0) {
            _cdtp_set_err(CDTP_SERVER_STOP_FAILED);
            return;
        }

        CDTPSocket *client = _cdtp_client_map_pop(server->clients, iter->clients[i]->client_id);
        _cdtp_crypto_aes_key_free(client->key);
        free(client);
    }

    _cdtp_client_map_iter_free(iter);

    if (close(server->sock->sock) != 0) {
        _cdtp_set_err(CDTP_SERVER_STOP_FAILED);
        return;
    }

    // Wait for threads to exit
    if (pthread_equal(server->serve_thread, pthread_self()) == 0) {
        int err_code = pthread_join(server->serve_thread, NULL);

        if (err_code != 0) {
            _cdtp_set_error(CDTP_SERVE_THREAD_NOT_CLOSING, err_code);
            return;
        }
    } else {
        int err_code = pthread_detach(server->serve_thread);

        if (err_code != 0) {
            _cdtp_set_error(CDTP_SERVE_THREAD_NOT_CLOSING, err_code);
            return;
        }
    }
#endif
}

CDTP_EXPORT bool cdtp_server_is_serving(CDTPServer *server)
{
    return server->serving;
}

CDTP_EXPORT char *cdtp_server_get_host(CDTPServer *server)
{
    // Make sure the server is running
    if (!server->serving) {
        _cdtp_set_error(CDTP_SERVER_NOT_SERVING, 0);
        return NULL;
    }

    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);

    if (getsockname(server->sock->sock, (struct sockaddr *) (&addr), &len) != 0) {
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

CDTP_EXPORT unsigned short cdtp_server_get_port(CDTPServer *server)
{
    // Make sure the server is running
    if (!server->serving) {
        _cdtp_set_error(CDTP_SERVER_NOT_SERVING, 0);
        return 0;
    }

    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);

    if (getsockname(server->sock->sock, (struct sockaddr *) (&addr), &len) != 0) {
        _cdtp_set_err(CDTP_SERVER_ADDRESS_FAILED);
        return 0;
    }

    struct sockaddr_in *s = (struct sockaddr_in *) (&addr);
    unsigned short port = ntohs(s->sin_port);

    return port;
}

CDTP_EXPORT char *cdtp_server_get_client_host(CDTPServer *server, size_t client_id)
{
    // Make sure the server is running
    if (!server->serving) {
        _cdtp_set_error(CDTP_SERVER_NOT_SERVING, 0);
        return NULL;
    }

    CDTPSocket *client = _cdtp_client_map_get(server->clients, client_id);

    // Make sure the client exists
    if (client == NULL) {
        _cdtp_set_error(CDTP_CLIENT_DOES_NOT_EXIST, 0);
        return NULL;
    }

    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);

    if (getpeername(client->sock, (struct sockaddr *) (&addr), &len) != 0) {
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

CDTP_EXPORT unsigned short cdtp_server_get_client_port(CDTPServer *server, size_t client_id)
{
    // Make sure the server is running
    if (!server->serving) {
        _cdtp_set_error(CDTP_SERVER_NOT_SERVING, 0);
        return 0;
    }

    CDTPSocket *client = _cdtp_client_map_get(server->clients, client_id);

    // Make sure the client exists
    if (client == NULL) {
        _cdtp_set_error(CDTP_CLIENT_DOES_NOT_EXIST, 0);
        return 0;
    }

    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);

    if (getpeername(client->sock, (struct sockaddr *) (&addr), &len) != 0) {
        _cdtp_set_err(CDTP_CLIENT_ADDRESS_FAILED);
        return 0;
    }

    struct sockaddr_in *s = (struct sockaddr_in *) (&addr);
    unsigned short port = ntohs(s->sin_port);

    return port;
}

CDTP_EXPORT void cdtp_server_remove_client(CDTPServer *server, size_t client_id)
{
    // Make sure the server is running
    if (!server->serving) {
        _cdtp_set_error(CDTP_SERVER_NOT_SERVING, 0);
        return;
    }

    CDTPSocket *client = _cdtp_client_map_pop(server->clients, client_id);

    // Make sure the client exists
    if (client == NULL) {
        _cdtp_set_error(CDTP_CLIENT_DOES_NOT_EXIST, 0);
        return;
    }

#ifdef _WIN32
    if (closesocket(client->sock) != 0) {
        _cdtp_set_err(CDTP_CLIENT_REMOVE_FAILED);
        return;
    }
#else
    if (close(client->sock) != 0) {
        _cdtp_set_err(CDTP_CLIENT_REMOVE_FAILED);
        return;
    }
#endif

    _cdtp_crypto_aes_key_free(client->key);
    free(client);
}

CDTP_EXPORT void cdtp_server_send(CDTPServer *server, size_t client_id, void *data, size_t data_size)
{
    // Make sure the server is running
    if (!server->serving) {
        _cdtp_set_error(CDTP_SERVER_NOT_SERVING, 0);
        return;
    }

    CDTPSocket *client = _cdtp_client_map_get(server->clients, client_id);

    // Make sure the client exists
    if (client == NULL) {
        _cdtp_set_error(CDTP_CLIENT_DOES_NOT_EXIST, 0);
        return;
    }

    CDTPCryptoData *data_encrypted = _cdtp_crypto_aes_encrypt(client->key, data, data_size);
    char *message = _cdtp_construct_message(data_encrypted->data, data_encrypted->data_size);

    if (send(client->sock, message, CDTP_LENSIZE + data_encrypted->data_size, 0) < 0) {
        _cdtp_set_err(CDTP_SERVER_SEND_FAILED);
        return;
    }

    _cdtp_crypto_data_free(data_encrypted);
    free(message);
}

CDTP_EXPORT void cdtp_server_send_all(CDTPServer *server, void *data, size_t data_size)
{
    // Make sure the server is running
    if (!server->serving) {
        _cdtp_set_error(CDTP_SERVER_NOT_SERVING, 0);
        return;
    }

    CDTPClientMapIter *iter = _cdtp_client_map_iter(server->clients);

    for (size_t i = 0; i < iter->size; i++) {
        cdtp_server_send(server, iter->clients[i]->client_id, data, data_size);
    }

    _cdtp_client_map_iter_free(iter);
}

CDTP_EXPORT void cdtp_server_free(CDTPServer *server)
{
    // Make sure the client is done
    if (!server->done) {
        _cdtp_set_error(CDTP_SERVER_NOT_DONE, 0);
        return;
    }

    free(server->sock);
    _cdtp_client_map_free(server->clients);
    free(server);
}
