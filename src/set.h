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
void set_init_with_size(set_t *set, compar_t compar, int size);
void set_singleton(set_t *set, compar_t compar, const void *elem);
void set_union(set_t *set, const set_t *left, const set_t *right);
void set_difference(set_t *set, const set_t *left, const set_t *right);
void set_intersection(set_t *set, const set_t *left, const set_t *right);
void set_free(set_t *set);

const void *set_get(const set_t *set, int index);
int set_size(const set_t *set);
int set_find(const set_t *set, const void *elem);
bool set_contains(const set_t *set, const void *elem);

bool set_eq(const set_t *set1, const set_t *set2);

void set_add(set_t *set, const void *elem);

void set_remove(set_t *set, const void *elem);
void set_clear(set_t *set);

#endif

