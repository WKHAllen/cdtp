/*
 * Server types and functions for cdtp.
 */

#pragma once
#ifndef CDTP_SERVER_H
#define CDTP_SERVER_H

#include "defs.h"
#include "util.h"
#include "threading.h"

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
 */
CDTP_EXPORT CDTPServer* cdtp_server(
  size_t max_clients,
  ServerOnRecvCallback on_recv,
  ServerOnConnectCallback on_connect,
  ServerOnDisconnectCallback on_disconnect,
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
CDTP_EXPORT void cdtp_server_start(CDTPServer* server, char* host, unsigned short port);

/*
 * Stop the server, disconnect all clients, and free up memory
 *
 * server: the server object
 */
CDTP_EXPORT void cdtp_server_stop(CDTPServer* server);

/*
 * Check if the server is serving
 *
 * server: the server object
 */
CDTP_EXPORT int cdtp_server_is_serving(CDTPServer* server);

/*
 * Get the server host address.
 *
 * server: the server object
 *
 * The returned value's memory will need to be freed after use
 */
CDTP_EXPORT char* cdtp_server_get_host(CDTPServer* server);

/*
 * Get the server port.
 *
 * server: the server object
 */
CDTP_EXPORT unsigned short cdtp_server_get_port(CDTPServer* server);

/*
 * Get the host of a client.
 *
 * server: the server object
 * client_id: the ID of the client
 *
 * The returned value's memory will need to be freed after use
 */
CDTP_EXPORT char* cdtp_server_get_client_host(CDTPServer* server, size_t client_id);

/*
 * Get the port of a client.
 *
 * server: the server object
 * client_id: the ID of the client
 */
CDTP_EXPORT unsigned short cdtp_server_get_client_port(CDTPServer* server, size_t client_id);

/*
 * Remove a client by ID
 *
 * server:    the server object
 * client_id: the ID of the client to be removed
 */
CDTP_EXPORT void cdtp_server_remove_client(CDTPServer* server, size_t client_id);

/*
 * Send data to a client
 *
 * server:    the server object
 * client_id: the ID of the client to send the data to
 * data:      the data to send
 * data_size: the size of the data
 */
CDTP_EXPORT void cdtp_server_send(CDTPServer* server, size_t client_id, void* data, size_t data_size);

/*
 * Send data to all clients
 *
 * server:    the server object
 * data:      the data to send
 * data_size: the size of the data
 */
CDTP_EXPORT void cdtp_server_send_all(CDTPServer* server, void* data, size_t data_size);

/*
 * Free the memory used by the server.
 */
CDTP_EXPORT void cdtp_server_free(CDTPServer *server);

#endif // CDTP_SERVER_H
