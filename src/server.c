#include "server.h"

// Get the first available client ID
size_t _cdtp_server_new_client_id(CDTPServer* server)
{
    if (server->num_clients >= server->max_clients) {
        return CDTP_MAX_CLIENTS_REACHED;
    }

    for (size_t i = 0; i < server->max_clients; i++) {
        if (server->allocated_clients[i] != CDTP_TRUE) {
            return i;
        }
    }

    return CDTP_MAX_CLIENTS_REACHED;
}

// Send a client a status code
#ifdef _WIN32
void _cdtp_server_send_status(SOCKET client_sock, int status_code)
#else
void _cdtp_server_send_status(int client_sock, int status_code)
#endif
{
    char* message = _cdtp_construct_message(&status_code, sizeof(status_code));

    if (send(client_sock, message, CDTP_LENSIZE + sizeof(status_code), 0) < 0) {
        _cdtp_set_err(CDTP_STATUS_SEND_FAILED);
    }

    free(message);
}

// Disconnect a socket
void _cdtp_server_disconnect_sock(CDTPServer* server, size_t client_id)
{
#ifdef _WIN32
    closesocket(server->clients[client_id]->sock);
#else
    close(server->clients[client_id]->sock);
#endif

    server->allocated_clients[client_id] = CDTP_FALSE;
    free(server->clients[client_id]);
    server->num_clients--;
}

// Call the on_recv function
void _cdtp_server_call_on_recv(CDTPServer* server, size_t client_id, void* data, size_t data_size)
{
    if (server->on_recv != NULL) {
        _cdtp_start_thread_on_recv_server(server->on_recv, client_id, data, data_size, server->on_recv_arg);
    }
}

// Call the on_connect function
void _cdtp_server_call_on_connect(CDTPServer* server, size_t client_id)
{
    if (server->on_connect != NULL) {
        _cdtp_start_thread_on_connect(server->on_connect, client_id, server->on_connect_arg);
    }
}

// Call the on_disconnect function
void _cdtp_server_call_on_disconnect(CDTPServer* server, size_t client_id)
{
    if (server->on_disconnect != NULL) {
        _cdtp_start_thread_on_disconnect(server->on_disconnect, client_id, server->on_disconnect_arg);
    }
}

// Server serve function
void _cdtp_server_serve(CDTPServer* server)
{
    fd_set read_socks;
    int activity;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

#ifdef _WIN32
    SOCKET new_sock;
    int max_sd = 0;
#else
    int new_sock;
    int max_sd = server->sock->sock;
#endif

    unsigned char size_buffer[CDTP_LENSIZE];

    while (server->serving == CDTP_TRUE) {
        // Create the read sockets
        FD_ZERO(&read_socks);
        FD_SET(server->sock->sock, &read_socks);

        for (size_t i = 0; i < server->max_clients; i++) {
            if (server->allocated_clients[i] == CDTP_TRUE) {
                FD_SET(server->clients[i]->sock, &read_socks);

#ifndef _WIN32
                if (server->clients[i]->sock > max_sd) {
                    max_sd = server->clients[i]->sock;
                }
#endif
            }
        }

        // Wait for activity
        activity = select(max_sd + 1, &read_socks, NULL, NULL, NULL);

        // Check if the server has been stopped
        if (server->serving != CDTP_TRUE) {
            return;
        }

        // Check for select errors
        if (activity < 0) {
            _cdtp_set_err(CDTP_SELECT_FAILED);
            return;
        }

        // Check if something happened on the main socket
        if (FD_ISSET(server->sock->sock, &read_socks)) {
            // Accept the new socket and check if an error has occurred
#ifdef _WIN32
            new_sock = accept(server->sock->sock, (struct sockaddr*)&address, (int*)&addrlen);

            if (new_sock == INVALID_SOCKET) {
                int err_code = WSAGetLastError();

                if (err_code != WSAENOTSOCK || server->serving == CDTP_TRUE) {
                    _cdtp_set_error(CDTP_SOCKET_ACCEPT_FAILED, err_code);
                }

                return;
            }
#else
            new_sock = accept(server->sock->sock, (struct sockaddr*)&address, (socklen_t*)&addrlen);

            if (new_sock < 0) {
                int err_code = errno;

                if (err_code != ENOTSOCK || server->serving == CDTP_TRUE) {
                    _cdtp_set_error(CDTP_SOCKET_ACCEPT_FAILED, err_code);
                }

                return;
            }
#endif

            // Put new socket in the client list
            int new_client_id = _cdtp_server_new_client_id(server);

            if (new_client_id != CDTP_MAX_CLIENTS_REACHED) {
                // Add the new socket to the client array
                server->clients[new_client_id] = (CDTPSocket*) malloc(sizeof(CDTPSocket));
                server->clients[new_client_id]->sock = new_sock;
                memcpy(&(server->clients[new_client_id]->address), &address, sizeof(address));
                server->allocated_clients[new_client_id] = CDTP_TRUE;
                server->num_clients++;
                _cdtp_server_send_status(new_sock, CDTP_SUCCESS);
                _cdtp_server_call_on_connect(server, new_client_id);
            }
            else {
                // Tell the client that the server is full
                _cdtp_server_send_status(new_sock, CDTP_SERVER_FULL);

#ifdef _WIN32
                closesocket(new_sock);
#else
                close(new_sock);
#endif

            }
        }

        // Check if something happened on one of the client sockets
        for (size_t i = 0; i < server->max_clients; i++) {
            if (server->allocated_clients[i] == CDTP_TRUE && FD_ISSET(server->clients[i]->sock, &read_socks)) {
#ifdef _WIN32
                int recv_code = recv(server->clients[i]->sock, (char*)size_buffer, CDTP_LENSIZE, 0);

                if (recv_code == SOCKET_ERROR) {
                    int err_code = WSAGetLastError();

                    if (err_code == WSAECONNRESET) {
                        _cdtp_server_disconnect_sock(server, i);
                        _cdtp_server_call_on_disconnect(server, i);
                    }
                    else {
                        _cdtp_set_error(CDTP_SERVER_RECV_FAILED, err_code);
                        return;
                    }
                }
                else if (recv_code == 0) {
                    _cdtp_server_disconnect_sock(server, i);
                    _cdtp_server_call_on_disconnect(server, i);
                }
                else {
                    size_t msg_size = _cdtp_decode_message_size(size_buffer);
                    char* buffer = (char*) malloc(msg_size * sizeof(char));

                    // Wait in case the message is sent in multiple chunks
                    cdtp_sleep(0.01);

                    recv_code = recv(server->clients[i]->sock, buffer, msg_size, 0);

                    if (recv_code == SOCKET_ERROR) {
                        int err_code = WSAGetLastError();

                        if (err_code == WSAECONNRESET) {
                            _cdtp_server_disconnect_sock(server, i);
                            _cdtp_server_call_on_disconnect(server, i);
                        }
                        else {
                            _cdtp_set_error(CDTP_SERVER_RECV_FAILED, err_code);
                            return;
                        }
                    }
                    else if (recv_code == 0) {
                        _cdtp_server_disconnect_sock(server, i);
                        _cdtp_server_call_on_disconnect(server, i);
                    }
                    else {
                        _cdtp_server_call_on_recv(server, i, (void*)buffer, msg_size);
                    }
                }
#else
                int recv_code = read(server->clients[i]->sock, size_buffer, CDTP_LENSIZE);

                if (recv_code == 0) {
                    _cdtp_server_disconnect_sock(server, i);
                    _cdtp_server_call_on_disconnect(server, i);
                }
                else {
                    size_t msg_size = _cdtp_decode_message_size(size_buffer);
                    char* buffer = (char*) malloc(msg_size * sizeof(char));

                    // Wait in case the message is sent in multiple chunks
                    cdtp_sleep(0.01);

                    recv_code = read(server->clients[i]->sock, buffer, msg_size);

                    if (recv_code == 0) {
                        _cdtp_server_disconnect_sock(server, i);
                        _cdtp_server_call_on_disconnect(server, i);
                    }
                    else {
                        _cdtp_server_call_on_recv(server, i, (void*)buffer, msg_size);
                    }
                }
#endif
            }
        }
    }
}

// Call the serve function
void _cdtp_server_call_serve(CDTPServer* server)
{
    server->serve_thread = _cdtp_start_serve_thread(_cdtp_server_serve, server);
}

EXPORT CDTPServer* cdtp_server(
    size_t max_clients,
    ServerOnRecvCallback on_recv,
    ServerOnConnectCallback on_connect,
    ServerOnDisconnectCallback on_disconnect,
    void* on_recv_arg,
    void* on_connect_arg,
    void* on_disconnect_arg
)
{
    CDTPServer* server = (CDTPServer*) malloc(sizeof(CDTPServer));

    // Initialize the server object
    server->max_clients = max_clients;
    server->on_recv = on_recv;
    server->on_connect = on_connect;
    server->on_disconnect = on_disconnect;
    server->on_recv_arg = on_recv_arg;
    server->on_connect_arg = on_connect_arg;
    server->on_disconnect_arg = on_disconnect_arg;
    server->serving = CDTP_FALSE;
    server->done = CDTP_FALSE;
    server->num_clients = 0;

    // Initialize the library
    if (CDTP_INIT != CDTP_TRUE) {
        int return_code = _cdtp_init();

        if (return_code != 0) {
            _cdtp_set_error(CDTP_WINSOCK_INIT_FAILED, return_code);
            return NULL;
        }
    }

    // Initialize the server socket
    server->sock = (CDTPSocket*) malloc(sizeof(CDTPSocket));

    // Initialize the socket info
    int opt = 1;

#ifdef _WIN32
    // Initialize the socket
    if ((server->sock->sock = socket(CDTP_ADDRESS_FAMILY, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        _cdtp_set_err(CDTP_SERVER_SOCK_INIT_FAILED);
        return NULL;
    }
    if (setsockopt(server->sock->sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
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

    // Initialize the client socket array
    server->clients = (CDTPSocket**) malloc(max_clients * sizeof(CDTPSocket*));

    // Initialize the allocated clients array
    server->allocated_clients = (int*) malloc(max_clients * sizeof(int));

    for (size_t i = 0; i < max_clients; i++) {
        server->allocated_clients[i] = CDTP_FALSE;
    }

    return server;
}

EXPORT void cdtp_server_start(CDTPServer* server, char* host, unsigned short port)
{
    // Change 'localhost' to '127.0.0.1'
    if (strcmp(host, "localhost") == 0) {
        host = "127.0.0.1";
    }

    // Make sure the server has not been run before
    if (server->done == CDTP_TRUE) {
        _cdtp_set_error(CDTP_SERVER_CANNOT_RESTART, 0);
        return;
    }

    // Make sure the server is not already serving
    if (server->serving == CDTP_TRUE) {
        _cdtp_set_error(CDTP_SERVER_ALREADY_SERVING, 0);
        return;
    }

    // Set the server address
#ifdef _WIN32
    int addrlen = CDTP_ADDRSTRLEN;

    if (WSAStringToAddress(host, CDTP_ADDRESS_FAMILY, NULL, (LPSOCKADDR) & (server->sock->address), &addrlen) != 0) {
        _cdtp_set_err(CDTP_SERVER_ADDRESS_FAILED);
        return;
    }
#else
    if (inet_pton(CDTP_ADDRESS_FAMILY, host, &(server->sock->address)) != 1) {
        _cdtp_set_err(CDTP_SERVER_ADDRESS_FAILED);
        return;
    }
#endif

    server->sock->address.sin_family = CDTP_ADDRESS_FAMILY;
    server->sock->address.sin_port = htons(port);

    // Bind the address to the server
    if (bind(server->sock->sock, (struct sockaddr*)&(server->sock->address), sizeof(server->sock->address)) < 0) {
        _cdtp_set_err(CDTP_SERVER_BIND_FAILED);
        return;
    }

    // Listen for connections
    if (listen(server->sock->sock, CDTP_LISTEN_BACKLOG) < 0) {
        _cdtp_set_err(CDTP_SERVER_LISTEN_FAILED);
        return;
    }

    // Serve
    server->serving = CDTP_TRUE;
    _cdtp_server_call_serve(server);
}

EXPORT void cdtp_server_stop(CDTPServer* server)
{
    server->serving = CDTP_FALSE;
    server->done = CDTP_TRUE;

#ifdef _WIN32
    // Close sockets
    for (size_t i = 0; i < server->max_clients; i++) {
        if (server->allocated_clients[i] == CDTP_TRUE) {
            if (closesocket(server->clients[i]->sock) != 0) {
                _cdtp_set_err(CDTP_SERVER_STOP_FAILED);
                return;
            }
        }
    }

    if (closesocket(server->sock->sock) != 0) {
        _cdtp_set_err(CDTP_SERVER_STOP_FAILED);
        return;
    }

    // Wait for threads to exit
    if (WaitForSingleObject(server->serve_thread, INFINITE) == WAIT_FAILED) {
        _cdtp_set_error(CDTP_SERVE_THREAD_NOT_CLOSING, GetLastError());
        return;
    }
#else
    // Close sockets
    for (size_t i = 0; i < server->max_clients; i++) {
        if (server->allocated_clients[i] == CDTP_TRUE) {
            if (close(server->clients[i]->sock) != 0) {
                _cdtp_set_err(CDTP_SERVER_STOP_FAILED);
                return;
            }
        }
    }

    // Force the select function to return by attempting to connect
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = server->sock->address.sin_addr.s_addr;
    addr.sin_family = server->sock->address.sin_family;
    addr.sin_port = server->sock->address.sin_port;
    int client_sock = socket(CDTP_ADDRESS_FAMILY, SOCK_STREAM, 0);

    if (client_sock == -1) {
        _cdtp_set_err(CDTP_SERVER_STOP_FAILED);
        return;
    }

    if (connect(client_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        _cdtp_set_err(CDTP_SERVER_STOP_FAILED);
        return;
    }

    cdtp_sleep(0.01);

    if (close(client_sock) != 0) {
        _cdtp_set_err(CDTP_SERVER_STOP_FAILED);
        return;
    }

    if (close(server->sock->sock) != 0) {
        _cdtp_set_err(CDTP_SERVER_STOP_FAILED);
        return;
    }

    // Wait for threads to exit
    int err_code = pthread_join(server->serve_thread, NULL);

    if (err_code != 0) {
        _cdtp_set_error(CDTP_SERVE_THREAD_NOT_CLOSING, err_code);
        return;
    }
#endif

    // Free memory
    free(server->sock);
    free(server->clients);
    free(server->allocated_clients);
    free(server);
}

EXPORT int cdtp_server_serving(CDTPServer* server)
{
    return server->serving;
}

EXPORT char* cdtp_server_host(CDTPServer* server)
{
    // Make sure the server is running
    if (server->serving != CDTP_TRUE) {
        _cdtp_set_error(CDTP_SERVER_NOT_SERVING, 0);
        return NULL;
    }

    char* addr = (char*) malloc(CDTP_ADDRSTRLEN * sizeof(char));

#ifdef _WIN32
    int addrlen = CDTP_ADDRSTRLEN;

    if (WSAAddressToString((LPSOCKADDR) & (server->sock->address), sizeof(server->sock->address), NULL, addr, (LPDWORD)&addrlen) != 0) {
        _cdtp_set_err(CDTP_SERVER_ADDRESS_FAILED);
        return NULL;
    }

    // Remove the port
    for (int i = 0; i < CDTP_ADDRSTRLEN && addr[i] != '\0'; i++) {
        if (addr[i] == ':') {
            addr[i] = '\0';
            break;
        }
    }
#else
    if (inet_ntop(CDTP_ADDRESS_FAMILY, &(server->sock->address), addr, CDTP_ADDRSTRLEN) == NULL) {
        _cdtp_set_err(CDTP_SERVER_ADDRESS_FAILED);
        return NULL;
    }
#endif

    return addr;
}

EXPORT int cdtp_server_port(CDTPServer* server)
{
    // Make sure the server is running
    if (server->serving != CDTP_TRUE) {
        _cdtp_set_error(CDTP_SERVER_NOT_SERVING, 0);
        return 0;
    }

    return ntohs(server->sock->address.sin_port);
}

EXPORT void cdtp_server_remove_client(CDTPServer* server, size_t client_id)
{
    // Make sure the server is running
    if (server->serving != CDTP_TRUE) {
        _cdtp_set_error(CDTP_SERVER_NOT_SERVING, 0);
        return;
    }

    if (client_id >= server->max_clients || server->allocated_clients[client_id] != CDTP_TRUE) {
        _cdtp_set_error(CDTP_CLIENT_DOES_NOT_EXIST, 0);
        return;
    }

#ifdef _WIN32
    if (closesocket(server->clients[client_id]->sock) != 0) {
        _cdtp_set_err(CDTP_CLIENT_REMOVE_FAILED);
        return;
    }
#else
    if (close(server->clients[client_id]->sock) != 0) {
        _cdtp_set_err(CDTP_CLIENT_REMOVE_FAILED);
        return;
    }
#endif

    server->allocated_clients[client_id] = CDTP_FALSE;
}

EXPORT void cdtp_server_send(CDTPServer* server, size_t client_id, void* data, size_t data_size)
{
    // Make sure the server is running
    if (server->serving != CDTP_TRUE) {
        _cdtp_set_error(CDTP_SERVER_NOT_SERVING, 0);
        return;
    }

    char* message = _cdtp_construct_message(data, data_size);

    if (send(server->clients[client_id]->sock, message, CDTP_LENSIZE + data_size, 0) < 0) {
        _cdtp_set_err(CDTP_SERVER_SEND_FAILED);
    }

    free(message);
}

EXPORT void cdtp_server_send_all(CDTPServer* server, void* data, size_t data_size)
{
    // Make sure the server is running
    if (server->serving != CDTP_TRUE) {
        _cdtp_set_error(CDTP_SERVER_NOT_SERVING, 0);
        return;
    }

    char* message = _cdtp_construct_message(data, data_size);

    for (size_t i = 0; i < server->max_clients; i++) {
        if (server->allocated_clients[i] == CDTP_TRUE) {
            if (send(server->clients[i]->sock, message, CDTP_LENSIZE + data_size, 0) < 0) {
                _cdtp_set_err(CDTP_SERVER_SEND_FAILED);
            }
        }
    }

    free(message);
}
