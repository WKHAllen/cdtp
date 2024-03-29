#include "map.h"

// Minimum capacity of a client map.
#define CDTP_MAP_MIN_CAPACITY 16

// Starting capacity of a client map.
#define CDTP_MAP_START_CAPACITY CDTP_MAP_MIN_CAPACITY

/**
 * Allocate a new empty node for the client map.
 *
 * @return The new node.
 */
CDTPClientMapNode *_cdtp_client_map_node(void)
{
    CDTPClientMapNode *node = (CDTPClientMapNode *) malloc(sizeof(CDTPClientMapNode));

    node->allocated = false;
    node->client_id = SIZE_MAX;
    node->sock = NULL;

    return node;
}

/**
 * Free the memory used by a client map node.
 *
 * @param node The node.
 */
void _cdtp_client_map_node_free(CDTPClientMapNode *node)
{
    free(node);
}

CDTP_TEST_EXPORT CDTPClientMap *_cdtp_client_map(void)
{
    CDTPClientMap *map = (CDTPClientMap *) malloc(sizeof(CDTPClientMap));

    map->size = 0;
    map->capacity = CDTP_MAP_START_CAPACITY;
    map->nodes = (CDTPClientMapNode **) malloc((map->capacity) * sizeof(CDTPClientMapNode *));

    for (size_t i = 0; i < map->capacity; i++) {
        map->nodes[i] = _cdtp_client_map_node();
    }

    return map;
}

/**
 * Get the hash for a given key.
 *
 * @param map The client map.
 * @param key The key.
 * @return The key's hash.
 */
size_t _cdtp_client_map_hash(CDTPClientMap *map, size_t key)
{
    return key % (map->capacity);
}

/**
 * Resize a map and rehash its contents.
 *
 * @param map The client map.
 * @param new_capacity The new capacity of the map.
 */
void _cdtp_client_map_resize(CDTPClientMap *map, size_t new_capacity)
{
    size_t old_capacity = map->capacity;
    CDTPClientMapNode **old_nodes = (CDTPClientMapNode **) realloc(map->nodes, old_capacity * sizeof(CDTPClientMapNode *));
    map->capacity = new_capacity;
    map->nodes = (CDTPClientMapNode **) malloc((map->capacity) * sizeof(CDTPClientMapNode *));

    for (size_t i = 0; i < map->capacity; i++) {
        map->nodes[i] = _cdtp_client_map_node();
    }

    for (size_t i = 0; i < old_capacity; i++) {
        if (old_nodes[i]->allocated) {
            size_t hash = _cdtp_client_map_hash(map, old_nodes[i]->client_id);

            while (map->nodes[hash]->allocated) {
                hash = (hash + 1) % (map->capacity);
            }

            map->nodes[hash]->allocated = true;
            map->nodes[hash]->client_id = old_nodes[i]->client_id;
            map->nodes[hash]->sock = old_nodes[i]->sock;
        }
    }

    free(old_nodes);
}

/**
 * Increase the capacity of a client map.
 *
 * @param map The client map.
 */
void _cdtp_client_map_resize_up(CDTPClientMap *map)
{
    _cdtp_client_map_resize(map, map->capacity * 2);
}

/**
 * Decrease the capacity of a client map.
 *
 * @param map The client map.
 */
void _cdtp_client_map_resize_down(CDTPClientMap *map)
{
    _cdtp_client_map_resize(map, map->capacity / 2);
}

/**
 * Attempt to increase or decrease the capacity of the map, depending on its current size and capacity.
 *
 * @param map The client map.
 * @return -1 if the capacity decreased, 1 if the capacity increased, or 0 if nothing changed.
 */
int _cdtp_client_map_try_resize(CDTPClientMap *map)
{
    if (map->size >= map->capacity) {
        _cdtp_client_map_resize_up(map);
        return 1;
    } else if (map->capacity > CDTP_MAP_MIN_CAPACITY && map->size * 4 <= map->capacity) {
        _cdtp_client_map_resize_down(map);
        return -1;
    } else {
        return 0;
    }
}

CDTP_TEST_EXPORT bool _cdtp_client_map_contains(CDTPClientMap *map, size_t client_id)
{
    size_t hash = _cdtp_client_map_hash(map, client_id);
    size_t original_hash = hash;

    do {
        if (map->nodes[hash]->allocated && map->nodes[hash]->client_id == client_id) {
            return true;
        }

        hash = (hash + 1) % (map->capacity);
    } while (hash != original_hash);

    return false;
}

CDTP_TEST_EXPORT CDTPSocket *_cdtp_client_map_get(CDTPClientMap *map, size_t client_id)
{
    size_t hash = _cdtp_client_map_hash(map, client_id);
    size_t original_hash = hash;

    do {
        if (map->nodes[hash]->allocated && map->nodes[hash]->client_id == client_id) {
            return map->nodes[hash]->sock;
        }

        hash = (hash + 1) % (map->capacity);
    } while (hash != original_hash);

    return NULL;
}

CDTP_TEST_EXPORT bool _cdtp_client_map_set(CDTPClientMap *map, size_t client_id, CDTPSocket *sock)
{
    if (_cdtp_client_map_contains(map, client_id)) {
        return false;
    }

    _cdtp_client_map_try_resize(map);

    size_t hash = _cdtp_client_map_hash(map, client_id);

    while (map->nodes[hash]->allocated) {
        hash = (hash + 1) % (map->capacity);
    }

    map->nodes[hash]->allocated = true;
    map->nodes[hash]->client_id = client_id;
    map->nodes[hash]->sock = sock;

    map->size++;

    return true;
}

CDTP_TEST_EXPORT CDTPSocket *_cdtp_client_map_pop(CDTPClientMap *map, size_t client_id)
{
    _cdtp_client_map_try_resize(map);

    size_t hash = _cdtp_client_map_hash(map, client_id);
    size_t original_hash = hash;

    do {
        if (map->nodes[hash]->allocated && map->nodes[hash]->client_id == client_id) {
            CDTPSocket *sock = map->nodes[hash]->sock;

            map->nodes[hash]->allocated = false;
            map->nodes[hash]->client_id = SIZE_MAX;
            map->nodes[hash]->sock = NULL;

            map->size--;

            return sock;
        }

        hash = (hash + 1) % (map->capacity);
    } while (hash != original_hash);

    return NULL;
}

CDTP_TEST_EXPORT CDTPClientMapIter *_cdtp_client_map_iter(CDTPClientMap *map)
{
    CDTPClientMapIter *iter = (CDTPClientMapIter *) malloc(sizeof(CDTPClientMapIter));

    iter->size = map->size;
    iter->clients = (CDTPClientMapIterNode **) malloc((iter->size) * sizeof(CDTPClientMapIterNode *));

    size_t iter_index = 0;

    for (size_t i = 0; i < map->capacity; i++) {
        if (map->nodes[i]->allocated) {
            CDTPClientMapIterNode *iter_node = (CDTPClientMapIterNode *) malloc(sizeof(CDTPClientMapIterNode));

            iter_node->client_id = map->nodes[i]->client_id;
            iter_node->sock = map->nodes[i]->sock;

            iter->clients[iter_index++] = iter_node;
        }
    }

    return iter;
}

CDTP_TEST_EXPORT void _cdtp_client_map_iter_free(CDTPClientMapIter *iter)
{
    for (size_t i = 0; i < iter->size; i++) {
        free(iter->clients[i]);
    }

    free(iter->clients);
    free(iter);
}

CDTP_TEST_EXPORT void _cdtp_client_map_free(CDTPClientMap *map)
{
    for (size_t i = 0; i < map->capacity; i++) {
        _cdtp_client_map_node_free(map->nodes[i]);
    }

    free(map->nodes);
    free(map);
}
