/*
 * Threading functions and definitions for cdtp
 */

#pragma once
#ifndef CDTP_THREADING_H
#define CDTP_THREADING_H

#include "util.h"
#include <string.h>

#ifdef _WIN32
    #include <Windows.h>
#else
    #include <pthread.h>
#endif

typedef struct CDTPEventFunc CDTPEventFunc;

// Thread function
#ifdef _WIN32
DWORD WINAPI _cdtp_thread(LPVOID func_info);
#else
void *_cdtp_thread(void *func_info);
#endif

// Call a function using a thread
void _cdtp_start_thread(CDTPEventFunc *func_info);

// Call on_recv (server)
void _cdtp_start_thread_on_recv_server(void (*func)(int, void *, void *), int client_id, void *data, void *arg);

// Call on_connect (server)
void _cdtp_start_thread_on_connect(void (*func)(int, void *), int client_id, void *arg);

// Call on_disconnect (server)
void _cdtp_start_thread_on_disconnect(void (*func)(int, void *), int client_id, void *arg);

// Call on_recv (client)
void _cdtp_start_thread_on_recv_client(void (*func)(void *, void *), void *data, void *arg);

// Call on_disconnected (client)
void _cdtp_start_thread_on_disconnected(void (*func)(void *), void *arg);

#endif // CDTP_THREADING_H
