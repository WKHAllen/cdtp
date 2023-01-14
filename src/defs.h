/**
 * CDTP type definitions.
 */

#pragma once
#ifndef CDTP_DEFS_H
#define CDTP_DEFS_H

#include "util.h"

/**
 * Server receive event callback function.
 */
typedef void (*ServerOnRecvCallback)(size_t, void *, size_t, void *);

/**
 * Server connect event callback function.
 */
typedef void (*ServerOnConnectCallback)(size_t, void *);

/**
 * Server disconnect event callback function.
 */
typedef void (*ServerOnDisconnectCallback)(size_t, void *);

/**
 * Client receive event callback function.
 */
typedef void (*ClientOnRecvCallback)(void *, size_t, void *);

/**
 * Client disconnect event callback function.
 */
typedef void (*ClientOnDisconnectedCallback)(void *);

/**
 * Generic socket type.
 */
typedef struct _CDTPSocket {
#ifdef _WIN32
    SOCKET sock;
#else
    int sock;
#endif
    struct sockaddr_in address;
} CDTPSocket;

/**
 * Client map node type.
 */
typedef struct _CDTPClientMapNode {
    int allocated;
    size_t client_id;
    CDTPSocket *sock;
} CDTPClientMapNode;

/**
 * Client map type.
 */
typedef struct _CDTPClientMap {
    size_t size;
    size_t capacity;
    CDTPClientMapNode **nodes;
} CDTPClientMap;

/**
 * Client map iterator node type.
 */
typedef struct _CDTPClientMapIterNode {
    size_t client_id;
    CDTPSocket *sock;
} CDTPClientMapIterNode;

/**
 * Client map iterator type.
 */
typedef struct _CDTPClientMapIter {
    size_t size;
    CDTPClientMapIterNode **clients;
} CDTPClientMapIter;

/**
 * Socket server type.
 */
typedef struct _CDTPServer {
    ServerOnRecvCallback on_recv;
    ServerOnConnectCallback on_connect;
    ServerOnDisconnectCallback on_disconnect;
    void *on_recv_arg;
    void *on_connect_arg;
    void *on_disconnect_arg;
    int serving;
    int done;
    CDTPSocket *sock;
    CDTPClientMap *clients;
    size_t next_client_id;
#ifdef _WIN32
    HANDLE serve_thread;
#else
    pthread_t serve_thread;
#endif
} CDTPServer;

/**
 * Socket client type.
 */
typedef struct _CDTPClient {
    ClientOnRecvCallback on_recv;
    ClientOnDisconnectedCallback on_disconnected;
    void *on_recv_arg;
    void *on_disconnected_arg;
    int connected;
    int done;
    CDTPSocket *sock;
#ifdef _WIN32
    HANDLE handle_thread;
#else
    pthread_t handle_thread;
#endif
} CDTPClient;

#endif // CDTP_DEFS_H
