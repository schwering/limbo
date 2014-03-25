// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * A map structure based on set.h.
 * The comparison function shall only use the kv_t's key attribute.
 *
 * Most functions have a corresponding counterpart in map.h, for example,
 * map_init[_with_size](), map_free(), map_get(), map_size(), map_clear().
 *
 * map_find(), map_contains(), map_lookup() operate on keys instead of indices.
 * map_lookup() returns the value corrensponding to the key.
 *
 * map_add[_replace]() insert a new key/value pair. While map_add() returns
 * true iff the key was not present before, map_add_replace() overrides the
 * old one if necessary and returns the old value or NULL.
 *
 * map_remove() also returns the old value if the given key is present.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#ifndef _MAP_H_
#define _MAP_H_

#include "set.h"

typedef struct {
    set_t set;
} map_t;

typedef struct {
    const void *key;
    const void *val;
} kv_t;

typedef int (*kv_compar_t)(const kv_t *, const kv_t *);

void map_init(map_t *map, kv_compar_t compar);
void map_init_with_size(map_t *map, kv_compar_t compar, int size);
void map_free(map_t *map);

int map_find(const map_t *map, const void *key);
bool map_contains(const map_t *map, const void *key);
const kv_t *map_get(const map_t *map, int index);
const void *map_lookup(const map_t *map, const void *key);
int map_size(const map_t *map);

bool map_add(map_t *map, const void *key, const void *val);
const void *map_add_replace(map_t *map, const void *key, const void *val);

const void *map_remove(map_t *map, void *key);
void map_clear(map_t *map);

#endif

