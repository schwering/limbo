// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * Type definitions for variables and standard names.
 *
 * We choose convenience over type-safety here: variables are negative integers,
 * standard names are non-negative integers.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#ifndef _TERM_H_
#define _TERM_H_

#include "vector.h"
#include "set.h"
#include "map.h"
#include <assert.h>
#include <limits.h>

typedef long int term_t;
typedef term_t stdname_t;
typedef term_t var_t;

#define STDNAME_MAX LONG_MAX

#define IS_VARIABLE(x) ((x) < 0)
#define IS_STDNAME(x)  ((x) >= 0)

MAP_DECL(varmap, var_t, stdname_t);

SET_DECL(varset, var_t);

SET_DECL(stdset, stdname_t);

VECTOR_DECL(stdvec, stdname_t);

SET_DECL(stdvecset, stdvec_t *);

#endif

