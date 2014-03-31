// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * A map structure based on set.h.
 * The comparison function shall only use the kv_t's key attribute.
 *
 * Most functions have a corresponding counterpart in map.h, for example,
 * map_init[_with_size](), map_free(), map_get(), map_size(), map_clear().
 *
 * map_find(), map_contains(), map_lookup() operate on keys instead of indices.
 * map_lookup() returns the value corrensponding to the key or NULL if it is not
 * present.
 *
 * map_add[_replace]() insert a new key/value pair. While map_add() returns
 * true iff the key was not present before, map_add_replace() overrides the
 * old one if necessary and returns the old value or NULL.
 *
 * map_remove() also returns the old value if the given key is present.
 *
 * For typesafe usage there is a MAP macro which declares appropriate wrapper
 * functions.
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

map_t map_init(kv_compar_t compar);
map_t map_init_with_size(kv_compar_t compar, int size);
void map_free(map_t *map);

int map_find(const map_t *map, const void *key);
bool map_contains(const map_t *map, const void *key);
const kv_t *map_get(const map_t *map, int index);
const void *map_lookup(const map_t *map, const void *key);
int map_size(const map_t *map);

bool map_add(map_t *map, const void *key, const void *val);
const void *map_add_replace(map_t *map, const void *key, const void *val);

const void *map_remove(map_t *map, const void *key);
void map_clear(map_t *map);

#define MAP_DECL(prefix, keytype, valtype) \
    typedef union { map_t m; } prefix##_t;\
    typedef struct { keytype key; valtype val; } prefix##_kv_t;\
    prefix##_t prefix##_init(void);\
    prefix##_t prefix##_init_with_size(int size);\
    void prefix##_free(prefix##_t *m);\
    int prefix##_find(const prefix##_t *m, const keytype key);\
    bool prefix##_contains(const prefix##_t *m, const keytype key);\
    const prefix##_kv_t *prefix##_get(const prefix##_t *m, int index);\
    const valtype prefix##_lookup(const prefix##_t *m, const keytype key);\
    int prefix##_size(const prefix##_t *m);\
    bool prefix##_add(prefix##_t *m, const keytype key, const valtype val);\
    const valtype prefix##_add_replace(prefix##_t *m,\
            const keytype key, const valtype val);\
    const valtype prefix##_remove(prefix##_t *m, const keytype key);\
    void prefix##_clear(prefix##_t *m);

#define MAP_IMPL(prefix, keytype, valtype, keycompar) \
    static inline int prefix##_kv_compar(const kv_t *l, const kv_t *r) {\
        return keycompar((const keytype) l->key, (const keytype) r->key); }\
    prefix##_t prefix##_init(void) {\
        return (prefix##_t) { .m = map_init(prefix##_kv_compar) }; }\
    prefix##_t prefix##_init_with_size(int size) {\
        return (prefix##_t) { .m = map_init_with_size(prefix##_kv_compar,\
                size) }; }\
    void prefix##_free(prefix##_t *m) {\
        map_free(&m->m); }\
    int prefix##_find(const prefix##_t *m, const keytype key) {\
        return map_find(&m->m, (const void *) key); }\
    bool prefix##_contains(const prefix##_t *m, const keytype key) {\
        return map_contains(&m->m, (const void *) key); }\
    const prefix##_kv_t *prefix##_get(const prefix##_t *m, int index) {\
        return (prefix##_kv_t *) map_get(&m->m, index); }\
    const valtype prefix##_lookup(const prefix##_t *m, const keytype key) {\
        return (const valtype) map_lookup(&m->m, (const void *) key); }\
    int prefix##_size(const prefix##_t *m) {\
        return map_size(&m->m); }\
    bool prefix##_add(prefix##_t *m, const keytype key, const valtype val) {\
        return map_add(&m->m, (const void *) key, (const void *) val); }\
    const valtype prefix##_add_replace(prefix##_t *m,\
            const keytype key, const valtype val) {\
        return (const valtype) map_add_replace(&m->m,\
                (const void *) key, (const void *) val); }\
    const valtype prefix##_remove(prefix##_t *m, const keytype key) {\
        return (const valtype) map_remove(&m->m, (const void *) key); }\
    void prefix##_clear(prefix##_t *m) {\
        map_clear(&m->m); }

#endif

