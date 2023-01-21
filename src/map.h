/**
 * CDTP interfaces to dynamically keep track of clients within a server.
 */

#pragma once
#ifndef CDTP_MAP_H
#define CDTP_MAP_H

#include "defs.h"
#include <stdbool.h>

/**
 * Create a new client map.
 *
 * @return The new client map.
 */
CDTP_TEST_EXPORT CDTPClientMap *_cdtp_client_map(void);

/**
 * Check if a client map contains a given key.
 *
 * @param map The client map.
 * @param client_id The client ID key.
 * @return If the map contains the given client ID key.
 */
CDTP_TEST_EXPORT bool _cdtp_client_map_contains(CDTPClientMap *map, size_t client_id);

/**
 * Get the client with a given key.
 *
 * @param map The client map.
 * @param client_id The client ID key.
 * @return The client socket, or NULL if the key does not exist in the map.
 */
CDTP_TEST_EXPORT CDTPSocket *_cdtp_client_map_get(CDTPClientMap *map, size_t client_id);

/**
 * Set the client at a given key, given that the key is not already in use.
 *
 * @param map The client map.
 * @param client_id The client ID key.
 * @param sock The client socket.
 * @return If the value was successfully inserted. Returns false if the key already exists in the map.
 */
CDTP_TEST_EXPORT bool _cdtp_client_map_set(CDTPClientMap *map, size_t client_id, CDTPSocket *sock);

/**
 * Pop a client from the map by key.
 *
 * @param map The client map.
 * @param client_id The client ID key.
 * @return The client socket, or NULL if the key does not exist in the map.
 *
 * Note that the client socket is returned, and the memory is still valid (`free` has not been called).
 */
CDTP_TEST_EXPORT CDTPSocket *_cdtp_client_map_pop(CDTPClientMap *map, size_t client_id);

/**
 * Create an iterator over the client map.
 *
 * @param map The client map.
 * @return An iterator over the clients in the map.
 */
CDTP_TEST_EXPORT CDTPClientMapIter *_cdtp_client_map_iter(CDTPClientMap *map);

/**
 * Free the memory used by a client map iterator.
 *
 * @param iter The client map iterator.
 */
CDTP_TEST_EXPORT void _cdtp_client_map_iter_free(CDTPClientMapIter *iter);

/**
 * Free the memory used by a client map.
 *
 * @param map The client map.
 */
CDTP_TEST_EXPORT void _cdtp_client_map_free(CDTPClientMap *map);

#endif // CDTP_MAP_H
