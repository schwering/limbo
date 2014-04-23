// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * A negative value of vector_t's `capacity' attribute indicates that the
 * object does not own the memory of its `array' attribute.
 */
#include "vector.h"
#include "memory.h"
#include <assert.h>
#include <string.h>

#define RESIZE_FACTOR 2
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

vector_t vector_init(void)
{
    return vector_init_with_size(4);
}

vector_t vector_init_with_size(int size)
{
    assert(size >= 0);
    return (vector_t) {
        .array = MALLOC(size * sizeof(void *)),
        .capacity = size,
        .size = 0,
    };
}

vector_t vector_copy(const vector_t *src)
{
    return vector_copy_range(src, 0, src->size);
}

vector_t vector_copy_range(const vector_t *src, int from, int to)
{
    const int n_new_elems = to - from;
    vector_t dst = vector_init_with_size(n_new_elems);
    memcpy(dst.array,
            src->array + from,
            n_new_elems * sizeof(void *));
    dst.size = n_new_elems;
    return dst;
}

vector_t vector_lazy_copy(const vector_t *src)
{
    return vector_lazy_copy_range(src, 0, src->size);
}

vector_t vector_lazy_copy_range(const vector_t *src, int from, int to)
{
    const int n_new_elems = to - from;
    vector_t dst = {
        .array = src->array + sizeof(void *) * from,
        .capacity = -1,
        .size = n_new_elems
    };
    return dst;
}

vector_t vector_prepend_copy(const void *elem, const vector_t *src)
{
    vector_t dst = vector_init_with_size(vector_size(src) + 1);
    vector_append(&dst, elem);
    vector_append_all(&dst, src);
    return dst;
}

vector_t vector_copy_append(const vector_t *src, const void *elem)
{
    vector_t dst = vector_init_with_size(vector_size(src) + 1);
    vector_append_all(&dst, src);
    vector_append(&dst, elem);
    return dst;
}

vector_t vector_singleton(const void *e)
{
    vector_t dst = vector_init_with_size(1);
    vector_append(&dst, e);
    return dst;
}

vector_t vector_concat(const vector_t *vec1, const vector_t *vec2)
{
    vector_t vec = vector_init_with_size(vector_size(vec1) + vector_size(vec2));
    vector_append_all(&vec, vec1);
    vector_append_all(&vec, vec2);
    return vec;
}

vector_t vector_from_array(const void *array[], int n)
{
    vector_t dst = vector_init_with_size(n);
    memcpy(dst.array,
            array,
            n * sizeof(void *));
    dst.size = n;
    return dst;
}

void vector_cleanup(vector_t *vec)
{
    if (vec->capacity >= 0 && vec->array != NULL) {
#ifndef NDEBUG
        memset(vec->array, 0, vec->capacity * sizeof(void *));
#endif
        FREE(vec->array);
    }
    vec->array = NULL;
    vec->size = 0;
    vec->capacity = 0;
}

bool vector_is_lazy_copy(const vector_t *vec)
{
    return vec->capacity < 0;
}

static void alloc_if_lazy(vector_t *vec)
{
    if (vec->capacity < 0) {
        void const **array = vec->array;
        vec->capacity = vec->size;
        vec->array = MALLOC(vec->capacity * sizeof(void *));
        memcpy(vec->array, array, vec->size * sizeof(void *));
    }
}

const void *vector_get(const vector_t *vec, int index)
{
    assert(0 <= index && index < vec->size);
    return vec->array[index];
}

void *vector_get_unsafe(vector_t *vec, int index)
{
    return (void *) vector_get(vec, index);
}

int vector_cmp(const vector_t *vec1, const vector_t *vec2,
        int (*compar)(const void *, const void *))
{
    // Don't touch unless necessary.
    // The fact that length is the first ordering criterion is used particularly
    // in setup.c.
    if (vec1 == vec2) {
        return 0;
    }
    if (vec1->array == vec2->array) {
        return vec1->size - vec2->size;
    }
    if (vec1->size != vec2->size) {
        return vec1->size - vec2->size;
    }
    const int n = MIN(vec1->size, vec2->size);
    if (compar != NULL) {
        for (int i = 0; i < n; ++i) {
            const int cmp = compar(vec1->array[i], vec2->array[i]);
            if (cmp != 0) {
                return cmp;
            }
        }
    } else {
        const int cmp = memcmp(vec1->array, vec2->array, n * sizeof(void *));
        if (cmp != 0) {
            return cmp;
        }
    }
    return vec1->size - vec2->size;
}

bool vector_eq(const vector_t *vec1, const vector_t *vec2,
        int (*compar)(const void *, const void *))
{
    return vec1->size == vec2->size && vector_cmp(vec1, vec2, compar) == 0;
}

bool vector_is_prefix(const vector_t *vec1, const vector_t *vec2,
        int (*compar)(const void *, const void *))
{
    if (vector_size(vec1) > vector_size(vec2)) {
        return false;
    }
    const vector_t prefix = vector_lazy_copy_range(vec2, 0, vector_size(vec1));
    return vector_eq(vec1, &prefix, compar);
}

const void **vector_array(const vector_t *vec)
{
    return vec->array;
}

int vector_size(const vector_t *vec)
{
    return vec->size;
}

void vector_set(vector_t *vec, int index, const void *elem)
{
    assert(0 <= index && index <= vec->size);
    alloc_if_lazy(vec);
    vec->array[index] = elem;
}

void vector_prepend(vector_t *vec, const void *elem)
{
    vector_insert(vec, 0, elem);
}

void vector_append(vector_t *vec, const void *elem)
{
    vector_insert(vec, vec->size, elem);
}

void vector_insert(vector_t *vec, int index, const void *elem)
{
    assert(0 <= index && index <= vec->size);
    alloc_if_lazy(vec);
    if (vec->size + 1 >= vec->capacity) {
        vec->capacity = (vec->capacity + 1) * RESIZE_FACTOR;
        vec->array = REALLOC(vec->array, vec->capacity * sizeof(void *));
    }
    memmove(vec->array + index + 1,
            vec->array + index,
            (vec->size - index) * sizeof(void *));
    vec->array[index] = elem;
    ++vec->size;
}

void vector_prepend_all(vector_t *vec, const vector_t *elems)
{
    vector_insert_all(vec, 0, elems);
}

void vector_append_all(vector_t *vec, const vector_t *elems)
{
    vector_insert_all(vec, vec->size, elems);
}

void vector_insert_all(vector_t *vec, int index, const vector_t *elems)
{
    vector_insert_all_range(vec, index, elems, 0, elems->size);
}

void vector_prepend_all_range(vector_t *vec, const vector_t *elems,
        int from, int to)
{
    vector_insert_all_range(vec, 0, elems, from, to);
}

void vector_append_all_range(vector_t *vec, const vector_t *elems,
        int from, int to)
{
    vector_insert_all_range(vec, vec->size, elems, from, to);
}

void vector_insert_all_range(vector_t *vec, int index,
        const vector_t *elems, int from, int to)
{
    assert(0 <= index && index <= vec->size);
    if (elems->size == 0) {
        return;
    }
    alloc_if_lazy(vec);
    const int n_new_elems = to - from;
    if (vec->size + n_new_elems >= vec->capacity) {
        while (vec->size + n_new_elems >= vec->capacity) {
            vec->capacity = (vec->capacity + 1) * RESIZE_FACTOR;
        }
        vec->array = REALLOC(vec->array, vec->capacity * sizeof(void *));
    }
    memmove(vec->array + index + n_new_elems,
            vec->array + index,
            (vec->size - index) * sizeof(void *));
    memcpy(vec->array + index,
            elems->array + from,
            n_new_elems * sizeof(void *));
    vec->size += n_new_elems;
}

const void *vector_remove_first(vector_t *vec)
{
    return vector_remove(vec, 0);
}

const void *vector_remove_last(vector_t *vec)
{
    return vector_remove(vec, vec->size - 1);
}

const void *vector_remove(vector_t *vec, int index)
{
    const void *elem;
    assert(0 <= index && index < vec->size);
    elem = vec->array[index];
    if (index + 1 < vec->size) {
        alloc_if_lazy(vec);
        memmove(vec->array + index,
                vec->array + index + 1,
                (vec->size - index - 1) * sizeof(void *));
    }
    --vec->size;
    return elem;
}

void vector_remove_range(vector_t *vec, int from, int to)
{
    const int n_less_elems = to - from;
    if (n_less_elems <= 0) {
        return;
    }
    assert(0 <= from && to <= vec->size);
    if (to < vec->size) {
        alloc_if_lazy(vec);
        memmove(vec->array + from,
                vec->array + to,
                (vec->size - to) * sizeof(void *));
    }
    vec->size -= n_less_elems;
}

void vector_remove_all(vector_t *vec, const int indices[], int n_indices)
{
    if (n_indices == 0) {
        return;
    }
    for (int i = n_indices - 1, lo = indices[i], hi = lo; i >= 0; --i) {
        lo = indices[i];
        if (i == 0 || lo-1 != indices[i-1]) {
            if (hi + 1 < vec->size) {
                alloc_if_lazy(vec);
                memmove(vec->array + lo,
                        vec->array + hi + 1,
                        (vec->size - hi - 1) * sizeof(void *));
            }
            vec->size -= hi - lo + 1;
            if (i > 0) {
                hi = indices[i-1];
            }
        }
    }
}

void vector_clear(vector_t *vec)
{
    vec->size = 0;
}

