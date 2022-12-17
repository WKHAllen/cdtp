/*
 * Type definitions for cdtp.
 */

#pragma once
#ifndef CDTP_DEFS_H
#define CDTP_DEFS_H

#include "util.h"

// Callback function types
typedef void (*ServerOnRecvCallback)(size_t, void*, size_t, void*);
typedef void (*ServerOnConnectCallback)(size_t, void*);
typedef void (*ServerOnDisconnectCallback)(size_t, void*);
typedef void (*ClientOnRecvCallback)(void*, size_t, void*);
typedef void (*ClientOnDisconnectedCallback)(void*);

// Socket type
typedef struct _CDTPSocket {
#ifdef _WIN32
    SOCKET sock;
#else
    int sock;
#endif
    struct sockaddr_in address;
} CDTPSocket;

// Socket server type
typedef struct _CDTPServer {
    size_t max_clients;
    ServerOnRecvCallback on_recv;
    ServerOnConnectCallback on_connect;
    ServerOnDisconnectCallback on_disconnect;
    void* on_recv_arg;
    void* on_connect_arg;
    void* on_disconnect_arg;
    int serving;
    int done;
    size_t num_clients;
    CDTPSocket* sock;
    CDTPSocket** clients;
    int* allocated_clients;
#ifdef _WIN32
    HANDLE serve_thread;
#else
    pthread_t serve_thread;
#endif
} CDTPServer;

// Socket client type
typedef struct _CDTPClient {
    ClientOnRecvCallback on_recv;
    ClientOnDisconnectedCallback on_disconnected;
    void* on_recv_arg;
    void* on_disconnected_arg;
    int connected;
    int done;
    CDTPSocket* sock;
#ifdef _WIN32
    HANDLE handle_thread;
#else
    pthread_t handle_thread;
    CDTPServer* local_server;
#endif
} CDTPClient;

#endif // CDTP_DEFS_H
