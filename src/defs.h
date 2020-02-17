/*
 * CDTP type definitions
 */

#pragma once
#ifndef CDTP_DEFS_H
#define CDTP_DEFS_H

#include "util.h"

// Socket type
typedef struct CDTPSocket
{
#ifdef _WIN32
    SOCKET sock;
#else
    int sock;
#endif
    struct sockaddr_in address;
} CDTPSocket;

// Socket server type
typedef struct CDTPServer
{
    size_t max_clients;
    void (*on_recv      )(int, void *, size_t, void *);
    void (*on_connect   )(int, void *);
    void (*on_disconnect)(int, void *);
    void *on_recv_arg;
    void *on_connect_arg;
    void *on_disconnect_arg;
    int blocking;
    int event_blocking;
    int serving;
    int done;
    int num_clients;
    CDTPSocket *sock;
    CDTPSocket **clients;
    int *allocated_clients;
#ifdef _WIN32
    HANDLE serve_thread;
#else
    pthread_t serve_thread;
#endif
} CDTPServer;

// Socket client type
typedef struct CDTPClient
{
    void (*on_recv        )(void *, size_t, void *);
    void (*on_disconnected)(void *);
    void *on_recv_arg;
    void *on_disconnected_arg;
    int blocking;
    int event_blocking;
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
