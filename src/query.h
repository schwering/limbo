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
 * If necessary, this involves splitting k literals and adding the SF literals.
 * This of course increases complexity because we need to check positive and
 * negative outcomes for all split and SF literals.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#ifndef _QUERY_H_
#define _QUERY_H_

#include "setup.h"

typedef struct query query_t;

bool query_test(
        const box_univ_clauses_t *dynamic_bat,
        const univ_clauses_t *static_bat,
        const litset_t *sensing_results,
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

