// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * Query constructors and query evaluation procedure for queries.
 * Queries consist of the following elements: (in)equality of standard names,
 * disjunction, conjunction, negation, existential quantification, universal
 * quantification, actions, and evaluations (which are a vehicle for nested
 * queries, see below).
 *
 * Queries are created by query_[n]eq(), query_[atom|lit](), query_neg(),
 * query_[and|or](), query_[exists|forall](), query_act(), and query_eval().
 *
 * Query evaluation is done with query_entailed_by_bat().
 * As a first step, this computes the setup for the given BAT.
 * In the (usual) case of multiple queries wrt the same BAT, the functions
 * query_is_entailed_by_setup() is more efficient.
 * The context structure memorizes a some data needed for query answering;
 * for the initial context use context_init().
 *
 * To improve performance in cases where similar queries are evaluated wrt the
 * same BAT, we have the contexts which cache the setup etc.
 * The pointers handed over to context_init() must survive the context
 * lifecycle.
 *
 * The query evaluation procedure differs a bit from the KR paper about ESL.
 * We first convert the formula into ENNF (extended negation normal form), that
 * is, all actions and negations are pushed inwards. Additionally existentials
 * are replaced with disjunctions or conjunctions over a set of standard names.
 * Then we bring the resulting formual into CNF, which may be a bad idea.
 * Anyway, then we check all these clauses for subsumption.
 * During that procedure we treat SF literals just like normal split literals.
 * We therefore add them to the PEL set and increase the maximum split number
 * by their count.
 *
 * To simplify this the procedure which converts a formula to CNF conversion,
 * we directly implement conjunctions, whereas the paper just defines
 * conjunction to be a negation of a disjunction.
 *
 * The evaluation construct represent an opaque sub-query which can only be
 * evaluated at once. For that purpose it consists of an eval callback and a
 * void pointer arg for optional payload. Additionally one needs to provide
 * callbacks which compute the number of variables and the standard names in
 * this sub-query.
 * These evaluation constructs are evaluated already during query
 * simplification. This is somewhat odd, as it could be seen as an indication
 * that their evaluation always shall be cheap. (The other constructs
 * evaluated already during simplification are (in)equality of standard names.)
 * However, evaluation may be as complex as evaluating any other query by, say,
 * calling query_entailed_by_setup().
 * The standard use case is to evaluate the query wrt the same setup, which
 * means that just the splits are dropped.
 * Another use case for multi-agent reasoning is to evaluate the query wrt a
 * setup which represents what the `outer' agent knows about the `inner' agent.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#ifndef _QUERY_H_
#define _QUERY_H_

#include "setup.h"

typedef struct query query_t;

typedef struct {
    // The following attributes represent the context of the setup:
    // The BAT plus the already executed actions and their sensing results.
    const univ_clauses_t *static_bat;
    const box_univ_clauses_t *dynamic_bat;
    const stdvec_t *context_z;
    const splitset_t *context_sf;
    // The following attributes are stored for caching purposes.
    stdset_t query_names;
    int query_n_vars;
    stdset_t hplus;
    stdvecset_t query_zs;
    setup_t static_setup;
    setup_t dynamic_setup;
    setup_t setup;
    pelset_t setup_pel;
} context_t;

context_t context_init(
        const univ_clauses_t *static_bat,
        const box_univ_clauses_t *dynamic_bat,
        const stdvec_t *context_z,
        const splitset_t *context_sf);

void context_cleanup(context_t *setup);

bool query_entailed_by_setup(
        context_t *setup,
        const bool force_keep_setup,
        const query_t *phi,
        const int k);

bool query_entailed_by_bat(
        const univ_clauses_t *static_bat,
        const box_univ_clauses_t *dynamic_bat,
        const stdvec_t *context_z,
        const splitset_t *sensing_results,
        const query_t *phi,
        const int k);

const query_t *query_eq(stdname_t n1, stdname_t n2);
const query_t *query_neq(stdname_t n1, stdname_t n2);
const query_t *query_atom(pred_t p, stdvec_t args);
const query_t *query_lit(stdvec_t z, bool sign, pred_t p, stdvec_t args);
const query_t *query_neg(const query_t *phi);
const query_t *query_or(const query_t *phi1, const query_t *phi2);
const query_t *query_and(const query_t *phi1, const query_t *phi2);
const query_t *query_exists(const query_t *(phi)(stdname_t x));
const query_t *query_forall(const query_t *(phi)(stdname_t x));
const query_t *query_act(stdname_t n, const query_t *phi);
const query_t *query_eval(
        int (*n_vars)(void *arg),
        stdset_t (*names)(void *arg),
        bool (*eval)(const stdvec_t *context_z, void *arg),
        void *arg);

#endif

