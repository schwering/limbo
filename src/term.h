// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * Type definitions for variables and standard names.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#ifndef _TERM_H_
#define _TERM_H_

#include "map.h"
#include <limits.h>

typedef long int stdname_t;
typedef long int var_t;

#define STDNAME_MAX LONG_MAX

int stdname_compar(stdname_t l, stdname_t r);

MAP_DECL(varmap, var_t, stdname_t);

VECTOR_DECL(varvec, var_t);

SET_DECL(varset, var_t);

VECTOR_DECL(stdvec, stdname_t);
const stdvec_t *stdvec_empty(void);
stdvec_t stdvec_prepend_copy(stdname_t n, const stdvec_t *src);
stdvec_t stdvec_copy_append(const stdvec_t *src, stdname_t n);
stdvec_t stdvec_concat(const stdvec_t *left, const stdvec_t *right);
bool stdvec_is_prefix(const stdvec_t *left, const stdvec_t *right);

SET_DECL(stdvecset, stdvec_t *);

SET_DECL(stdset, stdname_t);

#endif

