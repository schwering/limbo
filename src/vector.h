// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * An automatically resizing array container.
 * The capacity doubles each time the current capacity is exhausted.
 * Elements of the vector are const void pointers.
 * This should help preventing unintended changes of elements in vectors.
 * Since our set is based on the vector, such changes might compromise the
 * set structure.
 */
#ifndef _VECTOR_H_
#define _VECTOR_H_

#include <stdbool.h>

typedef struct {
    void const **array;
    int capacity;
    int size;
} vector_t;

vector_t *vector_new(void);
void vector_init(vector_t *vec);
void vector_free(vector_t *vec);

void vector_copy(vector_t *dst, const vector_t *src);
void vector_copy_range(vector_t *dst, const vector_t *src, int from, int to);

const void *vector_get(const vector_t *vec, int index);
int vector_size(const vector_t *vec);

bool vector_eq(const vector_t *vec1, const vector_t *vec2, int (*compar)(const void *, const void *));

void vector_prepend(vector_t *vec, const void *elem);
void vector_append(vector_t *vec, const void *elem);
void vector_insert(vector_t *vec, int index, const void *elem);

void vector_prepend_all(vector_t *vec, const vector_t *elems);
void vector_append_all(vector_t *vec, const vector_t *elems);
void vector_insert_all(vector_t *vec, int index, const vector_t *elems);

const void *vector_remove(vector_t *vec, int index);
const void *vector_clear(vector_t *vec);

#endif

