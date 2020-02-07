/*
 * Server types and functions for cdtp
 */

#ifndef CDTP_SERVER_H
#define CDTP_SERVER_H

#include "util.h"

#include <stdlib.h>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
    #ifdef CDTP_WINSOCK_INIT
        #undef CDTP_WINSOCK_INIT
    #endif
#else
    #include <unistd.h>
    #include <stdio.h>
    #include <sys/socket.h>
    #include <stdlib.h>
    #include <netinet/in.h>
    #include <string.h>
    #include <arpa/inet.h>
#endif

#define CDTP_LISTEN_BACKLOG 3

#define CDTP_SERVER_SUCCESS             0
#define CDTP_SERVER_WINSOCK_INIT_FAILED 1
#define CDTP_SERVER_SOCK_INIT_FAILED    2
#define CDTP_SERVER_SETSOCKOPT_FAILED   3
#define CDTP_SERVER_ALREADY_SERVING     4
#define CDTP_SERVER_BIND_FAILED         5
#define CDTP_SERVER_LISTEN_FAILED       6

typedef struct _CDTPServer
{
    size_t max_clients;
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
    SOCKET *clients;
#else
    int sock;
    int *clients;
#endif
    int *allocated_clients;
} CDTPServer;

CDTPServer cdtp_server(size_t max_clients,
                       void (*on_recv      )(int, void *, void *),
                       void (*on_connect   )(int, void *, void *),
                       void (*on_disconnect)(int, void *, void *),
                       void *on_recv_arg, void *on_connect_arg, void *on_disconnect_arg,
                       int blocking, int event_blocking, int daemon,
                       int *err);

CDTPServer cdtp_server_default(size_t max_clients,
                               void (*on_recv      )(int, void *, void *),
                               void (*on_connect   )(int, void *, void *),
                               void (*on_disconnect)(int, void *, void *),
                               void *on_recv_arg, void *on_connect_arg, void *on_disconnect_arg,
                               int *err);

int cdtp_start(CDTPServer *server, char *host, int port);

#ifdef _WIN32
int cdtp_start_host(CDTPServer *server, ULONG host, int port);
#else
int cdtp_start_host(CDTPServer *server, in_addr_t host, int port);
#endif

int cdtp_start_default_host(CDTPServer *server, int port);

int cdtp_start_default(CDTPServer *server);

void cdtp_stop(CDTPServer *server);

int cdtp_serving(CDTPServer *server);

void cdtp_serve(CDTPServer *server);

// TODO: add more functions

#endif /* CDTP_SERVER_H */
