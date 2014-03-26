// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * An automatically resizing array container.
 * The capacity doubles each time the current capacity is exhausted.
 * Elements of the vector are const void pointers.
 * This should help preventing unintended changes of elements in vectors.
 * Since our set is based on the vector, such changes might compromise the
 * set structure.
 *
 * Each vector object needs to be initialized with vector_init[_with_size]()
 * or vector_copy[_range](). The latter ones copy [the range of] a vector into
 * the new one.
 * Deallocation is done with vector_free().
 *
 * vector_cmp() compares does byte-wise comparison if the compar parameter is
 * NULL and applies compar to all elements otherwise.
 *
 * After vector_insert(), vector_get() returns the inserted element for the
 * respective index. After vector_insert_all[_rang](), first element of the
 * [indicated range of the] source is at the respective index of the target,
 * followed by the remaining elements from [the range of] the target.
 *
 * Indices start at 0.
 *
 * For all functions with _range in its name: the from index is meant
 * inclusively, the to index is exclusive. That is, the range has (to - from)
 * elements.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#ifndef _VECTOR_H_
#define _VECTOR_H_

#include <stdbool.h>

typedef struct {
    void const **array;
    int capacity;
    int size;
} vector_t;

void vector_init(vector_t *vec);
void vector_init_with_size(vector_t *vec, int size);
void vector_copy(vector_t *dst, const vector_t *src);
void vector_copy_range(vector_t *dst, const vector_t *src, int from, int to);
void vector_free(vector_t *vec);

const void *vector_get(const vector_t *vec, int index);
int vector_size(const vector_t *vec);

int vector_cmp(const vector_t *vec1, const vector_t *vec2,
        int (*compar)(const void *, const void *));
bool vector_eq(const vector_t *vec1, const vector_t *vec2,
        int (*compar)(const void *, const void *));

void vector_prepend(vector_t *vec, const void *elem);
void vector_append(vector_t *vec, const void *elem);
void vector_insert(vector_t *vec, int index, const void *elem);

void vector_prepend_all(vector_t *vec, const vector_t *elems);
void vector_append_all(vector_t *vec, const vector_t *elems);
void vector_insert_all(vector_t *vec, int index, const vector_t *elems);

void vector_append_all_range(vector_t *vec,
        const vector_t *elems, int from, int to);
void vector_preend_all_range(vector_t *vec,
        const vector_t *elems, int from, int to);
void vector_insert_all_range(vector_t *vec, int index,
        const vector_t *elems, int from, int to);

const void *vector_remove(vector_t *vec, int index);
void vector_clear(vector_t *vec);

#endif

