// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "set.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define MIN(x,y) ((x) < (y) ? (x) : (y))

#define COMPAR(set, left, right) \
    ((set)->compar != NULL ?\
     (set)->compar((left), (right)) :\
     (left) - (right))

static inline int search(const set_t *set, const void *obj, int lo, int hi)
{
    // XXX could use Knuth's uniform binary search to safe some comparisons
    assert(0 <= lo && hi <= vector_size(&set->vec) - 1);
    while (lo <= hi) {
        const int i = (lo + hi) / 2;
        const int cmp = COMPAR(set, obj, vector_get(&set->vec, i));
        if (cmp == 0) { // found
            return i;
        } else if (cmp < 0) { // left half
            hi = i - 1;
        } else { // right half
            lo = i + 1;
        }
    }
    return -1;
}

static inline int insert_pos(const set_t *set, const void *obj, int lo, int hi)
{
    // XXX could use Knuth's uniform binary search to safe some comparisons
    assert(0 <= lo && hi <= vector_size(&set->vec) - 1);
    while (lo <= hi) {
        const int i = (lo + hi) / 2;
        const int cmp = COMPAR(set, obj, vector_get(&set->vec, i));
        if (cmp == 0) { // element already present
            return -1;
        } else if (cmp <= 0) { // left half
            if (i == 0 || COMPAR(set, obj, vector_get(&set->vec, i-1)) > 0) {
                // position found
                return i;
            } else { // left half
                hi = i - 1;
            }
        } else { // right half
            lo = i + 1;
        }
    }
    return vector_size(&set->vec);
}

set_t set_init(compar_t compar)
{
    return (set_t) {
        .vec = vector_init(),
        .compar = compar
    };
}

set_t set_init_with_size(compar_t compar, int size)
{
    return (set_t) {
        .vec = vector_init_with_size(size),
        .compar = compar
    };
}

set_t set_copy(const set_t *src)
{
    return (set_t) {
        .vec = vector_copy(&src->vec),
        .compar = src->compar
    };
}

set_t set_lazy_copy(const set_t *src)
{
    return (set_t) {
        .vec = vector_lazy_copy(&src->vec),
        .compar = src->compar
    };
}

set_t set_singleton(compar_t compar, const void *elem)
{
    set_t set = set_init_with_size(compar, 1);
    set_add(&set, elem);
    return set;
}

set_t set_union(const set_t *l, const set_t *r)
{
    assert(l->compar == r->compar);
    set_t set = set_init_with_size(l->compar, set_size(l) + set_size(r));
    int i = 0;
    int j = 0;
    while (i < vector_size(&l->vec) && j < vector_size(&r->vec)) {
        const int cmp = COMPAR(l,
                vector_get(&l->vec, i),
                vector_get(&r->vec, j));
        if (cmp < 0) {
            vector_append(&set.vec, vector_get(&l->vec, i));
            ++i;
        } else if (cmp > 0) {
            vector_append(&set.vec, vector_get(&r->vec, j));
            ++j;
        } else {
            vector_append(&set.vec, vector_get(&l->vec, i));
            ++i;
            ++j;
        }
    }
    vector_append_all_range(&set.vec, &l->vec, i, vector_size(&l->vec));
    vector_append_all_range(&set.vec, &r->vec, j, vector_size(&r->vec));
    return set;
}

set_t set_difference(const set_t *l, const set_t *r)
{
    assert(l->compar == r->compar);
    set_t set = set_init_with_size(l->compar, set_size(l));
    for (int i = 0, j = 0; i < vector_size(&l->vec); ++i) {
        const void *e = vector_get(&l->vec, i);
        const int k = search(r, e, j, vector_size(&r->vec) - 1);
        if (k == -1) {
            vector_append(&set.vec, vector_get(&l->vec, i));
        } else {
            j = k + 1;
        }
    }
    return set;
}

set_t set_intersection(const set_t *l, const set_t *r)
{
    assert(l->compar == r->compar);
    set_t set = set_init_with_size(l->compar, MIN(set_size(l), set_size(r)));
    int i = 0;
    int j = 0;
    while (i < vector_size(&l->vec) && j < vector_size(&r->vec)) {
        const int cmp = COMPAR(l,
                vector_get(&l->vec, i),
                vector_get(&r->vec, j));
        if (cmp < 0) {
            ++i;
        } else if (cmp > 0) {
            ++j;
        } else {
            vector_append(&set.vec, vector_get(&l->vec, i));
            ++i;
            ++j;
        }
    }
    return set;
}

void set_cleanup(set_t *set)
{
    vector_cleanup(&set->vec);
}

int set_cmp(const set_t *set1, const set_t *set2)
{
    assert(set1->compar == set2->compar);
    return vector_cmp(&set1->vec, &set2->vec, set1->compar);
}

bool set_eq(const set_t *set1, const set_t *set2)
{
    assert(set1->compar == set2->compar);
    return vector_eq(&set1->vec, &set2->vec, set1->compar);
}

const void *set_get(const set_t *set, int index)
{
    return vector_get(&set->vec, index);
}

void *set_get_unsafe(set_t *set, int index)
{
    return (void *) set_get(set, index);
}

const void **set_array(const set_t *set)
{
    return vector_array(&set->vec);
}

int set_size(const set_t *set)
{
    return vector_size(&set->vec);
}

int set_find(const set_t *set, const void *elem)
{
    return search(set, elem, 0, vector_size(&set->vec) - 1);
}

bool set_contains(const set_t *set, const void *elem)
{
    return set_find(set, elem) != -1;
}

bool set_contains_all(const set_t *set, const set_t *elems)
{
    assert(set->compar == elems->compar);
    if (set_size(set) < set_size(elems)) {
        return false;
    }
    for (int i = 0, j = 0; i < vector_size(&elems->vec); ++i) {
        const void *e = vector_get(&elems->vec, i);
        const int k = search(set, e, j, vector_size(&set->vec) - 1);
        if (k == -1) {
            return false;
        } else {
            j = k + 1;
        }
    }
    return true;
}

bool set_add(set_t *set, const void *elem)
{
    const int i = insert_pos(set, elem, 0, vector_size(&set->vec) - 1);
    if (i != -1) {
        vector_insert(&set->vec, i, elem);
        return true;
    } else {
        return false;
    }
}

void set_add_all(set_t *set, const set_t *elems)
{
    for (int i = 0; i < set_size(elems); ++i) {
        set_add(set, set_get(elems, i));
    }
}

bool set_remove(set_t *set, const void *elem)
{
    const int i = set_find(set, elem);
    if (i != -1) {
        vector_remove(&set->vec, i);
        return true;
    } else {
        return false;
    }
}

void set_remove_all(set_t *set, const set_t *elems)
{
    assert(set->compar == elems->compar);
    int *indices = malloc(vector_size(&set->vec) * sizeof(int));
    int n_indices = 0;
    for (int i = 0, j = 0; i < vector_size(&set->vec); ++i) {
        const void *e = vector_get(&set->vec, i);
        const int k = search(elems, e, j, vector_size(&elems->vec) - 1);
        if (k != -1) {
            indices[n_indices] = i;
            ++n_indices;
            j = k + 1;
        }
    }
    vector_remove_all(&set->vec, indices, n_indices);
}

const void *set_remove_index(set_t *set, int index)
{
    const void *elem = vector_get(&set->vec, index);
    vector_remove(&set->vec, index);
    return elem;
}

void set_remove_all_indices(set_t *set, const int indices[], int n_indices)
{
    vector_remove_all(&set->vec, indices, n_indices);
}

void set_clear(set_t *set)
{
    vector_clear(&set->vec);
}

