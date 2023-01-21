/**
 * CDTP client implementation.
 */

#pragma once
#ifndef CDTP_CLIENT_H
#define CDTP_CLIENT_H

#include "defs.h"
#include "util.h"
#include "crypto.h"
#include "threading.h"
#include "server.h"

/**
 * Instantiate a socket client.
 *
 * @param on_recv A pointer to a function that will be called when a message is received from the server.
 * @param on_disconnected A pointer to a function that will be called when the client is disconnected from the server.
 * @param on_recv_arg A value that will be passed to the `on_recv` event function.
 * @param on_disconnected_arg A value that will be passed to the `on_disconnected` event function.
 * @return The new socket client.
 *
 * The `on_recv` function should take four parameters:
 *   - a `CDTPClient *` representing the client itself
 *   - a `void *` representing the received data
 *   - a `size_t` representing the size of the received data, in bytes
 *   - a `void *` containing the `on_recv_arg`
 * Note that the data (the first parameter) is allocated on the heap by CDTP. Users are responsible for calling `free`
 * on the data at some point, whether that be at the end of the `on_recv` function or at a later time.
 *
 * The `on_disconnected` functions should take one parameter:
 *   - a `void *` containing the `on_disconnected_arg`
 *
 * All event functions are executed in their own threads to prevent halting the client's event loop.
 */
CDTP_EXPORT CDTPClient *cdtp_client(
  ClientOnRecvCallback on_recv,
  ClientOnDisconnectedCallback on_disconnected,
  void *on_recv_arg,
  void *on_disconnected_arg
);

/**
 * Connect to a server.
 *
 * @param client The socket client.
 * @param host The server host.
 * @param port The server port.
 */
CDTP_EXPORT void cdtp_client_connect(CDTPClient *client, char *host, unsigned short port);

/**
 * Disconnect from the server.
 *
 * @param client The socket client.
 */
CDTP_EXPORT void cdtp_client_disconnect(CDTPClient *client);

/**
 * Check if the client is connected to a server.
 *
 * @param client The socket client.
 * @return If the client is connected to a server.
 */
CDTP_EXPORT bool cdtp_client_is_connected(CDTPClient *client);

/**
 * Get the host of the client.
 *
 * @param client The socket client.
 * @return The host address of the client.
 *
 * Note that the returned value is allocated on the heap, and `free` will need to be called on it.
 */
CDTP_EXPORT char *cdtp_client_get_host(CDTPClient *client);

/**
 * Get the port of the client.
 *
 * @param client The socket client.
 * @return The port of the client.
 */
CDTP_EXPORT unsigned short cdtp_client_get_port(CDTPClient *client);

/**
 * The host of the server.
 *
 * @param client The socket client.
 * @return The host address of the server.
 *
 * Note that the returned value is allocated on the heap, and `free` will need to be called on it.
 */
CDTP_EXPORT char *cdtp_client_get_server_host(CDTPClient *client);

/**
 * Get the port of the server.
 *
 * @param client The socket client.
 * @return The port of the server.
 */
CDTP_EXPORT unsigned short cdtp_client_get_server_port(CDTPClient *client);

/**
 * Send data to the server.
 *
 * @param client The socket client.
 * @param data The data to send.
 * @param data_size The size of the data, in bytes.
 */
CDTP_EXPORT void cdtp_client_send(CDTPClient *client, void *data, size_t data_size);

/**
 * Free the memory used by the client.
 *
 * @param client The socket client.
 */
CDTP_EXPORT void cdtp_client_free(CDTPClient *client);

#endif // CDTP_CLIENT_H
