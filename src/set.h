// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * A set structure based on an ordered array and binary search.
 * The comparison is carried out by a functions of type compar_t which shall
 * behave as usual in C:
 *  compar(x,y) <  0 iff x < y
 *  compar(x,y) == 0 iff x = y
 *  compar(x,y) >  0 iff x > y
 *
 * The underlying data structure is vector_t. Most functions have a
 * corresponding counterpart in vector.h, for example, set_init[_with_size](),
 * set_free(), set_cmp(), set_eq(), set_size(), set_clear().
 *
 * The purpose of set_get() is mostly for iteration over the set's elements as
 * the indx depends on the element's order.
 * set_find() returns the index of an element which is equal to the given one
 * according to the set's comparison function; it returns -1 if there is no
 * such element in the set.
 *
 * While union, difference, intersection are currently only implemented as
 * constructors, we could add variants which modify one of the operands.
 *
 * set_add() only inserts the element if it wasn't present before.
 * set_add() returns true iff the element was actually inserted.
 * set_remove() returns true iff the element was actually removed.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#ifndef _SET_H_
#define _SET_H_

#include "vector.h"
#include <stdbool.h>

typedef int (*compar_t)(const void *, const void *);

typedef struct {
    vector_t vec;
    compar_t compar;
} set_t;

void set_init(set_t *set, compar_t compar);
void set_init_with_size(set_t *set, compar_t compar, int size);
void set_copy(set_t *dst, const set_t *src);
void set_singleton(set_t *set, compar_t compar, const void *elem);
void set_union(set_t *set, const set_t *left, const set_t *right);
void set_difference(set_t *set, const set_t *left, const set_t *right);
void set_intersection(set_t *set, const set_t *left, const set_t *right);
void set_free(set_t *set);

int set_cmp(const set_t *set1, const set_t *set2);
bool set_eq(const set_t *set1, const set_t *set2);

const void *set_get(const set_t *set, int index);
int set_size(const set_t *set);
int set_find(const set_t *set, const void *elem);
bool set_contains(const set_t *set, const void *elem);

bool set_add(set_t *set, const void *elem);

bool set_remove(set_t *set, const void *elem);
const void *set_remove_index(set_t *set, int index);
void set_clear(set_t *set);

#endif

