// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * Constructors and evaluation procedure for queries.
 * Queries consist of the following elements: (in)equality of standard names,
 * disjunction, conjunction, negation, existential quantification, universal
 * quantification, and actions.
 *
 * Queries are created by query_[n]eq(), query_[atom|lit](), query_neg(),
 * query_[and|or](), query_[exists|forall](), and query_act().
 * Note that queries are not allowed to quantify over actions.
 * I think the only reason is that query_ennf_zs() wouldn't work correctly
 * otherwise.
 *
 * Queries are evaluated against a context, which encapsulates, for example, the
 * setup and the current situation. The context is initialized through
 * [k|b]context_init(), whose arguments must live as long as the context or
 * copies of the context created by context_copy().
 * The current situation can be changed through context_add_action() and
 * context_prev().
 *
 * Query evaluation is done with query_entailed(). As a first step, this
 * computes the setup for the given BAT. In case the query mentions standard
 * names or more variables than the previous queries, it updates the internal
 * structures, which may be expensive. These checks are skipped iff
 * force_no_update = true, which, however, is obviously dangerous.
 *
 * The query evaluation procedure differs a bit from the KR paper about ESL.
 * We first convert the formula into ENNF (extended negation normal form), that
 * is, all actions and negations are pushed inwards. If USE_QUERY_CNF is
 * defined, quantifiers are replaced with disjunctions and
 * conjunctions, respectively, over a set of standard names.
 * Then the query is decomposed and disjunctions of literals are taken as
 * clauses for which subsumption is tested.
 * If USE_QUERY_CNF is defined, the query is first converted to CNF and each of
 * the clauses is tested for subsumption, however, the CNF may be exponential in
 * size of the original formula.
 * During that procedure we treat SF literals just like normal split literals.
 * We therefore add them to the PEL set and setup_with_splits_and_sf_subsumes()
 * from setup.h will split them after splitting k non-SF literals. This
 * treatment should retain equivalence to the ESL paper.
 *
 * To simplify this the procedure which converts a formula to CNF conversion,
 * we directly implement conjunctions, whereas the paper just defines
 * conjunction to be a negation of a disjunction.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#ifndef _QUERY_H_
#define _QUERY_H_

#include "setup.h"
#include "belief.h"

typedef struct query query_t;

typedef struct {
    // The following attributes represent the context of the setup:
    // The BAT plus the already executed actions and their sensing results.
    const bool is_belief;
    const int belief_k;
    const univ_clauses_t *static_bat;
    const belief_conds_t *beliefs;
    const box_univ_clauses_t *dynamic_bat;
    const stdvec_t *situation;
    const bitmap_t *sensings;
    // The following attributes are stored for caching purposes.
    stdset_t query_names;
    int query_n_vars;
    stdvecset_t query_zs;
    stdset_t hplus;
    setup_t dynamic_setup;
    union {
        struct {
            setup_t static_setup;
            setup_t setup;
        } k;
        struct {
            bsetup_t static_setups;
            bsetup_t setups;
        } b;
    } u;
} context_t;

context_t kcontext_init(
        const univ_clauses_t *static_bat,
        const box_univ_clauses_t *dynamic_bat);
context_t bcontext_init(
        const univ_clauses_t *static_bat,
        const belief_conds_t *beliefs,
        const box_univ_clauses_t *dynamic_bat,
        const int belief_k);
context_t context_copy(const context_t *ctx);

void context_add_action(context_t *ctx, const stdname_t n, bool r);
void context_prev(context_t *ctx);

bool query_entailed(
        context_t *ctx,
        const bool force_no_update,
        const query_t *phi,
        const int k);

const query_t *query_eq(stdname_t n1, stdname_t n2);
const query_t *query_neq(stdname_t n1, stdname_t n2);
const query_t *query_lit(const literal_t *l);
const query_t *query_neg(const query_t *phi);
const query_t *query_or(const query_t *phi1, const query_t *phi2);
const query_t *query_and(const query_t *phi1, const query_t *phi2);
const query_t *query_impl(const query_t *phi1, const query_t *phi2);
const query_t *query_equiv(const query_t *phi1, const query_t *phi2);
const query_t *query_exists(var_t x, const query_t *phi);
const query_t *query_forall(var_t x, const query_t *phi);
const query_t *query_act(term_t n, const query_t *phi);

#endif

