// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "map.h"
#include <stdlib.h>

map_t map_init(kv_compar_t compar)
{
    return (map_t) { .set = set_init((compar_t) compar) };
}

map_t map_init_with_size(kv_compar_t compar, int size)
{
    return (map_t) { .set = set_init_with_size((compar_t) compar, size) };
}

void map_free(map_t *map)
{
    for (int i = 0; i < set_size(&map->set); ++i) {
        free((void *) set_get(&map->set, i));
    }
    set_free(&map->set);
}

int map_find(const map_t *map, const void *key)
{
    const kv_t kv = { .key = key, .val = NULL };
    return set_find(&map->set, &kv);
}

bool map_contains(const map_t *map, const void *key)
{
    return map_find(map, key) != -1;
}

const kv_t *map_get(const map_t *map, int index)
{
    return set_get(&map->set, index);
}

const void *map_lookup(const map_t *map, const void *key)
{
    const kv_t kv = { .key = key, .val = NULL };
    const int i = set_find(&map->set, &kv);
    if (i == -1) {
        return NULL;
    }
    const kv_t *kvp = set_get(&map->set, i);
    return kvp->val;
}

int map_size(const map_t *map)
{
    return set_size(&map->set);
}

bool map_add(map_t *map, const void *key, const void *val)
{
    kv_t *kvp = malloc(sizeof(kv_t));
    kvp->key = key;
    kvp->val = val;
    return set_add(&map->set, kvp);
}

const void *map_add_replace(map_t *map, const void *key, const void *val)
{
    const int i = map_find(map, key);
    if (i != -1) {
        const void *old_val = NULL;
        const kv_t *kvp = (const kv_t *) set_get(&map->set, i);
        old_val = kvp->val;
        ((kv_t *) kvp)->val = val;
        return old_val;
    } else {
        kv_t *kvp = malloc(sizeof(kv_t));
        kvp->key = key;
        kvp->val = val;
        set_add(&map->set, kvp);
        return NULL;
    }
}

const void *map_remove(map_t *map, const void *key)
{
    const int i = map_find(map, key);
    if (i != -1) {
        const kv_t *kvp = set_remove_index(&map->set, i);
        const void *old_val = kvp->val;
        free((kv_t *) kvp);
        return old_val;
    } else {
        return NULL;
    }
}

void map_clear(map_t *map)
{
    set_clear(&map->set);
}

