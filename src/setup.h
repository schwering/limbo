// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * Setups are sets of ground clauses, which are disjunctions of (extended)
 * literals (as defined in literal.h).
 * Setups are the semantic primitive of ESL.
 * In ESL a setup is usually closed under unit propagation and subsumption,
 * and a setup entails a clause if it is contained in the setup.
 * Query answering is answered in queries.h.
 *
 * This header provides functions to generate setups from proper+ basic action
 * theories (BATs) and operations on setups, particularly subsumption test.
 * Proper+ BATs consist of implications where all variables are universally
 * quantified with maximum scope, the antecedent only is a so-called ewff, which
 * is a formula which mentions no other predicate than equality, and the
 * consequent is a disjunction of literals; formulas like successor state axioms
 * and sensed fluent axioms are boxed, that is, have a leading box operator
 * which says that they hold in any situation.
 * We don't close setups under subsumption to keep them finite; instead we check
 * if some clause in the setup subsumes the given one.
 *
 * Static formulas (for the initial situation) are expressed with
 * univ_clause_init(), while dynamic formulas (for preconditions, successor
 * state axioms, sensed fluent axioms) are expressed with
 * box_univ_clause_init().
 * Each of them consists of an ewff, which is a formula with only equality and
 * no other literals and all variables are implicitly universally quantified.
 * Variables are represented by negative values, standard names by non-negative
 * integers.
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
 * The returned setup is just the set containing all clauses resulting from the
 * ground -- no minimization or unit propagation has been done yet. Thus the
 * setup can be seen as the immediate result of grounding the clauses.
 *
 * setup_pel() computes the positive versions of all literals in the setup.
 * Note that for the split literals, you also need to consider those from the
 * query.
 * For that, clause_pel_and_sf() may be useful.
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
 * setup_subsumes() returns true if unit propagation of the setup literals
 * contains a clause which subsumes the given clause, that is, is a subset of
 * the given clause.
 * Thus, setup_subsumes() is sound but not complete.
 *
 * setup_with_splits_and_sf_subsumes() uses setup_subsumes() for reasoning by
 * cases.
 * Firstly, it adds SF literals to PEL which correspond to the action sequences
 * in the given clause and are actually worth splitting in the given setup.
 * Then it splits up to k non-SF literals from the PEL set, and if still no
 * subsumption has been detected using these splits, it tries to split the SF
 * literals which were just added.
 * This handling of SF literals almost like normal split literals allows to
 * handle both in a single function. The reason for splitting SF literals only
 * if k = 0 is just to remain equivalent to the ESL paper.
 * Note that setup_with_splits_subsumes() does not change the setup besides unit
 * propagation, that is, the setup is, from a semantic standpoint, at least as
 * good as before.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#ifndef _SETUP_H_
#define _SETUP_H_

#include "literal.h"
#include "set.h"
#include "term.h"

SET_DECL(clause, literal_t *);
SET_DECL(splitset, literal_t *);
SET_DECL(pelset, literal_t *);
SET_DECL(setup, clause_t *);

typedef struct ewff ewff_t;

typedef struct {
    const ewff_t *cond;
    const clause_t *clause;
    stdset_t names;
    varset_t vars;
} univ_clause_t;

typedef union { univ_clause_t c; } box_univ_clause_t;

VECTOR_DECL(univ_clauses, univ_clause_t *);
VECTOR_DECL(box_univ_clauses, box_univ_clause_t *);

const ewff_t *ewff_true(void);
const ewff_t *ewff_eq(term_t t1, term_t t2);
const ewff_t *ewff_neq(term_t t1, term_t t2);
const ewff_t *ewff_sort(term_t t, bool (*is_sort)(stdname_t n));
const ewff_t *ewff_neg(const ewff_t *e1);
const ewff_t *ewff_or(const ewff_t *e1, const ewff_t *e2);
const ewff_t *ewff_and(const ewff_t *e1, const ewff_t *e2);
bool ewff_eval(const ewff_t *e, const varmap_t *varmap);
void ewff_ground(
        const ewff_t *e,
        const varset_t *vars,
        const stdset_t *hplus,
        void (*ground)(const varmap_t *));

clause_t clause_substitute(const clause_t *c, const varmap_t *map);
const clause_t *clause_empty(void);

const univ_clause_t *univ_clause_init(
        const ewff_t *cond,
        const clause_t *clause);
const box_univ_clause_t *box_univ_clause_init(
        const ewff_t *cond,
        const clause_t *clause);

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

bool setup_would_be_needless_split(const setup_t *setup, const literal_t *l);
pelset_t setup_pel(const setup_t *setup);

void setup_minimize(setup_t *setup);
void setup_propagate_units(setup_t *setup);

bool setup_is_inconsistent(const setup_t *s);
bool setup_subsumes(setup_t *setup, const clause_t *c);
bool setup_with_splits_and_sf_subsumes(
        setup_t *setup,
        const pelset_t *pel,
        const clause_t *c,
        const int k);

#endif

