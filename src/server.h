/*
 * Server types and functions for cdtp
 */

#pragma once
#ifndef CDTP_SERVER_H
#define CDTP_SERVER_H

#include "defs.h"
#include "util.h"
#include "threading.h"

 // Socket server listen backlog
#define CDTP_LISTEN_BACKLOG 3

// Server max clients indicator
#define CDTP_MAX_CLIENTS_REACHED (-1)

/*
 * Server creation/initialization
 *
 * max_clients:       the maximum number of clients the server will allow
 * on_recv:           pointer to a function that will be called when a packet is received
 * on_connect:        pointer to a function that will be called when a client connects
 * on_disconnect:     pointer to a function that will be called when a client disconnects
 * on_recv_arg:       a value that will be passed to the on_recv function
 * on_connect_arg:    a value that will be passed to the on_connect function
 * on_disconnect_arg: a value that will be passed to the on_disconnect function
 * blocking:          whether or not the cdtp_server_start function and related functions will block
 * event_blocking:    whether or not on_recv, on_connect, and on_disconnect will block
 */
CDTPServer* cdtp_server(
  size_t max_clients,
  void (*on_recv)(int, void*, size_t, void*),
  void (*on_connect)(int, void*),
  void (*on_disconnect)(int, void*),
  void* on_recv_arg,
  void* on_connect_arg,
  void* on_disconnect_arg,
  int blocking,
  int event_blocking
);

/*
 * Server creation/initialization, with default blocking parameters
 *
 * max_clients:       the maximum number of clients the server will allow
 * on_recv:           pointer to a function that will be called when a packet is received
 * on_connect:        pointer to a function that will be called when a client connects
 * on_disconnect:     pointer to a function that will be called when a client disconnects
 * on_recv_arg:       a value that will be passed to the on_recv function
 * on_connect_arg:    a value that will be passed to the on_connect function
 * on_disconnect_arg: a value that will be passed to the on_disconnect function
 *
 * blocking and event_blocking are set to false
 */
CDTPServer* cdtp_server_default(size_t max_clients,
  void (*on_recv)(int, void*, size_t, void*),
  void (*on_connect)(int, void*),
  void (*on_disconnect)(int, void*),
  void* on_recv_arg,
  void* on_connect_arg,
  void* on_disconnect_arg
);

/*
 * Start a server
 *
 * server: the server object
 * host:   the host as a string
 * port:   the port as an integer
 */
void cdtp_server_start(CDTPServer* server, char* host, int port);

/*
 * Start a server with a non-string host
 *
 * server: the server object
 * host:   the host in the socket-library-provided format
 * port:   the port as an integer
 */
#ifdef _WIN32
void cdtp_server_start_host(CDTPServer* server, ULONG host, int port);
#else
void cdtp_server_start_host(CDTPServer* server, in_addr_t host, int port);
#endif

/*
 * Start a server with the default host
 *
 * server: the server object
 * port:   the port as an integer
 *
 * host is set to INADDR_ANY
 */
void cdtp_server_start_default_host(CDTPServer* server, int port);

/*
 * Start a server with the default port
 *
 * server: the server object
 * host:   the host as a string
 *
 * port is set to CDTP_PORT
 */
void cdtp_server_start_default_port(CDTPServer* server, char* host);

/*
 * Start a server with a non-string host and the default port
 *
 * server: the server object
 * host:   the host in the socket-library-provided format
 *
 * port is set to CDTP_PORT
 */
#ifdef _WIN32
void cdtp_server_start_host_default_port(CDTPServer* server, ULONG host);
#else
void cdtp_server_start_host_default_port(CDTPServer* server, in_addr_t host);
#endif

/*
 * Start a server with the default host and the default port
 *
 * server: the server object
 *
 * host is set to INADDR_ANY, port is set to CDTP_PORT
 */
void cdtp_server_start_default(CDTPServer* server);

/*
 * Stop the server, disconnect all clients, and free up memory
 *
 * server: the server object
 */
void cdtp_server_stop(CDTPServer* server);

/*
 * Check if the server is serving
 *
 * server: the server object
 */
int cdtp_server_serving(CDTPServer* server);

/*
 * Get the server address
 *
 * server: the server object
 */
struct sockaddr_in cdtp_server_addr(CDTPServer* server);

/*
 * Get the server ip address
 *
 * server: the server object
 *
 * The returned value's memory will need to be freed after use
 */
char* cdtp_server_host(CDTPServer* server);

/*
 * Get the server port
 *
 * server: the server object
 */
int cdtp_server_port(CDTPServer* server);

/*
 * Remove a client by ID
 *
 * server:    the server object
 * client_id: the ID of the client to be removed
 */
void cdtp_server_remove_client(CDTPServer* server, int client_id);

/*
 * Send data to a client
 *
 * server:    the server object
 * client_id: the ID of the client to send the data to
 * data:      the data to send
 * data_size: the size of the data
 */
void cdtp_server_send(CDTPServer* server, int client_id, void* data, size_t data_size);

/*
 * Send data to all clients
 *
 * server:    the server object
 * data:      the data to send
 * data_size: the size of the data
 */
void cdtp_server_send_all(CDTPServer* server, void* data, size_t data_size);

// Call the serve function
void _cdtp_server_call_serve(CDTPServer* server);

// Server serve function
void _cdtp_server_serve(CDTPServer* server);

// Call the on_recv function
void _cdtp_server_call_on_recv(CDTPServer* server, int client_id, void* data, size_t data_size);

// Call the on_connect function
void _cdtp_server_call_on_connect(CDTPServer* server, int client_id);

// Call the on_disconnect function
void _cdtp_server_call_on_disconnect(CDTPServer* server, int client_id);

// Get the first available client ID
int _cdtp_server_new_client_id(CDTPServer* server);

// Send a client a status code
#ifdef _WIN32
void _cdtp_server_send_status(SOCKET client_sock, int status_code);
#else
void _cdtp_server_send_status(int client_sock, int status_code);
#endif

// Disconnect a socket
void _cdtp_server_disconnect_sock(CDTPServer* server, int client_id);

#endif // CDTP_SERVER_H
