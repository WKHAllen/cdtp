/*
 * Map interfaces to track clients within a server.
 */

#pragma once
#ifndef CDTP_MAP_H
#define CDTP_MAP_H

#include "defs.h"

#define CDTP_MAP_MIN_CAPACITY 16
#define CDTP_MAP_START_CAPACITY CDTP_MAP_MIN_CAPACITY

// Create a new client map
CDTP_TEST_EXPORT CDTPClientMap *_cdtp_client_map(void);

// Check if a client map contains a given key
CDTP_TEST_EXPORT int _cdtp_client_map_contains(CDTPClientMap *map, size_t client_id);

// Get the client at a given key
CDTP_TEST_EXPORT CDTPSocket *_cdtp_client_map_get(CDTPClientMap *map, size_t client_id);

// Set the client at a given key, given that the key is not already in use
CDTP_TEST_EXPORT int _cdtp_client_map_set(CDTPClientMap *map, size_t client_id, CDTPSocket *sock);

// Pop a client from the map by key
CDTP_TEST_EXPORT CDTPSocket *_cdtp_client_map_pop(CDTPClientMap *map, size_t client_id);

// Create an iterator over the client map
CDTP_TEST_EXPORT CDTPClientMapIter *_cdtp_client_map_iter(CDTPClientMap *map);

// Free the memory used by a client map iterator
CDTP_TEST_EXPORT void _cdtp_client_map_iter_free(CDTPClientMapIter *iter);

// Free the memory used by a client map
CDTP_TEST_EXPORT void _cdtp_client_map_free(CDTPClientMap *map);

#endif // CDTP_MAP_H
