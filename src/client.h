/*
 * Client types and functions for cdtp.
 */

#pragma once
#ifndef CDTP_CLIENT_H
#define CDTP_CLIENT_H

#include "defs.h"
#include "util.h"
#include "threading.h"
#include "server.h"

/*
  * Client creation/initialization
  *
  * on_recv:             pointer to a function that will be called when a packet is received
  * on_connect:          pointer to a function that will be called when the client is disconnected
  * on_recv_arg:         a value that will be passed to the on_recv function
  * on_disconnected_arg: a value that will be passed to the on_disconnected function
  */
EXPORT CDTPClient* cdtp_client(
  ClientOnRecvCallback on_recv,
  ClientOnDisconnectedCallback on_disconnected,
  void* on_recv_arg,
  void* on_disconnected_arg
);

/*
 * Connect to a server
 *
 * client: the client object
 * host:   the host as a string
 * port:   the port as an integer
 */
EXPORT void cdtp_client_connect(CDTPClient* client, char* host, unsigned short port);

/*
 * Disconnect from the server and free up memory
 *
 * client: the client object
 */
EXPORT void cdtp_client_disconnect(CDTPClient* client);

/*
 * Check if the client is connected to a server
 *
 * client: the client object
 */
EXPORT int cdtp_client_connected(CDTPClient* client);

/*
 * Get the client host address.
 *
 * client: the client object
 *
 * The returned value's memory will need to be freed after use
 */
EXPORT char* cdtp_client_get_host(CDTPClient* client);

/*
 * Get the client port.
 *
 * client: the client object
 */
EXPORT unsigned short cdtp_client_get_port(CDTPClient* client);

/*
 * Send data to the server
 *
 * client:    the client object
 * data:      the data to send
 * data_size: the size of the data
 */
EXPORT void cdtp_client_send(CDTPClient* client, void* data, size_t data_size);

#endif // CDTP_CLIENT_H
