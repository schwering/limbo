// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * Definitions and evaluation procedure for queries.
 *
 * The procedure differs a bit from the KR paper about ESL.
 * We first convert the formula into ENNF (extended negation normal form), that
 * is, all actions and negations are pushed inwards. Additionally existentials
 * are replaced with disjunctions or conjunctions over a set of standard names.
 * Then we bring the resulting formual into CNF, which may be a bad idea.
 * Anyway, then we check all these clauses for subsumption.
 * During that procedure we treat SF literals just like normal split literals.
 * We therefore add them to the PEL set and increase the maximum split number by
 * their count.
 *
 * To improve performance in cases where similar queries are evaluated wrt the
 * same BAT, we have the contexts which cache the setup etc.
 * The pointers handed over to context_init() must survive the context
 * lifecycle.
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
        bool force_keep_setup,
        query_t *phi,
        int k);

bool query_entailed_by_bat(
        const univ_clauses_t *static_bat,
        const box_univ_clauses_t *dynamic_bat,
        const stdvec_t *context_z,
        const splitset_t *sensing_results,
        query_t *phi,
        int k);

void query_free(query_t *phi);

query_t *query_eq(stdname_t n1, stdname_t n2);
query_t *query_neq(stdname_t n1, stdname_t n2);
query_t *query_lit(stdvec_t z, bool sign, pred_t p, stdvec_t args);
query_t *query_neg(query_t *phi);
query_t *query_or(query_t *phi1, query_t *phi2);
query_t *query_and(query_t *phi1, query_t *phi2);
query_t *query_exists(query_t *(phi)(stdname_t x));
query_t *query_forall(query_t *(phi)(stdname_t x));
query_t *query_act(stdname_t n, query_t *phi);

#endif

