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
 * For typesafe usage there is a MAP macro which declares appropriate wrapper
 * functions.
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
const void **set_array(const set_t *set);
int set_size(const set_t *set);
int set_find(const set_t *set, const void *elem);
bool set_contains(const set_t *set, const void *elem);

bool set_add(set_t *set, const void *elem);

bool set_remove(set_t *set, const void *elem);
const void *set_remove_index(set_t *set, int index);
void set_clear(set_t *set);

#define SET_DECL(prefix, type) \
    typedef union { set_t s; } prefix##_t;\
    void prefix##_init(prefix##_t *s);\
    void prefix##_init_with_size(prefix##_t *s, int size);\
    void prefix##_copy(prefix##_t *dst, const prefix##_t *src);\
    void prefix##_singleton(prefix##_t *s, const type elem);\
    void prefix##_union(prefix##_t *s,\
            const prefix##_t *left, const prefix##_t *right);\
    void prefix##_difference(prefix##_t *s,\
            const prefix##_t *left, const prefix##_t *right);\
    void prefix##_intersection(prefix##_t *s,\
            const prefix##_t *left, const prefix##_t *right);\
    void prefix##_free(prefix##_t *s);\
    int prefix##_cmp(const prefix##_t *s1, const prefix##_t *s2);\
    bool prefix##_eq(const prefix##_t *s1, const prefix##_t *s2);\
    const type prefix##_get(const prefix##_t *s, int index);\
    const type *prefix##_array(const prefix##_t *s);\
    int prefix##_size(const prefix##_t *s);\
    int prefix##_find(const prefix##_t *s, const type elem);\
    bool prefix##_contains(const prefix##_t *s, const type elem);\
    bool prefix##_add(prefix##_t *s, const type elem);\
    bool prefix##_remove(prefix##_t *s, const type elem);\
    const type prefix##_remove_index(prefix##_t *s, int index);\
    void prefix##_clear(prefix##_t *s);

#define SET_IMPL(prefix, type, compar) \
    void prefix##_init(prefix##_t *s) {\
        set_init(&s->s, (compar_t) compar); }\
    void prefix##_init_with_size(prefix##_t *s, int size) {\
        set_init_with_size(&s->s, (compar_t) compar, size); }\
    void prefix##_copy(prefix##_t *dst, const prefix##_t *src) {\
        set_copy(&dst->s, &src->s); }\
    void prefix##_singleton(prefix##_t *s, const type elem) {\
        set_singleton(&s->s, (compar_t) compar, (const void *) elem); }\
    void prefix##_union(prefix##_t *s,\
            const prefix##_t *left, const prefix##_t *right) {\
        set_union(&s->s, &left->s, &right->s); }\
    void prefix##_difference(prefix##_t *s,\
            const prefix##_t *left, const prefix##_t *right) {\
        set_difference(&s->s, &left->s, &right->s); }\
    void prefix##_intersection(prefix##_t *s,\
            const prefix##_t *left, const prefix##_t *right) {\
        set_intersection(&s->s, &left->s, &right->s); }\
    void prefix##_free(prefix##_t *s) {\
        set_free(&s->s); }\
    int prefix##_cmp(const prefix##_t *s1,\
            const prefix##_t *s2) {\
        return set_cmp(&s1->s, &s2->s); }\
    bool prefix##_eq(const prefix##_t *s1,\
            const prefix##_t *s2) {\
        return set_eq(&s1->s, &s2->s); }\
    const type prefix##_get(const prefix##_t *s, int index) {\
        return (const type) set_get(&s->s, index); }\
    const type *prefix##_array(const prefix##_t *s) {\
        return (const type *) set_array(&s->s); }\
    int prefix##_size(const prefix##_t *s) {\
        return set_size(&s->s); }\
    int prefix##_find(const prefix##_t *s, const type elem) {\
        return set_find(&s->s, (const void *) elem); }\
    bool prefix##_contains(const prefix##_t *s,\
            const type elem) {\
        return set_contains(&s->s, (const void *) elem); }\
    bool prefix##_add(prefix##_t *s, const type elem) {\
        return set_add(&s->s, (const void *) elem); }\
    bool prefix##_remove(prefix##_t *s, const type elem) {\
        return set_remove(&s->s, (const void *) elem); }\
    const type prefix##_remove_index(prefix##_t *s, int index) {\
        return (const type) set_remove_index(&s->s, index); }\
    void prefix##_clear(prefix##_t *s) {\
        set_clear(&s->s); }

#endif

