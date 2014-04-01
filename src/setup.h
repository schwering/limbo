// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * Functions for generating setups from universally quantified static and boxed
 * clauses.
 *
 * The univ_clause attribute of univ_clause_t should return NULL if the clause
 * variable assignment is not permitted.
 *
 * setup_clauses_ground() substitutes all variables with standard names from ns,
 * and substitutes prepends all prefixes of elements from zs to the box
 * formulas.
 *
 * setup_pel() comptues the positive versions of all literals in the setup. Note
 * that for the split literals, you also need to consider those from the query.
 *
 * setup_propagate_units() roughly said computes the resolution of all unit
 * clauses with other clauses in the given setup. More precisely, the clauses in
 * the modified setup subsume all clauses derivable by unit propagataion. That
 * is, some clauses derivable by unit propagation may not be contained in the
 * setup, but for each of those a sub-clause is contained in the setup. There
 * are two reasons for this: firstly, we can use set_remove_all() to compute the
 * unit propagation of each clause; secondly, we keep the setup small (by even
 * removing all subsumed clauses) which leads to faster subsequent unit
 * propagations.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#ifndef _SETUP_H_
#define _SETUP_H_

#include "literal.h"
#include "set.h"
#include "term.h"

SET_DECL(litset, literal_t *);
SET_ALIAS(clause, litset, literal_t *);
SET_ALIAS(pelset, litset, literal_t *);
SET_DECL(setup, clause_t *);

typedef struct {
    stdset_t names;
    varset_t vars;
    const clause_t *(*univ_clause)(const varmap_t *map);
} univ_clause_t;

typedef union { univ_clause_t c; } box_univ_clause_t;

VECTOR_DECL(univ_clauses, univ_clause_t *);
VECTOR_DECL(box_univ_clauses, box_univ_clause_t *);

const clause_t *clause_empty(void);
bool clause_subsumes(const clause_t *c, const clause_t *d);

setup_t setup_ground_clauses(
        const box_univ_clauses_t *dynamic_bat,
        const univ_clauses_t *static_bat,
        const stdvecset_t *query_zs,
        const stdset_t *query_ns,
        int n_query_vars);
pelset_t setup_pel(const setup_t *setup);
setup_t setup_copy_new_clauses(const setup_t *setup);
void setup_propagate_units(setup_t *setup);

#endif

