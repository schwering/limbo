// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * The value returned by set_add() and set_replace[_index]() is the index i at
 * which the new element is stored unless it was present before already; in
 * the latter case, 2*i-1 is returned. Use the macro ELEM_WAS_IN_SET to check
 * if the index indicates that the element was present before and
 * REAL_SET_INDEX to convert i or 2*i-1 to i, and UNREAL_SET_INDEX to convert i
 * or 2*i-1 to 2*i-1.
 * The most common purpose of these index computations is to adjust the loop
 * counting variable appropriately after such an action.
 */
#include "set.h"
#include "memory.h"
#include <assert.h>
#include <string.h>

#define MIN(x,y) ((x) < (y) ? (x) : (y))

#define COMPAR(set, left, right) \
    ((set)->compar != NULL ?\
     (set)->compar((left), (right)) :\
     (left) - (right))


#define ELEM_WAS_IN_SET(i)  ((i) < 0)
#define UNREAL_SET_INDEX(i) (ELEM_WAS_IN_SET(i) ? (i) : -1 * (i) - 1)
#define REAL_SET_INDEX(i)   (ELEM_WAS_IN_SET(i) ? -1 * ((i) + 1) : (i))

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
    // Let i be the index in range [lo,hi] where obj should be inserted so that
    // the order of the elements is monotone. If set does not yet contain obj, i
    // is returned; otherwise (-1 * i) - 1 is returned.
    // That is, negative values indicate that the value is already present, and
    // by adding 1 and multiplying it with -1 you get the index where the
    // element was stored.
    // XXX could use Knuth's uniform binary search to safe some comparisons
    assert(0 <= lo && hi <= vector_size(&set->vec) - 1);
    while (lo <= hi) {
        const int i = (lo + hi) / 2;
        const int cmp = COMPAR(set, obj, vector_get(&set->vec, i));
        if (cmp == 0) { // element already present
            return UNREAL_SET_INDEX(i);
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

bool set_is_lazy_copy(const set_t *set)
{
    return vector_is_lazy_copy(&set->vec);
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

int set_add(set_t *set, const void *elem)
{
    const int i = insert_pos(set, elem, 0, vector_size(&set->vec) - 1);
    if (i >= 0) {
        vector_insert(&set->vec, i, elem);
    }
    return i;
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
    int *indices = MALLOC(vector_size(&set->vec) * sizeof(int));
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
    return vector_remove(&set->vec, index);
}

void set_remove_index_range(set_t *set, int from, int to)
{
    vector_remove_range(&set->vec, from, to);
}

void set_remove_all_indices(set_t *set, const int indices[], int n_indices)
{
    vector_remove_all(&set->vec, indices, n_indices);
}

int set_replace(set_t *set, const void *old_elem, const void *new_elem)
{
    const int i = set_find(set, old_elem);
    if (i == -1) {
        return set_add(set, new_elem);
    } else {
        return set_replace_index(set, i, new_elem);
    }
}

int set_replace_index(set_t *set, int index, const void *new_elem)
{
    assert(0 <= index && index < vector_size(&set->vec));
    const int i = insert_pos(set, new_elem, 0, vector_size(&set->vec) - 1);
    const int j = REAL_SET_INDEX(i);
    if (ELEM_WAS_IN_SET(i)) {
        if (index != j) {
            vector_remove(&set->vec, index);
        }
        return UNREAL_SET_INDEX(index < j ? j-1 : j);
    } else if (j == index || j == index + 1) {
        vector_set(&set->vec, index, new_elem);
        return index;
    } else if (j < index) {
        // element to be removed comes after element to be inserted
        vector_remove(&set->vec, index);
        vector_insert(&set->vec, j, new_elem);
        return j;
    } else {
        // element to be removed comes before element to be inserted
        vector_remove(&set->vec, index);
        vector_insert(&set->vec, j - 1, new_elem);
        return j - 1;
    }
}

void set_clear(set_t *set)
{
    vector_clear(&set->vec);
}

set_iter_t set_iter(set_t *set, int index)
{
    return (set_iter_t) { .iter = vector_iter(&set->vec, index) };
}

bool set_iter_next(set_iter_t *iter)
{
    return vector_iter_next(&iter->iter);
}

const void *set_iter_get(const set_iter_t *iter)
{
    return vector_iter_get(&iter->iter);
}

int set_iter_index(const set_iter_t *iter)
{
    return vector_iter_index(&iter->iter);
}

void set_iter_add_auditor(set_iter_t *iter, set_iter_t *auditor)
{
    vector_iter_add_auditor(&iter->iter, &auditor->iter);
}

void set_iter_remove(set_iter_t *iter)
{
    vector_iter_remove(&iter->iter);
}

int set_iter_replace(set_iter_t *iter, const void *new_elem)
{
    set_t *set = (set_t *) iter->iter.i.vec;
    const int i = iter->iter.i.index;
    const int j = set_replace_index(set, i, new_elem);
    if (ELEM_WAS_IN_SET(j) || i < REAL_SET_INDEX(j)) {
        vector_iter_dispatch_removals(&iter->iter, i);
    }
    return j;
}

set_const_iter_t set_const_iter(const set_t *set, int index)
{
    return (set_const_iter_t) { .iter = vector_const_iter(&set->vec, index) };
}

bool set_const_iter_next(set_const_iter_t *iter)
{
    return vector_const_iter_next(&iter->iter);
}

const void *set_const_iter_get(const set_const_iter_t *iter)
{
    return vector_const_iter_get(&iter->iter);
}

