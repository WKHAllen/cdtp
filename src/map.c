#include "map.h"

CDTPClientMapNode *_cdtp_client_map_node(void)
{
    CDTPClientMapNode *node = (CDTPClientMapNode *) malloc(sizeof(CDTPClientMapNode));

    node->allocated = CDTP_FALSE;
    node->client_id = SIZE_MAX;
    node->sock = NULL;

    return node;
}

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

size_t _cdtp_client_map_hash(CDTPClientMap *map, size_t key)
{
    return key % (map->capacity);
}

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
        if (old_nodes[i]->allocated == CDTP_TRUE) {
            size_t hash = _cdtp_client_map_hash(map, old_nodes[i]->client_id);

            while (map->nodes[hash]->allocated == CDTP_TRUE) {
                hash = (hash + 1) % (map->capacity);
            }

            map->nodes[hash]->allocated = CDTP_TRUE;
            map->nodes[hash]->client_id = old_nodes[i]->client_id;
            map->nodes[hash]->sock = old_nodes[i]->sock;
        }
    }

    free(old_nodes);
}

void _cdtp_client_map_resize_up(CDTPClientMap *map)
{
    _cdtp_client_map_resize(map, map->capacity * 2);
}

void _cdtp_client_map_resize_down(CDTPClientMap *map)
{
    _cdtp_client_map_resize(map, map->capacity / 2);
}

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

CDTP_TEST_EXPORT int _cdtp_client_map_contains(CDTPClientMap *map, size_t client_id)
{
    size_t hash = _cdtp_client_map_hash(map, client_id);
    size_t original_hash = hash;

    do {
        if (map->nodes[hash]->allocated == CDTP_TRUE && map->nodes[hash]->client_id == client_id) {
            return CDTP_TRUE;
        }

        hash = (hash + 1) % (map->capacity);
    } while (hash != original_hash);

    return CDTP_FALSE;
}

CDTP_TEST_EXPORT CDTPSocket *_cdtp_client_map_get(CDTPClientMap *map, size_t client_id)
{
    size_t hash = _cdtp_client_map_hash(map, client_id);
    size_t original_hash = hash;

    do {
        if (map->nodes[hash]->allocated == CDTP_TRUE && map->nodes[hash]->client_id == client_id) {
            return map->nodes[hash]->sock;
        }

        hash = (hash + 1) % (map->capacity);
    } while (hash != original_hash);

    return NULL;
}

CDTP_TEST_EXPORT int _cdtp_client_map_set(CDTPClientMap *map, size_t client_id, CDTPSocket *sock)
{
    if (_cdtp_client_map_contains(map, client_id) == CDTP_TRUE) {
        return CDTP_FALSE;
    }

    _cdtp_client_map_try_resize(map);

    size_t hash = _cdtp_client_map_hash(map, client_id);

    while (map->nodes[hash]->allocated == CDTP_TRUE) {
        hash = (hash + 1) % (map->capacity);
    }

    map->nodes[hash]->allocated = CDTP_TRUE;
    map->nodes[hash]->client_id = client_id;
    map->nodes[hash]->sock = sock;

    map->size++;

    return CDTP_TRUE;
}

CDTP_TEST_EXPORT CDTPSocket *_cdtp_client_map_pop(CDTPClientMap *map, size_t client_id)
{
    _cdtp_client_map_try_resize(map);

    size_t hash = _cdtp_client_map_hash(map, client_id);
    size_t original_hash = hash;

    do {
        if (map->nodes[hash]->allocated == CDTP_TRUE && map->nodes[hash]->client_id == client_id) {
            CDTPSocket *sock = map->nodes[hash]->sock;

            map->nodes[hash]->allocated = CDTP_FALSE;
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
        if (map->nodes[i]->allocated == CDTP_TRUE) {
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
