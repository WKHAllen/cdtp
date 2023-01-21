/**
 * CDTP server implementation.
 */

#pragma once
#ifndef CDTP_SERVER_H
#define CDTP_SERVER_H

#include "defs.h"
#include "util.h"
#include "crypto.h"
#include "threading.h"
#include "map.h"

/**
 * Instantiate a socket server.
 *
 * @param on_recv A pointer to a function that will be called when a message is received from a client.
 * @param on_connect A pointer to a function that will be called when a client connects.
 * @param on_disconnect A pointer to a function that will be called when a client disconnects.
 * @param on_recv_arg A value that will be passed to the `on_recv` event function.
 * @param on_connect_arg A value that will be passed to the `on_connect` event function.
 * @param on_disconnect_arg A value that will be passed to the `on_disconnect` event function.
 * @return The new socket server.
 *
 * The `on_recv` function should take five parameters:
 *   - a `CDTPServer *` representing the server itself
 *   - a `size_t` representing the ID of the client that sent the message
 *   - a `void *` representing the received data
 *   - a `size_t` representing the size of the received data, in bytes
 *   - a `void *` containing the `on_recv_arg`
 * Note that the data (the second parameter) is allocated on the heap by CDTP. Users are responsible for calling `free`
 * on the data at some point, whether that be at the end of the `on_recv` function or at a later time.
 *
 * The `on_connect` and `on_disconnect` functions should each take two parameters:
 *   - a `size_t` representing the ID of the client that connected/disconnected
 *   - a `void *` containing the `on_connect_arg`/`on_disconnect_arg`
 *
 * All event functions are executed in their own threads to prevent halting the server's event loop.
 */
CDTP_EXPORT CDTPServer *cdtp_server(
  ServerOnRecvCallback on_recv,
  ServerOnConnectCallback on_connect,
  ServerOnDisconnectCallback on_disconnect,
  void *on_recv_arg,
  void *on_connect_arg,
  void *on_disconnect_arg
);

/**
 * Start the socket server.
 *
 * @param server The socket server.
 * @param host The address to host the server on.
 * @param port The port to host the server on.
 */
CDTP_EXPORT void cdtp_server_start(CDTPServer *server, char *host, unsigned short port);

/**
 * Stop the server.
 *
 * @param server The socket server.
 */
CDTP_EXPORT void cdtp_server_stop(CDTPServer *server);

/**
 * Check if the server is serving.
 *
 * @param server The socket server.
 * @return If the server is serving.
 */
CDTP_EXPORT bool cdtp_server_is_serving(CDTPServer *server);

/**
 * Get the host of the server.
 *
 * @param server The socket server.
 * @return The host address of the server.
 *
 * Note that the returned value is allocated on the heap, and `free` will need to be called on it.
 */
CDTP_EXPORT char *cdtp_server_get_host(CDTPServer *server);

/**
 * Get the port of the server.
 *
 * @param server The socket server.
 * @return The port of the server.
 */
CDTP_EXPORT unsigned short cdtp_server_get_port(CDTPServer *server);

/**
 * Get the host of a client.
 *
 * @param server The socket server.
 * @param client_id The ID of the client.
 * @return The host address of the client.
 *
 * Note that the returned value is allocated on the heap, and `free` will need to be called on it.
 */
CDTP_EXPORT char *cdtp_server_get_client_host(CDTPServer *server, size_t client_id);

/**
 * Get the port of a client.
 *
 * @param server The socket server.
 * @param client_id The ID of the client.
 * @return The port of the client.
 */
CDTP_EXPORT unsigned short cdtp_server_get_client_port(CDTPServer *server, size_t client_id);

/**
 * Disconnect a client from the server.
 *
 * @param server The socket server.
 * @param client_id The ID of the client to disconnect.
 */
CDTP_EXPORT void cdtp_server_remove_client(CDTPServer *server, size_t client_id);

/**
 * Send data to a client.
 *
 * @param server The socket server.
 * @param client_id The ID of the client to send the data to.
 * @param data The data to send.
 * @param data_size The size of the data, in bytes.
 */
CDTP_EXPORT void cdtp_server_send(CDTPServer *server, size_t client_id, void *data, size_t data_size);

/**
 * Send data to all clients.
 *
 * @param server The socket server.
 * @param data The data to send.
 * @param data_size The size of the data, in bytes.
 */
CDTP_EXPORT void cdtp_server_send_all(CDTPServer *server, void *data, size_t data_size);

/**
 * Free the memory used by the server.
 *
 * @param server The socket server.
 */
CDTP_EXPORT void cdtp_server_free(CDTPServer *server);

#endif // CDTP_SERVER_H
