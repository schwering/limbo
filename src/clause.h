// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * The univ_clause attribute of univ_clause_t should return NULL if the clause
 * variable assignment is not permitted.
 *
 * clauses_ground() substitutes all variables with standard names from ns, and
 * substitutes prepends all prefixes of elements from zs to the box formulas.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#ifndef _CLAUSE_H_
#define _CLAUSE_H_

#include "literal.h"
#include "set.h"
#include "term.h"

SET_DECL(clause, literal_t *);
SET_DECL(pelset, literal_t *);
SET_DECL(setup, clause_t *);

typedef struct {
    stdset_t names;
    varset_t vars;
    const clause_t *(*univ_clause)(const varmap_t *map);
} univ_clause_t;

typedef union { univ_clause_t c; } box_univ_clause_t;

VECTOR_DECL(univ_clauses, univ_clause_t *);
VECTOR_DECL(box_univ_clauses, box_univ_clause_t *);

setup_t setup_ground_clauses(
        const box_univ_clauses_t *dynamic_bat,
        const univ_clauses_t *static_bat,
        const stdvecset_t *query_zs,
        const stdset_t *query_ns,
        int n_query_vars);
pelset_t setup_pel(const setup_t *setup);

bool clause_resolvable(const clause_t *c, const literal_t *l);
clause_t clause_resolve(const clause_t *c, const literal_t *l);

bool clause_is_unit(const clause_t *c);
literal_t clause_unit(const clause_t *c);

void unit_propagataion(setup_t *setup);

#endif

