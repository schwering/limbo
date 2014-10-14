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
 * for quantification. These are the standard names from the BAT and the query
 * plus one additional standard name for each variable in the BAT and the query.
 *
 * setup_init_[static|dynamic]() create setups from the respective parts of a
 * BAT. It does so by substituting all variables with standard names from hplus
 * and, for the dynamic part, by instantiating the (implicit) box operators with
 * all prefixes of action sequences in the query, query_zs.
 * The returned setup is just the set containing all clauses resulting from the
 * ground -- no minimization or unit propagation has been done yet. Thus the
 * setup can be seen as the immediate result of grounding the clauses.
 * The setup_union() of the static and dynamic setups is perhaps what the user
 * is interested in. It is recommonded to minimize that setup once.
 *
 * setup_pel() computes the positive extended literals (PEL) other than SF
 * literals from the setup that are candidates for splitting.
 * SF literals are not included because they are treated specifically in
 * setup_with_splits_and_sf_subsumes().
 * Moreover, literals that occur in the setup as unit clauses are not relevant
 * for splitting.
 *
 * setup_minimize() removes all clauses subsumed by other clauses in the setup.
 * One the one hand, minimization may improve performance drastically. On the
 * other hand, it is itself quite expensive. It is hence recommonded to minimize
 * new setups and to avoid redundant minimizations.
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
 * setup_with_splits_and_sf_subsumes() uses reasoning by cases and subsumption.
 * The parameter k denotes the number of split non-SF PEL literals.
 * After that many splits, the SF literals corresponding to the actions executed
 * in the query are split as well.
 * This treatment of SF literals differs from the semantics from the ESL paper
 * (Lakemeyer and Levesque, KR-2014), but it simplifies the code because it lets
 * us treat SF literals almost like non-SF literals, while the ESL paper splits
 * them already while processing the query (rules 11 and 12 in the procedure V).
 * Due to this treatment of SF literals, the implementation would not find that
 * SF(a) v ~SF(a) is true even for k = 1 (c.f. test_eventual_completeness). For
 * fluent queries, however, eventual completeness is preserved (I think). A
 * formal analysis is pending.
 * Once the clause is subsumed by the setup plus split literals as indicated by
 * setup_subsumes(), the recursive descent stops with returning true.
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

void setup_add_sensing_result(
        setup_t *setup,
        const stdvec_t *z,
        const stdname_t n,
        const bool r);

void setup_minimize(setup_t *setup);
void setup_propagate_units(setup_t *setup);

bool setup_subsumes(setup_t *setup, const clause_t *c);
bool setup_inconsistent(setup_t *setup, int k);
bool setup_entails(setup_t *setup, const clause_t *c, const int k);

#endif

