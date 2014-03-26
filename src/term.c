// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * schwering@kbsg.rwth-aachen.de
 */
#include "term.h"
#include <stdlib.h>

static int kv_compar(const kv_t *l, const kv_t *r)
{
    return (var_t) l->key - (var_t) r->key;
}

void varmap_init(varmap_t *map)
{
    map_init(map, kv_compar);
}

void varmap_free(varmap_t *map)
{
    map_free(map);
}

bool varmap_add(varmap_t *map, var_t var, stdname_t name)
{
    return map_add(map, (const void *) var, (const void *) name);
}

var_t varmap_add_replace(varmap_t *map, var_t var, stdname_t name)
{
    return (var_t) map_add_replace(map, (const void *) var,
            (const void *) name);
}

void stdvec_init(stdvec_t *vec)
{
    vector_init(vec);
}

void stdvec_copy(stdvec_t *dst, const stdvec_t *src)
{
    vector_copy(dst, src);
}

void stdvec_prepend_copy(stdvec_t *dst, stdname_t n, const stdvec_t *src)
{
    vector_init_with_size(dst, vector_size(src) + 1);
    vector_append(dst, (const void *) n);
    vector_append_all(dst, src);
}

void stdvec_copy_append(stdvec_t *dst, const stdvec_t *src, stdname_t n)
{
    vector_init_with_size(dst, vector_size(src) + 1);
    vector_append_all(dst, src);
    vector_append(dst, (const void *) n);
}

void stdvec_free(stdvec_t *vec)
{
    vector_free(vec);
}

stdname_t stdvec_get(const stdvec_t *vec, int index)
{
    return (stdname_t) vector_get(vec, index);
}

int stdvec_size(const stdvec_t *vec)
{
    return vector_size(vec);
}

int stdvec_cmp(const stdvec_t *vec1, const stdvec_t *vec2)
{
    return vector_cmp(vec1, vec2, NULL);
}
bool stdvec_eq(const stdvec_t *vec1, const stdvec_t *vec2)
{
    return vector_eq(vec1, vec2, NULL);
}

