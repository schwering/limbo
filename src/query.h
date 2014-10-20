// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * Constructors and evaluation procedure for queries.
 * Queries consist of the following elements: (in)equality of standard names,
 * disjunction, conjunction, negation, existential quantification, universal
 * quantification, and actions.
 *
 * Queries are created by query_[n]eq(), query_[atom|lit](), query_neg(),
 * query_[and|or](), query_[exists|forall](), and query_act().
 * Other than in standard ESL, actions may be variables. (That is fine here
 * because quantifiers are grounde, see below.)
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
 * We first convert the formula into ENNF (extended negation normal form), which
 * means that actions and negations are pushed inwards and quantifiers are
 * grounded. Quantifiers are grounded by replacing them with disjunctions (for
 * existentials) or conjunctions (for universals) over H+, the set of standard
 * names from the BAT and query plus one additional for each variable in the
 * query. Then, all equality expressions can be eliminated, because they only
 * involve standard names. The resulting query is converted to CNF and each of
 * the clauses is tested for subsumption using setup_entails().
 *
 * The conversion to CNF is necessary because, to make the decision procedure
 * deterministic, literals are only split in setup_entails(), that is, only
 * clause-wise. However, splitting literals across conjunctions or quantifiers
 * is sometimes necessary:
 * Example 1: Consider the tautology (p v q v (~p ^ ~q)). It can be shown in ESL
 * for k = 2 by splitting p and q and then decomposing the query (if both split
 * literals are negative, [~p] and [~q] hold and otherwise [p,q] holds). The
 * implementation computes the CNF ((p v q v ~p) ^ (p v q v ~q)) and then both
 * clauses can be shown even for k = 1.
 * Example 2: Given a KB (P(#1) v P(#2)), in ESL (E x . P(x)) follows because
 * we can first split P(#1) and then P(#2) and then proceed with the semantics
 * of the existential, which requires to show P(#1) or to show P(#2). The
 * implementation obtains (P(#1) v P(#2)) after grounding the query and thus
 * proves the query even for k = 0.
 *
 * Notice that SF literals due to action occurrences in the query are split
 * during setup_entails() as well.
 *
 * The above treatment helps to keep splitting locally in setup_entails() and
 * thus makes the decision procedure deterministic, and the quantifier grounding
 * allows to handle equality atoms.
 * However, nested beliefs will not be compatible with that treatment. For
 * example, consider ([n]p v [n]B(~p)) where SF(n) <-> p. This is a tautology
 * because SF(n) is split and retained within the nested belief operator.
 *
 * To simplify this the procedure which converts a formula to CNF conversion,
 * we directly implement conjunctions, whereas the paper just defines
 * conjunction to be a negation of a disjunction.
 *
 * A version of the decision procedure with more closely follows the paper but
 * is incorrect is still present but outcommented with #if 0 ... #endif in
 * query.c.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#ifndef _QUERY_H_
#define _QUERY_H_

#include "bitmap.h"
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
    int consistency_k;
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

void context_guarantee_consistency(context_t *ctx, const int k);

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

