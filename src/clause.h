// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * The univ_clause attribute of univ_clause_t should return NULL if the clause
 * variable assignment is not permitted.
 *
 * ground_clauses() substitutes all variables with standard names from ns. The
 * result is a variable-free box clause. The box is then substituted by all
 * prefixes of any action sequence from zs.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#ifndef _CLAUSE_H_
#define _CLAUSE_H_

#include "literal.h"
#include "set.h"
#include "term.h"

SET_DECL(clause, literal_t *);
SET_DECL(setup, clause_t *);

typedef struct {
    varset_t vars;
    clause_t *(*univ_clause)(const varmap_t *map);
} univ_clause_t;

#endif

