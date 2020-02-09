/*
 * Server types and functions for cdtp
 */

#ifndef CDTP_SERVER_H
#define CDTP_SERVER_H

#include "util.h"
#include <stddef.h>

// Socket server listen backlog
#define CDTP_LISTEN_BACKLOG 3

// Socket server initialization return codes
#define CDTP_SERVER_SUCCESS             0
#define CDTP_SERVER_WINSOCK_INIT_FAILED 1
#define CDTP_SERVER_SOCK_INIT_FAILED    2
#define CDTP_SERVER_SETSOCKOPT_FAILED   3
#define CDTP_SERVER_ALREADY_SERVING     4
#define CDTP_SERVER_BIND_FAILED         5
#define CDTP_SERVER_LISTEN_FAILED       6

// Type definitions
typedef struct CDTPSocket CDTPSocket;
typedef struct CDTPServer CDTPServer;

/* 
 * Server creation/initialization
 * 
 * max_clients:         the maximum number of clients the server will allow
 * on_recv:             pointer to a function that will be called when a packet is received
 * on_connect:          pointer to a function that will be called when a client connects
 * on_disconnect:       pointer to a function that will be called when a client disconnects
 * on_recv_arg:         an value that will be passed to the on_recv function
 * on_connect_arg:      an value that will be passed to the on_connect function
 * on_disconnect_arg:   an value that will be passed to the on_disconnect function
 * blocking:            whether or not the cdtp_server_start function and related functions will block
 * event_blocking:      whether or not on_recv, on_connect, and on_disconnect will block
 * daemon:              whether or not the threads used are daemons
 * err:                 the return code, should an error occur
 */ 
CDTPServer *cdtp_server(size_t max_clients,
                       void (*on_recv      )(int, void *, void *),
                       void (*on_connect   )(int, void *, void *),
                       void (*on_disconnect)(int, void *, void *),
                       void *on_recv_arg, void *on_connect_arg, void *on_disconnect_arg,
                       int blocking, int event_blocking, int daemon,
                       int *err);

/*
 * Server creation/initialization, with default blocking and daemon parameters
 * 
 * max_clients:         the maximum number of clients the server will allow
 * on_recv:             pointer to a function that will be called when a packet is received
 * on_connect:          pointer to a function that will be called when a client connects
 * on_disconnect:       pointer to a function that will be called when a client disconnects
 * on_recv_arg:         an value that will be passed to the on_recv function
 * on_connect_arg:      an value that will be passed to the on_connect function
 * on_disconnect_arg:   an value that will be passed to the on_disconnect function
 * err:                 the return code, should an error occur
 * 
 * blocking and event blocking are set to false, daemon is set to true
 */
CDTPServer *cdtp_server_default(size_t max_clients,
                               void (*on_recv      )(int, void *, void *),
                               void (*on_connect   )(int, void *, void *),
                               void (*on_disconnect)(int, void *, void *),
                               void *on_recv_arg, void *on_connect_arg, void *on_disconnect_arg,
                               int *err);

/*
 * Start a server
 * 
 * server: the server object
 * host:   the host as a string
 * port:   the port as an integer
 * 
 * Returns an error code
 */
int cdtp_server_start(CDTPServer *server, char *host, int port);

/*
 * Start a server with a non-string host
 * 
 * server: the server object
 * host:   the host in the socket-library-provided format
 * port:   the port as an integer
 * 
 * Returns an error code
 */
#ifdef _WIN32
int cdtp_server_start_host(CDTPServer *server, ULONG host, int port);
#else
int cdtp_server_start_host(CDTPServer *server, in_addr_t host, int port);
#endif

/*
 * Start a server with the default host
 * 
 * server: the server object
 * port:   the port as an integer
 * 
 * Returns an error code
 * 
 * host is set to INADDR_ANY
 */
int cdtp_server_start_default_host(CDTPServer *server, int port);

/*
 * Start a server with the default port
 * 
 * server: the server object
 * host:   the host as a string
 * 
 * Returns an error code
 * 
 * port is set to CDTP_PORT
 */
int cdtp_server_start_default_port(CDTPServer *server, char *host);

/*
 * Start a server with a non-string host and the default port
 * 
 * server: the server object
 * host:   the host in the socket-library-provided format
 * 
 * Returns an error code
 * 
 * port is set to CDTP_PORT
 */
#ifdef _WIN32
int cdtp_server_start_host_default_port(CDTPServer *server, ULONG host);
#else
int cdtp_server_start_host_default_port(CDTPServer *server, in_addr_t host);
#endif

/*
 * Start a server with the default host and an unused port
 * 
 * server: the server object
 * 
 * Returns an error code
 * 
 * host is set to INADDR_ANY, port is set to CDTP_PORT
 */
int cdtp_server_start_default(CDTPServer *server);

/*
 * Stop the server, disconnect all clients, and free up memory
 * 
 * server: the server object
 */
void cdtp_server_stop(CDTPServer *server);

/*
 * Check if the server is serving
 * 
 * server: the server object
 */
int cdtp_server_serving(CDTPServer *server);

/*
 * Get the server address
 * 
 * server: the server object
 */
struct sockaddr_in cdtp_server_addr(CDTPServer *server);

/*
 * Get the server ip address
 * 
 * server: the server object
 * 
 * The returned value's memory will need to be freed after use
 */
char *cdtp_server_ip(CDTPServer *server);

/*
 * Server serve function
 * 
 * server: the server object
 */
void _cdtp_server_serve(CDTPServer *server);

// TODO: add more functions

#endif // CDTP_SERVER_H
