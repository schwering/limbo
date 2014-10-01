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
 * set_cleanup(), set_cmp(), set_eq(), set_size(), set_clear().
 *
 * The purpose of set_get() is mostly for iteration over the set's elements as
 * the index depends on the element's order.
 * There is also a set_get_unsafe() which returns a non-const pointer to the
 * requested element. Modifying the element is dangerous as it may destroy the
 * set's ordering. Furthermore this involves casting the const pointers to
 * non-const pointers, which may be undefined if the memory actually is const.
 * set_find() returns the index of an element which is equal to the given one
 * according to the set's comparison function; it returns -1 if there is no
 * such element in the set.
 *
 * While union, difference, intersection are currently only implemented as
 * constructors, we could add variants which modify one of the operands.
 *
 * set_add() only inserts the element if it wasn't present before.
 * set_remove() returns true iff the element was actually removed.
 * set_replace[_index]() has the same effect as removing the old element /
 * index and adding a new element afterwards, but may be more efficient.
 * The value returned by set_add() and set_replace[_index]() is the index i at
 * which the new element is stored unless it was present before already; in
 * the latter case, a negative value is returned.
 *
 * Iterators behave like for vectors with two exceptions. Firstly,
 * set_iter_replace() inserts the new element at its correct place according to
 * the set's order and returns that index. If the new element was already
 * present and thus no insertion happened, a negative value is returned.
 * Secondly, modifying elements is dangerous because it may invalidate the
 * ordering of the set. For that reason, the val attribute even of non-const
 * iterators is const (if you know what you're doing, use val_unsafe).
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

typedef union {
    const void * const val;
    void * const val_unsafe;
    vector_iter_t iter;
} set_iter_t;

typedef union {
    const void * const val;
    void * const val_unsafe;
    vector_const_iter_t iter;
} set_const_iter_t;

set_t set_init(compar_t compar);
set_t set_init_with_size(compar_t compar, int size);
set_t set_copy(const set_t *src);
set_t set_lazy_copy(const set_t *src);
set_t set_singleton(compar_t compar, const void *elem);
set_t set_union(const set_t *left, const set_t *right);
set_t set_difference(const set_t *left, const set_t *right);
set_t set_intersection(const set_t *left, const set_t *right);
void set_cleanup(set_t *set);
bool set_is_lazy_copy(const set_t *set);

int set_cmp(const set_t *set1, const set_t *set2);
bool set_eq(const set_t *set1, const set_t *set2);

const void *set_get(const set_t *set, int index);
void *set_get_unsafe(set_t *set, int index);
const void **set_array(const set_t *set);
int set_size(const set_t *set);
int set_find(const set_t *set, const void *elem);
bool set_contains(const set_t *set, const void *elem);
bool set_contains_all(const set_t *set, const set_t *elems);

int set_add(set_t *set, const void *elem);
void set_add_all(set_t *set, const set_t *elems);

bool set_remove(set_t *set, const void *elem);
void set_remove_all(set_t *set, const set_t *elems);
const void *set_remove_index(set_t *set, int index);
void set_remove_index_range(set_t *set, int from, int to);
void set_remove_all_indices(set_t *set, const int indices[], int n_indices);

int set_replace(set_t *set, const void *old_elem, const void *new_elem);
int set_replace_index(set_t *set, int index, const void *new_elem);

void set_clear(set_t *set);

set_iter_t set_iter(set_t *set);
set_iter_t set_iter_from(set_t *set, const void *elem);
bool set_iter_next(set_iter_t *iter);
const void *set_iter_get(const set_iter_t *iter);
int set_iter_index(const set_iter_t *iter);
void set_iter_add_auditor(set_iter_t *iter, set_iter_t *auditor);
void set_iter_remove(set_iter_t *iter);
int set_iter_replace(set_iter_t *iter, const void *new_elem);

set_const_iter_t set_const_iter(const set_t *set);
set_const_iter_t set_const_iter_from(const set_t *set, const void *elem);
bool set_const_iter_next(set_const_iter_t *iter);
const void *set_const_iter_get(const set_const_iter_t *iter);

#define SET_DECL(prefix, type) \
    typedef union { set_t s; } prefix##_t;\
    typedef union {\
        const type const val;\
        type const val_unsafe;\
        set_iter_t i;\
    } prefix##_iter_t;\
    typedef union {\
        const type const val;\
        type const val_unsafe;\
        set_const_iter_t i;\
    } prefix##_const_iter_t;\
    prefix##_t prefix##_init(void);\
    prefix##_t prefix##_init_with_size(int size);\
    prefix##_t prefix##_copy(const prefix##_t *src);\
    prefix##_t prefix##_lazy_copy(const prefix##_t *src);\
    prefix##_t prefix##_singleton(const type elem);\
    prefix##_t prefix##_union(\
            const prefix##_t *left, const prefix##_t *right);\
    prefix##_t prefix##_difference(\
            const prefix##_t *left, const prefix##_t *right);\
    prefix##_t prefix##_intersection(\
            const prefix##_t *left, const prefix##_t *right);\
    void prefix##_cleanup(prefix##_t *s);\
    bool prefix##_is_lazy_copy(const prefix##_t *s);\
    int prefix##_cmp(const prefix##_t *s1, const prefix##_t *s2);\
    bool prefix##_eq(const prefix##_t *s1, const prefix##_t *s2);\
    const type prefix##_get(const prefix##_t *s, int index);\
    type prefix##_get_unsafe(prefix##_t *s, int index);\
    const type *prefix##_array(const prefix##_t *s);\
    int prefix##_size(const prefix##_t *s);\
    int prefix##_find(const prefix##_t *s, const type elem);\
    bool prefix##_contains(const prefix##_t *s, const type elem);\
    bool prefix##_contains_all(const prefix##_t *s, const prefix##_t *elems);\
    int prefix##_add(prefix##_t *s, const type elem);\
    void prefix##_add_all(prefix##_t *s, const prefix##_t *elems);\
    bool prefix##_remove(prefix##_t *s, const type elem);\
    void prefix##_remove_all(prefix##_t *s, const prefix##_t *elems);\
    const type prefix##_remove_index(prefix##_t *s, int index);\
    void prefix##_remove_index_range(prefix##_t *s, int from, int to);\
    void prefix##_remove_all_indices(prefix##_t *s, const int indices[],\
            int n_indices);\
    int prefix##_replace(prefix##_t *s, const type old_elem,\
            const type new_elem);\
    int prefix##_replace_index(prefix##_t *s, int index, const type new_elem);\
    void prefix##_clear(prefix##_t *s);\
    prefix##_iter_t prefix##_iter(prefix##_t *set);\
    prefix##_iter_t prefix##_iter_from(prefix##_t *set, const type elem);\
    bool prefix##_iter_next(prefix##_iter_t *iter);\
    const type prefix##_iter_get(const prefix##_iter_t *iter);\
    int prefix##_iter_index(const prefix##_iter_t *iter);\
    void prefix##_iter_add_auditor(prefix##_iter_t *iter,\
            prefix##_iter_t *auditor);\
    void prefix##_iter_remove(prefix##_iter_t *iter);\
    int prefix##_iter_replace(prefix##_iter_t *iter, const type new_elem);\
    prefix##_const_iter_t prefix##_const_iter(const prefix##_t *set);\
    prefix##_const_iter_t prefix##_const_iter_from(const prefix##_t *set,\
            const type elem);\
    bool prefix##_const_iter_next(prefix##_const_iter_t *iter);\
    const type prefix##_const_iter_get(const prefix##_const_iter_t *iter);

#define SET_IMPL(prefix, type, compar) \
    prefix##_t prefix##_init(void) {\
        return (prefix##_t) { .s = set_init((compar_t) compar) }; }\
    prefix##_t prefix##_init_with_size(int size) {\
        return (prefix##_t) {\
            .s = set_init_with_size((compar_t) compar, size) }; }\
    prefix##_t prefix##_copy(const prefix##_t *src) {\
        return (prefix##_t) { .s = set_copy(&src->s) }; }\
    prefix##_t prefix##_lazy_copy(const prefix##_t *src) {\
        return (prefix##_t) { .s = set_lazy_copy(&src->s) }; }\
    prefix##_t prefix##_singleton(const type elem) {\
        return (prefix##_t) {\
            .s = set_singleton((compar_t) compar, (const void *) elem) }; }\
    prefix##_t prefix##_union(\
            const prefix##_t *left, const prefix##_t *right) {\
        return (prefix##_t) { .s = set_union(&left->s, &right->s) }; }\
    prefix##_t prefix##_difference(\
            const prefix##_t *left, const prefix##_t *right) {\
        return (prefix##_t) { .s = set_difference(&left->s, &right->s) }; }\
    prefix##_t prefix##_intersection(\
            const prefix##_t *left, const prefix##_t *right) {\
        return (prefix##_t) { .s = set_intersection(&left->s, &right->s) }; }\
    void prefix##_cleanup(prefix##_t *s) { set_cleanup(&s->s); }\
    bool prefix##_is_lazy_copy(const prefix##_t *s) {\
        return set_is_lazy_copy(&s->s); }\
    int prefix##_cmp(const prefix##_t *s1, const prefix##_t *s2) {\
        return set_cmp(&s1->s, &s2->s); }\
    bool prefix##_eq(const prefix##_t *s1, const prefix##_t *s2) {\
        return set_eq(&s1->s, &s2->s); }\
    const type prefix##_get(const prefix##_t *s, int index) {\
        return (const type) set_get(&s->s, index); }\
    type prefix##_get_unsafe(prefix##_t *s, int index) {\
        return (type) set_get_unsafe(&s->s, index); }\
    const type *prefix##_array(const prefix##_t *s) {\
        return (const type *) set_array(&s->s); }\
    int prefix##_size(const prefix##_t *s) {\
        return set_size(&s->s); }\
    int prefix##_find(const prefix##_t *s, const type elem) {\
        return set_find(&s->s, (const void *) elem); }\
    bool prefix##_contains(const prefix##_t *s, const type elem) {\
        return set_contains(&s->s, (const void *) elem); }\
    bool prefix##_contains_all(const prefix##_t *s, const prefix##_t *elems) {\
        return set_contains_all(&s->s, &elems->s); }\
    int prefix##_add(prefix##_t *s, const type elem) {\
        return set_add(&s->s, (const void *) elem); }\
    void prefix##_add_all(prefix##_t *s, const prefix##_t *elems) {\
        return set_add_all(&s->s, &elems->s); }\
    bool prefix##_remove(prefix##_t *s, const type elem) {\
        return set_remove(&s->s, (const void *) elem); }\
    void prefix##_remove_all(prefix##_t *s, const prefix##_t *elems) {\
        set_remove_all(&s->s, &elems->s); }\
    const type prefix##_remove_index(prefix##_t *s, int index) {\
        return (const type) set_remove_index(&s->s, index); }\
    void prefix##_remove_index_range(prefix##_t *s, int from, int to) {\
        set_remove_index_range(&s->s, from, to); }\
    void prefix##_remove_all_indices(prefix##_t *s, const int indices[],\
            int n_indices) {\
        set_remove_all_indices(&s->s, indices, n_indices); }\
    int prefix##_replace(prefix##_t *s, const type old_elem,\
            const type new_elem) {\
        return set_replace(&s->s, (const void *) old_elem,\
                (const void *) new_elem); }\
    int prefix##_replace_index(prefix##_t *s, int index, const type new_elem) {\
        return set_replace_index(&s->s, index, (const void *) new_elem); }\
    void prefix##_clear(prefix##_t *s) { set_clear(&s->s); }\
    prefix##_iter_t prefix##_iter(prefix##_t *set) {\
        return (prefix##_iter_t) { .i = set_iter(&set->s) }; }\
    prefix##_iter_t prefix##_iter_from(prefix##_t *set, const type elem) {\
        return (prefix##_iter_t) {\
            .i = set_iter_from(&set->s, (const void *) elem) }; }\
    bool prefix##_iter_next(prefix##_iter_t *iter) {\
        return set_iter_next(&iter->i); }\
    const type prefix##_iter_get(const prefix##_iter_t *iter) {\
        return (const type) set_iter_get(&iter->i); }\
    int prefix##_iter_index(const prefix##_iter_t *iter) {\
        return set_iter_index(&iter->i); }\
    void prefix##_iter_add_auditor(prefix##_iter_t *iter,\
            prefix##_iter_t *auditor) {\
        set_iter_add_auditor(&iter->i, &auditor->i); }\
    void prefix##_iter_remove(prefix##_iter_t *iter) {\
        return set_iter_remove(&iter->i); }\
    int prefix##_iter_replace(prefix##_iter_t *iter, const type new_elem) {\
        return set_iter_replace(&iter->i, (const void *) new_elem); }\
    prefix##_const_iter_t prefix##_const_iter(const prefix##_t *set) {\
        return (prefix##_const_iter_t) { .i = set_const_iter(&set->s) }; }\
    prefix##_const_iter_t prefix##_const_iter_from(const prefix##_t *set,\
            const type elem) {\
        return (prefix##_const_iter_t) {\
            .i = set_const_iter_from(&set->s, (const void *) elem) }; }\
    bool prefix##_const_iter_next(prefix##_const_iter_t *iter) {\
        return set_const_iter_next(&iter->i); }\
    const type prefix##_const_iter_get(const prefix##_const_iter_t *iter) {\
        return (const type) set_const_iter_get(&iter->i); }

#endif

