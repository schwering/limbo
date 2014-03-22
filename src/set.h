// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#ifndef _SET_H_
#define _SET_H_

#include "vector.h"
#include <stdbool.h>

typedef int (*compar_t)(const void *, const void *);

typedef struct {
    vector_t vec;
    compar_t compar;
} set_t;

set_t *set_new(compar_t compar);
void set_init(set_t *set, compar_t compar);

const void *set_get(const set_t *set, int index);
int set_size(const set_t *set);

bool set_eq(const set_t *set1, const set_t *set2);

void set_add(set_t *set, const void *elem);
void set_remove(set_t *set, const void *elem);

void set_prepend_all(set_t *set, const set_t *elems);
void set_append_all(set_t *set, const set_t *elems);

#endif

