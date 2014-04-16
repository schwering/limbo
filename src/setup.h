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
 * setup_propagate_units() computes a new setup which is closed under
 * resolution of its unit clauses with all other clauses. More precisely, for
 * each resolvent of a unit clause there is a clause in the new setup which
 * subsumes this resolvent. It may appear wasteful, but we need to create a
 * new setup anyway because the ordering could be confused otherwise. We use a
 * lazy copy to reduce the amount of copying.
 *
 * setup_subsumes() returns true if unit propagation of the setup plus split
 * literals contains a clause which is a subset of the given clause.
 * Thus, setup_subsumes() is sound but not complete.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#ifndef _SETUP_H_
#define _SETUP_H_

#include "literal.h"
#include "set.h"
#include "term.h"

SET_DECL(clause, literal_t *);
SET_ALIAS(splitset, clause, literal_t *);
SET_ALIAS(pelset, clause, literal_t *);
SET_DECL(setup, clause_t *);
SET_ALIAS(cnf, setup, clause_t *);

typedef struct {
    stdset_t names;
    varset_t vars;
    const clause_t *(*univ_clause)(const varmap_t *map);
} univ_clause_t;

typedef union { univ_clause_t c; } box_univ_clause_t;

VECTOR_DECL(univ_clauses, univ_clause_t *);
VECTOR_DECL(box_univ_clauses, box_univ_clause_t *);

const clause_t *clause_empty(void);

stdset_t bat_hplus(
        const univ_clauses_t *static_bat,
        const box_univ_clauses_t *dynamic_bat,
        const stdset_t *query_names,
        int n_query_vars);

setup_t setup_init_static(
        const univ_clauses_t *static_bat,
        const stdset_t *hplus);

setup_t setup_init_dynamic(
        const box_univ_clauses_t *dynamic_bat,
        const stdset_t *hplus,
        const stdvecset_t *query_zs);

setup_t setup_init_static_and_dynamic(
        const univ_clauses_t *static_bat,
        const box_univ_clauses_t *dynamic_bat,
        const stdset_t *hplus,
        const stdvecset_t *query_zs);

void setup_add_sensing_results(
        setup_t *setup,
        const splitset_t *sensing_results);

void add_pel_of_clause(pelset_t *pel, const clause_t *c, const setup_t *setup);
pelset_t setup_pel(const setup_t *setup);
bool setup_would_be_needless_split(const setup_t *setup, const literal_t *l);

void setup_propagate_units(setup_t *setup);
void setup_propagate_units2(setup_t *setup, const splitset_t *split);

bool setup_subsumes(
        setup_t *setup,
        const clause_t *c);
bool setup_subsumes2(
        const setup_t *setup,
        const splitset_t *split,
        const clause_t *c);

#endif

