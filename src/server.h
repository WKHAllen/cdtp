/*
 * Server types and functions for cdtp
 */

#ifndef CDTP_SERVER_H
#define CDTP_SERVER_H

#include "util.h"

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
    #ifdef CDTP_WINSOCK_INIT
        #undef CDTP_WINSOCK_INIT
    #endif
#else
    #include <sys/socket.h>
#endif

typedef struct
{
    void (*on_recv      )(int, void *, void *);
    void (*on_connect   )(int, void *, void *);
    void (*on_disconnect)(int, void *, void *);
    void *on_recv_arg;
    void *on_connect_arg;
    void *on_disconnect_arg;
    int blocking;
    int event_blocking;
    int daemon;
    int serving;
#ifdef _WIN32
    SOCKET sock;
#else
    int sock;
#endif
} CDTPServer;

CDTPServer cdtp_server(void (*on_recv      )(int, void *, void *),
                       void (*on_connect   )(int, void *, void *),
                       void (*on_disconnect)(int, void *, void *),
                       void *on_recv_arg, void *on_connect_arg, void *on_disconnect_arg,
                       int blocking, int event_blocking, int daemon);

CDTPServer cdtp_server_default(void (*on_recv      )(int, void *, void *),
                               void (*on_connect   )(int, void *, void *),
                               void (*on_disconnect)(int, void *, void *),
                               void *on_recv_arg, void *on_connect_arg, void *on_disconnect_arg);

#endif /* CDTP_SERVER_H */
