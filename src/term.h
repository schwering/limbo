// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * Type definitions for variables and standard names.
 * For variable assignments we use maps from variables to standard names.
 * For action sequences and arguments of predicates we use vectors of standard
 * names.
 * We have typedefs and wrapper functions for both.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#ifndef _TERM_H_
#define _TERM_H_

#include "map.h"

typedef long int stdname_t;
typedef long int var_t;
typedef map_t varmap_t;
typedef vector_t stdvec_t;

void varmap_init(varmap_t *map);
void varmap_free(varmap_t *map);

bool varmap_add(varmap_t *map, var_t var, stdname_t name);
var_t varmap_add_replace(varmap_t *map, var_t var, stdname_t name);

void stdvec_init(stdvec_t *vec);
void stdvec_copy(stdvec_t *dst, const stdvec_t *src);
void stdvec_prepend_copy(stdvec_t *dst, stdname_t n, const stdvec_t *src);
void stdvec_copy_append(stdvec_t *dst, const stdvec_t *src, stdname_t n);
void stdvec_free(stdvec_t *vec);

stdname_t stdvec_get(const stdvec_t *vec, int index);
int stdvec_size(const stdvec_t *vec);
int stdvec_cmp(const stdvec_t *vec1, const stdvec_t *vec2);
bool stdvec_eq(const stdvec_t *vec1, const stdvec_t *vec2);

#endif

