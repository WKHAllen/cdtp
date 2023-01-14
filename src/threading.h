/**
 * CDTP threading interface.
 */

#pragma once
#ifndef CDTP_THREADING_H
#define CDTP_THREADING_H

#include "util.h"
#include "defs.h"
#include "server.h"
#include <string.h>

#ifdef _WIN32
#  include <Windows.h>
#else
#  include <pthread.h>
#endif

/**
 * Call the server `on_recv` event function in another thread.
 *
 * @param func A pointer to the event function.
 * @param server The socket server itself.
 * @param client_id The client ID parameter.
 * @param data The data parameter.
 * @param data_size The data size parameter.
 * @param arg The function argument parameter.
 */
void _cdtp_start_thread_on_recv_server(
    ServerOnRecvCallback func,
    CDTPServer *server,
    size_t client_id,
    void *data,
    size_t data_size,
    void *arg
);

/**
 * Call the server `on_connect` event function in another thread.
 *
 * @param func A pointer to the event function.
 * @param server The socket server itself.
 * @param client_id The client ID parameter.
 * @param arg The function argument parameter.
 */
void _cdtp_start_thread_on_connect(
    ServerOnConnectCallback func,
    CDTPServer *server,
    size_t client_id,
    void *arg
);

/**
 * Call the server `on_disconnect` event function in another thread.
 *
 * @param func A pointer to the event function.
 * @param server The socket server itself.
 * @param client_id The client ID parameter.
 * @param arg The function argument parameter.
 */
void _cdtp_start_thread_on_disconnect(
    ServerOnDisconnectCallback func,
    CDTPServer *server,
    size_t client_id,
    void *arg
);

/**
 * Call the client `on_recv` event function in another thread.
 *
 * @param func A pointer to the event function.
 * @param client The socket client itself.
 * @param data The data parameter.
 * @param data_size The data size parameter.
 * @param arg The function argument parameter.
 */
void _cdtp_start_thread_on_recv_client(
    ClientOnRecvCallback func,
    CDTPClient *client,
    void *data,
    size_t data_size,
    void *arg
);

/**
 * Call the client `on_disconnected` event function in another thread.
 *
 * @param func A pointer to the event function.
 * @param client The socket client itself.
 * @param arg The function argument parameter.
 */
void _cdtp_start_thread_on_disconnected(
    ClientOnDisconnectedCallback func,
    CDTPClient *client,
    void *arg
);

/**
 * Call the server's serve function in a separate thread.
 *
 * @param func The serve function.
 * @param server The socket server.
 * @return A handle to the thread.
 */
#ifdef _WIN32
HANDLE _cdtp_start_serve_thread(
    void (*func)(CDTPServer *),
    CDTPServer *server
);
#else
pthread_t _cdtp_start_serve_thread(
    void (*func)(CDTPServer *),
    CDTPServer *server
);
#endif

/**
 * Call the client's handle function in a separate thread.
 *
 * @param func The handle function.
 * @param client The socket client.
 * @return A handle to the thread.
 */
#ifdef _WIN32
HANDLE _cdtp_start_handle_thread(
    void (*func)(CDTPClient *),
    CDTPClient *client
);
#else
pthread_t _cdtp_start_handle_thread(
    void (*func)(CDTPClient *),
    CDTPClient *client
);
#endif

#endif // CDTP_THREADING_H
