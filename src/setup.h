// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * Functions for generating setups from universally quantified static and boxed
 * clauses.
 *
 * The univ_clause attribute of univ_clause_t should return NULL if the clause
 * variable assignment is not permitted.
 *
 * bat_hplus() computes the set of standard name that needs to be considered
 * for quantification. Besides the BAT it depends on the standard names and
 * number of variables in the query.
 *
 * setup_init_[static|dynamic|static_and_dynamic] create setups from the
 * respective parts of a BAT. It does so by substituting all variables with
 * standard names from hplus and, for the dynamic part, by instantiating the
 * (implicit) box operators with all prefixes of action sequences in the
 * query, query_zs.
 *
 * setup_pel() computes the positive versions of all literals in the setup.
 * Note that for the split literals, you also need to consider those from the
 * query.
 * For that, add_pel_of_clause() may be useful.
 * setup_would_be_needless_split() returns true if splitting the given literal
 * would not give additional information to the setup. For example, splitting a
 * literal when the setup already contains a unit clause with that literal or
 * its negation is useless. That's currently the only sanity check done.
 *
 * setup_propagate_units() closes the given setup under resolution of its unit
 * clauses with all other clauses. More precisely, for each resolvent of a
 * unit clause there is a clause in the new setup which subsumes this
 * resolvent.
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

bool setup_subsumes(setup_t *setup, const clause_t *c);

#endif

