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

set_t set_init(compar_t compar);
set_t set_init_with_size(compar_t compar, int size);
set_t set_copy(const set_t *src);
set_t set_lazy_copy(const set_t *src);
set_t set_singleton(compar_t compar, const void *elem);
set_t set_union(const set_t *left, const set_t *right);
set_t set_difference(const set_t *left, const set_t *right);
set_t set_intersection(const set_t *left, const set_t *right);
void set_free(set_t *set);

int set_cmp(const set_t *set1, const set_t *set2);
bool set_eq(const set_t *set1, const set_t *set2);

const void *set_get(const set_t *set, int index);
void *set_get_unsafe(set_t *set, int index);
const void **set_array(const set_t *set);
int set_size(const set_t *set);
int set_find(const set_t *set, const void *elem);
bool set_contains(const set_t *set, const void *elem);
bool set_contains_all(const set_t *set, const set_t *elems);

bool set_add(set_t *set, const void *elem);
void set_add_all(set_t *set, const set_t *elems);

bool set_remove(set_t *set, const void *elem);
void set_remove_all(set_t *set, const set_t *elems);
const void *set_remove_index(set_t *set, int index);
void set_remove_all_indices(set_t *set, const int indices[], int n_indices);
void set_clear(set_t *set);

#define SET_DECL(prefix, type) \
    typedef union { set_t s; } prefix##_t;\
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
    void prefix##_free(prefix##_t *s);\
    int prefix##_cmp(const prefix##_t *s1, const prefix##_t *s2);\
    bool prefix##_eq(const prefix##_t *s1, const prefix##_t *s2);\
    const type prefix##_get(const prefix##_t *s, int index);\
    type prefix##_get_unsafe(prefix##_t *s, int index);\
    const type *prefix##_array(const prefix##_t *s);\
    int prefix##_size(const prefix##_t *s);\
    int prefix##_find(const prefix##_t *s, const type elem);\
    bool prefix##_contains(const prefix##_t *s, const type elem);\
    bool prefix##_contains_all(const prefix##_t *s, const prefix##_t *elems);\
    bool prefix##_add(prefix##_t *s, const type elem);\
    void prefix##_add_all(prefix##_t *s, const prefix##_t *elems);\
    bool prefix##_remove(prefix##_t *s, const type elem);\
    void prefix##_remove_all(prefix##_t *s, const prefix##_t *elems);\
    const type prefix##_remove_index(prefix##_t *s, int index);\
    void prefix##_remove_all_indices(prefix##_t *s, const int indices[],\
            int n_indices);\
    void prefix##_clear(prefix##_t *s);

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
    void prefix##_free(prefix##_t *s) { set_free(&s->s); }\
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
    bool prefix##_add(prefix##_t *s, const type elem) {\
        return set_add(&s->s, (const void *) elem); }\
    void prefix##_add_all(prefix##_t *s, const prefix##_t *elems) {\
        return set_add_all(&s->s, &elems->s); }\
    bool prefix##_remove(prefix##_t *s, const type elem) {\
        return set_remove(&s->s, (const void *) elem); }\
    void prefix##_remove_all(prefix##_t *s, const prefix##_t *elems) {\
        set_remove_all(&s->s, &elems->s); }\
    const type prefix##_remove_index(prefix##_t *s, int index) {\
        return (const type) set_remove_index(&s->s, index); }\
    void prefix##_remove_all_indices(prefix##_t *s, const int indices[],\
            int n_indices) {\
        set_remove_all_indices(&s->s, indices, n_indices); }\
    void prefix##_clear(prefix##_t *s) { set_clear(&s->s); }

#define SET_ALIAS(alias, prefix, type) \
    typedef union { prefix##_t s; } alias##_t;\
    static inline alias##_t alias##_init(void) {\
         return (alias##_t) { .s = prefix##_init() }; }\
    static inline alias##_t alias##_init_with_size(int size) {\
         return (alias##_t) { .s = prefix##_init_with_size(size) }; }\
    static inline alias##_t alias##_copy(const alias##_t *src) {\
         return (alias##_t) { .s = prefix##_copy(&src->s) }; }\
    static inline alias##_t alias##_lazy_copy(const alias##_t *src) {\
         return (alias##_t) { .s = prefix##_lazy_copy(&src->s) }; }\
    static inline alias##_t alias##_singleton(const type elem) {\
         return (alias##_t) { .s = prefix##_singleton(elem) }; }\
    static inline alias##_t alias##_union(\
             const alias##_t *left, const alias##_t *right) {\
         return (alias##_t) { .s = prefix##_union(&left->s, &right->s) }; }\
    static inline alias##_t alias##_difference(\
             const alias##_t *left, const alias##_t *right) {\
         return (alias##_t) {\
             .s = prefix##_difference(&left->s, &right->s) }; }\
    static inline alias##_t alias##_intersection(\
             const alias##_t *left, const alias##_t *right) {\
         return (alias##_t) {\
             .s = prefix##_intersection(&left->s, &right->s) }; }\
    static inline void alias##_free(alias##_t *s) {\
         prefix##_free(&s->s); }\
    static inline int alias##_cmp(const alias##_t *s1,\
             const alias##_t *s2) {\
         return prefix##_cmp(&s1->s, &s2->s); }\
    static inline bool alias##_eq(const alias##_t *s1,\
             const alias##_t *s2) {\
         return prefix##_eq(&s1->s, &s2->s); }\
    static inline const type alias##_get(const alias##_t *s, int index) {\
         return prefix##_get(&s->s, index); }\
    static inline type alias##_get_unsafe(alias##_t *s, int index) {\
         return prefix##_get_unsafe(&s->s, index); }\
    static inline const type *alias##_array(const alias##_t *s) {\
         return prefix##_array(&s->s); }\
    static inline int alias##_size(const alias##_t *s) {\
         return prefix##_size(&s->s); }\
    static inline int alias##_find(const alias##_t *s, const type elem) {\
         return prefix##_find(&s->s, elem); }\
    static inline bool alias##_contains(const alias##_t *s, const type elem) {\
         return prefix##_contains(&s->s, elem); }\
    static inline bool alias##_contains_all(const alias##_t *s,\
            const alias##_t *elems) {\
         return prefix##_contains_all(&s->s, &elems->s); }\
    static inline bool alias##_add(alias##_t *s, const type elem) {\
         return prefix##_add(&s->s, elem); }\
    static inline void alias##_add_all(alias##_t *s, const alias##_t *elems) {\
         return prefix##_add_all(&s->s, &elems->s); }\
    static inline bool alias##_remove(alias##_t *s, const type elem) {\
         return prefix##_remove(&s->s, elem); }\
    static inline void alias##_remove_all(alias##_t *s,\
            const alias##_t *elems) {\
         prefix##_remove_all(&s->s, &elems->s); }\
    static inline const type alias##_remove_index(alias##_t *s, int index) {\
         return prefix##_remove_index(&s->s, index); }\
    static inline void alias##_remove_all_indices(alias##_t *s,\
            const int indices[], int n_indices) {\
        prefix##_remove_all_indices(&s->s, indices, n_indices); }\
    static inline void alias##_clear(alias##_t *s) { prefix##_clear(&s->s); }

#endif

