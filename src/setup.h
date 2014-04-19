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
 * setup_minimize() removes all clauses subsumed by other clauses in the setup.
 *
 * setup_propagate_units() closes the given setup under resolution of its unit
 * clauses with all other clauses. That is, for each resolvent of a unit clause
 * there is a clause in the new setup which subsumes this resolvent.
 * Moreoever, the returned setup is minimal if the given setup was minimal,
 * where minimality means that no clause is subsumed by another one in the
 * setup.
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

typedef struct ewff ewff_t;

typedef struct univ_clause univ_clause_t;
typedef union box_univ_clause box_univ_clause_t;

SET_DECL(univ_clauses, univ_clause_t *);
SET_DECL(box_univ_clauses, box_univ_clause_t *);

const ewff_t *ewff_true(void);
const ewff_t *ewff_eq(term_t t1, term_t t2);
const ewff_t *ewff_neq(term_t t1, term_t t2);
const ewff_t *ewff_sort(term_t t, bool (*is_sort)(stdname_t n));
const ewff_t *ewff_neg(const ewff_t *e1);
const ewff_t *ewff_or(const ewff_t *e1, const ewff_t *e2);
const ewff_t *ewff_and(const ewff_t *e1, const ewff_t *e2);
bool ewff_eval(const ewff_t *e, const varmap_t *varmap);

const univ_clause_t *univ_clause_init(
        const ewff_t *cond,
        const clause_t *clause);
const box_univ_clause_t *box_univ_clause_init(
        const ewff_t *cond,
        const clause_t *clause);

const clause_t *clause_empty(void);
clause_t clause_substitute(const clause_t *c, const varmap_t *map);
bool clause_is_ground(const clause_t *c);
varset_t clause_vars(const clause_t *c);
stdset_t clause_names(const clause_t *c);

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

void setup_minimize(setup_t *setup);

void setup_propagate_units(setup_t *setup);

bool setup_subsumes(setup_t *setup, const clause_t *c);

#endif

