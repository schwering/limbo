// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * Type definitions for variables and standard names.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#ifndef _TERM_H_
#define _TERM_H_

#include "map.h"
#include <stdlib.h>

typedef long int stdname_t;
typedef long int var_t;

int stdname_compar(stdname_t l, stdname_t r);

MAP_DECL(varmap, var_t, stdname_t);

VECTOR_DECL(stdvec, stdname_t);
void stdvec_prepend_copy(stdvec_t *dst, stdname_t n, const stdvec_t *src);
void stdvec_copy_append(stdvec_t *dst, const stdvec_t *src, stdname_t n);

#endif

