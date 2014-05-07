// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "query.h"
#include "memory.h"
#include <assert.h>
#include <stdlib.h>

enum query_type { EQ, NEQ, LIT, OR, AND, NEG, EXISTS, FORALL, ACT, EVAL };

typedef struct { term_t t1; term_t t2; } query_names_t;
typedef struct { const query_t *phi; } query_unary_t;
typedef struct { const query_t *phi1; const query_t *phi2; } query_binary_t;
typedef struct { var_t var; const query_t *phi; } query_quantified_t;
typedef struct { term_t n; const query_t *phi; } query_action_t;
typedef struct {
    int (*n_vars)(void *);
    stdset_t (*names)(void *);
    bool (*eval)(const stdvec_t *, const splitset_t *, void *);
    void *arg;
    stdvec_t context_z;
    bool sign;
} query_eval_t;

struct query {
    enum query_type type;
    union {
        literal_t lit;
        query_names_t eq;
        query_binary_t bin;
        query_unary_t un;
        query_quantified_t qtf;
        query_action_t act;
        query_eval_t eval;
    } u;
};

static int query_n_vars(const query_t *phi)
{
    switch (phi->type) {
        case EQ:
        case NEQ:
        case LIT:
            return 0;
        case OR:
        case AND:
            return query_n_vars(phi->u.bin.phi1) +
                query_n_vars(phi->u.bin.phi2);
        case NEG:
            return query_n_vars(phi->u.un.phi);
        case EXISTS:
        case FORALL:
            return 1 + query_n_vars(phi->u.qtf.phi);
        case ACT:
            return query_n_vars(phi->u.act.phi);
        case EVAL:
            return phi->u.eval.n_vars(phi->u.eval.arg);
    }
    abort();
}

static stdset_t query_names(const query_t *phi)
{
    switch (phi->type) {
        case EQ:
        case NEQ: {
            stdset_t set = stdset_init_with_size(2);
            stdset_add(&set, phi->u.eq.t1);
            stdset_add(&set, phi->u.eq.t2);
            return set;
        }
        case LIT: {
            const stdvec_t *z = literal_z(&phi->u.lit);
            const stdvec_t *args = literal_args(&phi->u.lit);
            stdset_t set = stdset_init_with_size(
                    stdvec_size(z) + stdvec_size(args));
            for (EACH_CONST(stdvec, z, i)) {
                stdset_add(&set, i.val);
            }
            for (EACH_CONST(stdvec, args, i)) {
                stdset_add(&set, i.val);
            }
            return set;
        }
        case OR:
        case AND: {
            stdset_t set1 = query_names(phi->u.bin.phi1);
            const stdset_t set2 = query_names(phi->u.bin.phi2);
            stdset_add_all(&set1, &set2);
            return set1;
        }
        case NEG:
            return query_names(phi->u.un.phi);
        case EXISTS:
        case FORALL: {
            const query_t *phi1 = phi->u.qtf.phi;
            const query_t *phi2 = phi->u.qtf.phi;
            stdset_t set1 = query_names(phi1);
            stdset_t set2 = query_names(phi2);
            stdset_remove(&set1, 1);
            stdset_remove(&set2, 2);
            return stdset_union(&set1, &set2);
        }
        case ACT: {
            stdset_t set = query_names(phi->u.act.phi);
            stdset_add(&set, phi->u.act.n);
            return set;
        }
        case EVAL:
            return phi->u.eval.names(phi->u.eval.arg);
    }
    abort();
}

static const query_t *query_substitute(const query_t *phi, var_t x, stdname_t n)
{
    switch (phi->type) {
        case EQ:
        case NEQ: {
            const term_t t1 = phi->u.eq.t1;
            const term_t t2 = phi->u.eq.t2;
            if (x != t1 && x != t2) {
                return phi;
            }
            return query_eq(t1 == x ? n : t1, t2 == x ? n : t2);
        }
        case LIT: {
            const literal_t *l = &phi->u.lit;
            if (literal_is_ground(l)) {
                return phi;
            } else {
                stdvec_t *z = MALLOC(sizeof(stdvec_t));
                *z = stdvec_lazy_copy(literal_z(l));
                for (EACH(stdvec, z, i)) {
                    if (i.val == x) {
                        stdvec_iter_replace(&i, n);
                    }
                }
                stdvec_t *args = MALLOC(sizeof(stdvec_t));
                *args = stdvec_lazy_copy(literal_args(l));
                for (EACH(stdvec, args, i)) {
                    if (i.val == x) {
                        stdvec_iter_replace(&i, n);
                    }
                }
                literal_t *l = MALLOC(sizeof(literal_t));
                *l = literal_init(z, literal_sign(l), literal_pred(l), args);
                return query_lit(l);
            }
        }
        case OR:
            return query_or(query_substitute(phi->u.bin.phi1, x, n),
                    query_substitute(phi->u.bin.phi2, x, n));
        case AND:
            return query_and(query_substitute(phi->u.bin.phi1, x, n),
                    query_substitute(phi->u.bin.phi2, x, n));
        case NEG:
            return query_neg(query_substitute(phi->u.un.phi, x, n));
        case EXISTS:
            if (x != phi->u.qtf.var) {
                return query_exists(phi->u.qtf.var,
                        query_substitute(phi->u.qtf.phi, x, n));
            } else {
                return phi;
            }
        case FORALL:
            if (x != phi->u.qtf.var) {
                return query_forall(phi->u.qtf.var,
                        query_substitute(phi->u.qtf.phi, x, n));
            } else {
                return phi;
            }
        case ACT: {
            term_t a = x == phi->u.act.n ? n : phi->u.act.n;
            return query_act(a, phi->u.act.phi);
        }
        case EVAL:
            // TODO Currently no quantifying-in!
            // Could be fixed by managing varmap and as part of EVAL structs.
            return phi;
    }
    abort();
}

#ifdef USE_QUERY_CNF
static const query_t *query_ground_quantifier(bool existential,
        const query_t *phi, var_t x, const const stdset_t *hplus, int i)
{
    assert(0 <= i && i <= stdset_size(hplus));
    const query_t *psi1 = query_substitute(phi, x, stdset_get(hplus, i));
    if (i + 1 == stdset_size(hplus)) {
        return psi1;
    } else {
        const query_t *psi2 = query_ground_quantifier(existential, phi,
                x, hplus, i + 1);
        query_t *xi = MALLOC(sizeof(query_t));
        *xi = (query_t) {
            .type = existential ? OR : AND,
            .u.bin = (query_binary_t) { .phi1 = psi1, .phi2 = psi2 }
        };
        return xi;
    }
}
#endif

static const query_t *query_ennf_h(
        const stdvec_t *z,
        const query_t *phi,
        const stdset_t *hplus,
        const bool sign)
{
    // ENNF stands for Extended Negation Normal Form and means that
    // (1) actions are moved inwards to the extended literals
    // (2) negations are moved inwards to the extended literals
    // (3) quantifiers are grounded.
    //
    // The resulting formula only consists of the following elements:
    // EQ, NEQ, LIT, OR, AND[, EXISTS, FORALL], EVAL
    // where EXISTS and FORALL do not occur when USE_QUERY_CNF is defined.
    switch (phi->type) {
        case EQ:
        case NEQ: {
            if (sign) {
                return phi;
            } else {
                query_t *psi = MALLOC(sizeof(query_t));
                *psi = (query_t) {
                    .type = (phi->type == EQ) == sign ? EQ : NEQ,
                    .u.eq = phi->u.eq
                };
                return psi;
            }
        }
        case LIT: {
            if (sign && stdvec_size(z) == 0) {
                return phi;
            } else {
                query_t *psi = MALLOC(sizeof(query_t));
                *psi = *phi;
                if (!sign) {
                    psi->u.lit = literal_flip(&phi->u.lit);
                }
                if (stdvec_size(z) > 0) {
                    psi->u.lit = literal_prepend_all(z, &phi->u.lit);
                }
                return psi;
            }
        }
        case OR:
        case AND: {
            const query_t *psi1 = query_ennf_h(z, phi->u.bin.phi1, hplus, sign);
            const query_t *psi2 = query_ennf_h(z, phi->u.bin.phi2, hplus, sign);
            query_t *psi = MALLOC(sizeof(query_t));
            *psi = (query_t) {
                .type = (phi->type == OR) == sign ? OR : AND,
                .u.bin = (query_binary_t) { .phi1 = psi1, .phi2 = psi2 }
            };
            return psi;
        }
        case NEG: {
            return query_ennf_h(z, phi->u.un.phi, hplus, !sign);
        }
#ifdef USE_QUERY_CNF
        case EXISTS: {
            const bool is_existential = sign;
            const query_t *psi = query_ground_quantifier(is_existential,
                    phi->u.qtf.phi, phi->u.qtf.var, hplus, 0);
            return query_ennf_h(z, psi, hplus, sign);
        }
        case FORALL: {
            const bool is_existential = !sign;
            const query_t *psi = query_ground_quantifier(is_existential,
                    phi->u.qtf.phi, phi->u.qtf.var, hplus, 0);
            return query_ennf_h(z, psi, hplus, sign);
        }
#else
        case EXISTS:
        case FORALL: {
            query_t *psi = MALLOC(sizeof(query_t));
            *psi = (query_t) {
                .type = (phi->type == EXISTS) == sign ? EXISTS : FORALL,
                .u.qtf = (query_quantified_t) {
                    .var = phi->u.qtf.var,
                    .phi = query_ennf_h(z, phi->u.qtf.phi, hplus, sign)
                }
            };
            return psi;
        }
#endif
        case ACT: {
            const stdvec_t zz = stdvec_copy_append(z, phi->u.act.n);
            return query_ennf_h(&zz, phi->u.act.phi, hplus, sign);
        }
        case EVAL: {
            query_t *psi = MALLOC(sizeof(query_t));
            *psi = *phi;
            psi->u.eval.context_z = stdvec_lazy_copy(z);
            if (!sign) {
                psi->u.eval.sign = !psi->u.eval.sign;
            }
            return psi;
        }
    }
    abort();
}

static const query_t *query_ennf(const stdvec_t *context_z, const query_t *phi,
        const stdset_t *hplus)
{
    return query_ennf_h(context_z, phi, hplus, true);
}

static stdvecset_t query_ennf_zs(const query_t *phi)
{
    switch (phi->type) {
        case EQ:
        case NEQ:
            return stdvecset_init_with_size(0);
        case LIT:
            return stdvecset_singleton(literal_z(&phi->u.lit));
        case OR:
        case AND: {
            stdvecset_t l = query_ennf_zs(phi->u.bin.phi1);
            stdvecset_t r = query_ennf_zs(phi->u.bin.phi2);
            return stdvecset_union(&l, &r);
        }
        case NEG:
            return query_ennf_zs(phi->u.un.phi);
#ifdef USE_QUERY_CNF
        case EXISTS:
        case FORALL:
            abort();
#else
        case EXISTS:
        case FORALL:
            // variables don't quantify over actions
            return query_ennf_zs(phi->u.qtf.phi);
#endif
        case ACT:
            abort();
        case EVAL:
            return stdvecset_init_with_size(0);
    }
    abort();
}

static const query_t *query_simplify(
        const splitset_t *context_sf,
        const query_t *phi,
        bool *truth_value)
{
    // Removes standard name (in)equalities from the formula.
    //
    // The given formula must mention no other elements but:
    // EQ, NEQ, LIT, OR, AND, EXISTS, FORALL, EVAL
    //
    // The resulting formula only consists of the following elements:
    // LIT, OR, AND[, EXISTS, FORALL]
    // where EXISTS and FORALL only occur if they occurred in the given formula.
    switch (phi->type)
        case EQ: {
            *truth_value = phi->u.eq.t1 == phi->u.eq.t2;
            return NULL;
        case NEQ:
            *truth_value = phi->u.eq.t1 != phi->u.eq.t2;
            return NULL;
        case LIT:
            return phi;
        case OR:
        case AND: {
            query_t *psi = MALLOC(sizeof(query_t));
            psi->type = phi->type;
            psi->u.bin.phi1 = query_simplify(context_sf, phi->u.bin.phi1,
                    truth_value);
            if (psi->u.bin.phi1 == NULL && (psi->type == OR) == *truth_value) {
                return NULL;
            }
            psi->u.bin.phi2 = query_simplify(context_sf, phi->u.bin.phi2,
                    truth_value);
            if (psi->u.bin.phi2 == NULL && (psi->type == OR) == *truth_value) {
                return NULL;
            }
            if (psi->u.bin.phi1 == NULL) {
                return psi->u.bin.phi2;
            } else if (psi->u.bin.phi2 == NULL) {
                return psi->u.bin.phi1;
            } else {
                return psi;
            }
        }
        case NEG: {
            query_t *psi = MALLOC(sizeof(query_t));
            psi->type = phi->type;
            psi->u.un.phi = query_simplify(context_sf, phi->u.un.phi,
                    truth_value);
            if (psi->u.un.phi == NULL) {
                return NULL;
            }
            return psi;
        }
        case EXISTS:
        case FORALL: {
            query_t *psi = MALLOC(sizeof(query_t));
            psi->type = phi->type;
            psi->u.qtf.phi = query_simplify(context_sf, phi->u.qtf.phi,
                    truth_value);
            if (psi->u.qtf.phi == NULL) {
                return NULL;
            }
            return psi;
        }
        case ACT:
            abort();
        case EVAL:
            *truth_value = phi->u.eval.eval(&phi->u.eval.context_z,
                    context_sf, phi->u.eval.arg) == phi->u.eval.sign;
            return NULL;
    }
    abort();
}

#ifdef USE_QUERY_CNF
SET_ALIAS(cnf, setup, clause_t *);

static cnf_t query_cnf(const query_t *phi)
{
    // The given formula must mention no other elements but:
    // LIT, OR, AND
    switch (phi->type) {
        case LIT: {
            literal_t *l = MALLOC(sizeof(literal_t));
            *l = phi->u.lit;
            clause_t *c = MALLOC(sizeof(clause_t));
            *c = clause_singleton(l);
            return cnf_singleton(c);
        }
        case OR: {
            cnf_t cnf1 = query_cnf(phi->u.bin.phi1);
            cnf_t cnf2 = query_cnf(phi->u.bin.phi2);
            cnf_t cnf = cnf_init_with_size(cnf_size(&cnf1) * cnf_size(&cnf2));
            for (EACH_CONST(cnf, &cnf1, i)) {
                for (EACH_CONST(cnf, &cnf2, j)) {
                    const clause_t *c1 = i.val;
                    const clause_t *c2 = j.val;
                    clause_t *c = MALLOC(sizeof(clause_t));
                    *c = clause_union(c1, c2);
                    cnf_add(&cnf, c);
                }
            }
            return cnf;
        }
        case AND: {
            cnf_t cnf1 = query_cnf(phi->u.bin.phi1);
            cnf_t cnf2 = query_cnf(phi->u.bin.phi2);
            return cnf_union(&cnf1, &cnf2);
        }
        case EQ:
        case NEQ:
        case NEG:
        case EXISTS:
        case ACT:
        case EVAL:
            abort();
    }
    abort();
}
#endif

static void context_rebuild_setup(context_t *ctx)
{
    if (!ctx->is_belief) {
        ctx->u.k.setup = setup_union(&ctx->u.k.static_setup,
                &ctx->dynamic_setup);
        setup_add_sensing_results(&ctx->u.k.setup, ctx->context_sf);
        setup_propagate_units(&ctx->u.k.setup);
        //setup_minimize(&ctx->u.k.setup);
        ctx->u.k.pel = setup_pel(&ctx->u.k.setup);
    } else {
        ctx->u.b.setups = bsetup_unions(&ctx->u.b.static_setups,
                &ctx->dynamic_setup);
        bsetup_add_sensing_results(&ctx->u.b.setups, ctx->context_sf);
        bsetup_propagate_units(&ctx->u.b.setups);
        //bsetup_minimize(&ctx->u.b.setups);
        ctx->u.b.pels = bsetup_pels(&ctx->u.b.setups);
    }
}

static context_t context_init(
        const bool is_belief,
        const univ_clauses_t *static_bat,
        const belief_conds_t *beliefs,
        const box_univ_clauses_t *dynamic_bat,
        const int belief_k,
        const stdvec_t *context_z,
        const splitset_t *context_sf)
{
    assert(stdvec_size(context_z) == splitset_size(context_sf));
    context_t ctx = (context_t) {
        .is_belief   = is_belief,
        .belief_k    = belief_k,
        .static_bat  = static_bat,
        .beliefs     = beliefs,
        .dynamic_bat = dynamic_bat,
        .context_z   = context_z,
        .context_sf  = context_sf,
    };

    ctx.query_names = stdset_init_with_size(0);
    ctx.query_n_vars = 0;
    ctx.query_zs = stdvecset_singleton(context_z);
    if (!is_belief) {
        ctx.hplus = bat_hplus(static_bat, dynamic_bat,
                &ctx.query_names, ctx.query_n_vars);
    } else {
        ctx.hplus = bbat_hplus(static_bat, beliefs, dynamic_bat,
                &ctx.query_names, ctx.query_n_vars);
    }
    setup_t static_setup = setup_init_static(static_bat, &ctx.hplus);
    ctx.dynamic_setup = setup_init_dynamic(dynamic_bat, &ctx.hplus,
            &ctx.query_zs);
    if (!is_belief) {
        ctx.u.k.static_setup = static_setup;
    } else {
        ctx.u.b.static_setups = bsetup_init_beliefs(&static_setup, beliefs,
                &ctx.hplus, belief_k);
    }
    context_rebuild_setup(&ctx);
    return ctx;
}

context_t kcontext_init(
        const univ_clauses_t *static_bat,
        const box_univ_clauses_t *dynamic_bat,
        const stdvec_t *context_z,
        const splitset_t *context_sf)
{
    return context_init(false, static_bat, NULL, dynamic_bat, 0,
            context_z, context_sf);
}

context_t bcontext_init(
        const univ_clauses_t *static_bat,
        const belief_conds_t *beliefs,
        const box_univ_clauses_t *dynamic_bat,
        const int belief_k,
        const stdvec_t *context_z,
        const splitset_t *context_sf)
{
    return context_init(true, static_bat, beliefs, dynamic_bat, belief_k,
            context_z, context_sf);
}

context_t context_copy(const context_t *ctx)
{
    context_t copy = (context_t) {
        .is_belief     = ctx->is_belief,
        .belief_k      = ctx->belief_k,
        .static_bat    = ctx->static_bat,
        .beliefs       = ctx->beliefs,
        .dynamic_bat   = ctx->dynamic_bat,
        .context_z     = ctx->context_z,
        .context_sf    = ctx->context_sf,
        .query_names   = stdset_copy(&ctx->query_names),
        .query_n_vars  = ctx->query_n_vars,
        .query_zs      = stdvecset_copy(&ctx->query_zs),
        .hplus         = stdset_copy(&ctx->hplus),
        .dynamic_setup = setup_copy(&ctx->dynamic_setup)
    };
    if (!ctx->is_belief) {
        copy.u.k.static_setup = setup_copy(&ctx->u.k.static_setup);
        copy.u.k.setup = setup_copy(&ctx->u.k.setup);
        copy.u.k.pel = pelset_copy(&ctx->u.k.pel);
    } else {
        copy.u.b.static_setups = bsetup_deep_copy(&ctx->u.b.static_setups);
        copy.u.b.setups = bsetup_deep_copy(&ctx->u.b.setups);
        copy.u.b.pels = pelsets_deep_copy(&ctx->u.b.pels);
    }
    return copy;
}

void context_add_actions(
        context_t *ctx,
        const stdvec_t *add_context_z,
        const splitset_t *add_context_sf)
{
    assert(stdvec_size(add_context_z) == splitset_size(add_context_sf));
    ctx->context_z = ({
        stdvec_t *context_z = MALLOC(sizeof(stdvec_t));
        *context_z = stdvec_concat(ctx->context_z, add_context_z);
        context_z;
    });
    ctx->context_sf = ({
        splitset_t *context_sf = MALLOC(sizeof(splitset_t));
        *context_sf = splitset_union(ctx->context_sf, add_context_sf);
        context_sf;
    });
    ctx->dynamic_setup = setup_init_dynamic(ctx->dynamic_bat, &ctx->hplus,
                &ctx->query_zs);

    context_rebuild_setup(ctx);
}

void context_prev(context_t *ctx)
{
    assert(stdvec_size(ctx->context_z) == splitset_size(ctx->context_sf));
    const int size = stdvec_size(ctx->context_z);
    assert(size >= 1);
    ctx->context_z = ({
        stdvec_t *context_z = MALLOC(sizeof(stdvec_t));
        *context_z = stdvec_lazy_copy_range(ctx->context_z, 0, size - 1);
        context_z;
    });
    ctx->context_sf = ({
        splitset_t *context_sf = MALLOC(sizeof(splitset_t));
        *context_sf = splitset_lazy_copy(ctx->context_sf);
        context_sf;
    });
    ctx->query_zs = stdvecset_lazy_copy(&ctx->query_zs);
    ctx->dynamic_setup = setup_init_dynamic(ctx->dynamic_bat, &ctx->hplus,
                &ctx->query_zs);

    context_rebuild_setup(ctx);
}

void context_remove_undone_sf(context_t *ctx)
{
    splitset_t *context_sf = MALLOC(sizeof(splitset_t));
    *context_sf = splitset_init_with_size(splitset_size(ctx->context_sf));
    for (int i = 1; i < stdvec_size(ctx->context_z); ++i) {
        const stdvec_t z = stdvec_lazy_copy_range(ctx->context_z, 0, i);
        const stdname_t n = stdvec_get(ctx->context_z, i);
        const stdvec_t args = stdvec_singleton(n);
        literal_t *l1 = MALLOC(sizeof(literal_t));
        literal_t *l2 = MALLOC(sizeof(literal_t));
        *l1 = literal_init(&z, true, SF, &args);
        *l2 = literal_flip(l1);
        if (splitset_contains(ctx->context_sf, l1)) {
            splitset_add(context_sf, l1);
        }
        if (splitset_contains(ctx->context_sf, l2)) {
            splitset_add(context_sf, l2);
        }
    }
    ctx->context_sf = context_sf;
    ctx->dynamic_setup = setup_init_dynamic(ctx->dynamic_bat, &ctx->hplus,
                &ctx->query_zs);

    context_rebuild_setup(ctx);
}

#ifndef USE_QUERY_CNF
static bool clause_entailed(context_t *ctx, const clause_t *c, const int k)
{
    assert(c != NULL);
    if (!ctx->is_belief) {
        return setup_with_splits_and_sf_subsumes(&ctx->u.k.setup,
                &ctx->u.k.pel, c, k);
    } else {
        return bsetup_with_splits_and_sf_subsumes(&ctx->u.b.setups,
                &ctx->u.b.pels, c, k, NULL);
    }
}

static void query_entailed_h(context_t *ctx, const query_t *phi, const int k,
        clause_t **c, bool *r)
{
    // The given formula must mention no other elements but:
    // LIT, OR, AND
    switch (phi->type) {
        case EQ:
        case NEQ:
            abort();
        case LIT: {
            literal_t *l = MALLOC(sizeof(literal_t));
            *l = phi->u.lit;
            *c = MALLOC(sizeof(clause_t));
            **c = clause_singleton(l);
            *r = false;
            break;
        }
        case OR: {
            clause_t *c1, *c2;
            bool r1, r2;
            query_entailed_h(ctx, phi->u.bin.phi1, k, &c1, &r1);
            query_entailed_h(ctx, phi->u.bin.phi2, k, &c2, &r2);
            if (c1 != NULL && c2 != NULL) {
                *c = MALLOC(sizeof(clause_t));
                **c = clause_union(c1, c2);
            } else if (c1 != NULL && c2 == NULL) {
                *c = NULL;
                *r = r2 || clause_entailed(ctx, c1, k);
            } else if (c1 == NULL && c2 != NULL) {
                *c = NULL;
                *r = r1 || clause_entailed(ctx, c2, k);
            } else {
                *c = NULL;
                *r = r1 || r2;
            }
            break;
        }
        case AND: {
            clause_t *c1, *c2;
            bool r1, r2;
            query_entailed_h(ctx, phi->u.bin.phi1, k, &c1, &r1);
            query_entailed_h(ctx, phi->u.bin.phi2, k, &c2, &r2);
            if (c1 != NULL && c2 != NULL) {
                *c = NULL;
                *r = clause_entailed(ctx, c1, k) && clause_entailed(ctx, c2, k);
            } else if (c1 != NULL && c2 == NULL) {
                *c = NULL;
                *r = r2 && clause_entailed(ctx, c1, k);
            } else if (c1 == NULL && c2 != NULL) {
                *c = NULL;
                *r = r1 && clause_entailed(ctx, c2, k);
            } else {
                *c = NULL;
                *r = r1 && r2;
            }
            break;
        }
        case NEG:
            abort();
#ifdef USE_QUERY_CNF
        case EXISTS:
        case FORALL:
            abort();
#else
        case EXISTS:
        case FORALL: {
            *r = phi->type == FORALL;
            for (EACH_CONST(stdset, &ctx->hplus, i)) {
                const stdname_t n = i.val;
                const query_t *psi = query_substitute(phi->u.qtf.phi,
                        phi->u.qtf.var, n);
                query_entailed_h(ctx, psi, k, c, r);
                if (*c != NULL) {
                    *c = NULL;
                    *r = clause_entailed(ctx, *c, k);
                }
                if ((phi->type == EXISTS) == *r) {
                    break;
                }
            }
            break;
        }
#endif
        case ACT:
        case EVAL:
        default:
            abort();
    }
}
#endif

bool query_entailed(
        context_t *ctx,
        const bool force_no_update,
        const query_t *phi,
        const int k)
{
    // update hplus if necessary (needed for query rewriting and for setups)
    bool have_new_hplus = false;
    if (!force_no_update) {
        stdset_t ns = query_names(phi);
        const int nv = query_n_vars(phi);
        if (ctx->query_n_vars < nv) {
            if (!ctx->is_belief) {
                ctx->hplus = bat_hplus(ctx->static_bat,
                        ctx->dynamic_bat, &ns, nv);
            } else {
                ctx->hplus = bbat_hplus(ctx->static_bat, ctx->beliefs,
                        ctx->dynamic_bat, &ns, nv);
            }
            have_new_hplus = true;
        }
        if (!stdset_contains_all(&ctx->query_names, &ns)) {
            stdset_add_all(&ctx->query_names, &ns);
            stdset_add_all(&ctx->hplus, &ns);
            have_new_hplus = true;
        }
    }

    // prepare query, in some cases we have the result already
    bool truth_value;
    phi = query_ennf(ctx->context_z, phi, &ctx->hplus);
    phi = query_simplify(ctx->context_sf, phi, &truth_value);
    if (phi == NULL) {
        return truth_value;
    }

    // now update the setups if necessary
    bool have_new_static_setup = false;
    if (!force_no_update) {
        if (have_new_hplus) {
            setup_t static_setup = setup_init_static(ctx->static_bat,
                        &ctx->hplus);
            if (!ctx->is_belief) {
                ctx->u.k.static_setup = static_setup;
            } else {
                ctx->u.b.static_setups = bsetup_init_beliefs(&static_setup,
                        ctx->beliefs, &ctx->hplus, ctx->belief_k);
            }
            have_new_static_setup = true;
        }
    }
    bool have_new_dynamic_setup = false;
    if (!force_no_update) {
        stdvecset_t zs = query_ennf_zs(phi);
        if (have_new_hplus || !stdvecset_contains_all(&ctx->query_zs, &zs)) {
            ctx->query_zs = stdvecset_init_with_size(stdvecset_size(&zs));
            for (EACH_CONST(stdvecset, &zs, i)) {
                stdvec_t *z = MALLOC(sizeof(stdvec_t));
                *z = stdvec_copy(i.val);
                stdvecset_add(&ctx->query_zs, z);
            }
            ctx->dynamic_setup = setup_init_dynamic(ctx->dynamic_bat,
                    &ctx->hplus, &ctx->query_zs);
            have_new_dynamic_setup = true;
        }
    }
    if (have_new_static_setup || have_new_dynamic_setup) {
        context_rebuild_setup(ctx);
    }

#ifdef USE_QUERY_CNF
    cnf_t cnf = query_cnf(phi);
    truth_value = true;
    for (EACH_CONST(cnf, &cnf, i)) {
        if (!truth_value) {
            break;
        }
        const clause_t *c = i.val;
        if (!ctx->is_belief) {
            truth_value = setup_with_splits_and_sf_subsumes(&ctx->u.k.setup,
                    &ctx->u.k.pel, c, k);
        } else {
            truth_value = bsetup_with_splits_and_sf_subsumes(&ctx->u.b.setups,
                    &ctx->u.b.pels, c, k, NULL);
        }
    }
    return truth_value;
#else
    clause_t *c;
    query_entailed_h(ctx, phi, k, &c, &truth_value);
    return c != NULL ? clause_entailed(ctx, c, k) : truth_value;
#endif
}

const query_t *query_eq(term_t t1, term_t t2)
{
    return NEW(((query_t) {
        .type = EQ,
        .u.eq = (query_names_t) {
            .t1 = t1,
            .t2 = t2
        }
    }));
}

const query_t *query_neq(term_t t1, term_t t2)
{
    return NEW(((query_t) {
        .type = NEQ,
        .u.eq = (query_names_t) {
            .t1 = t1,
            .t2 = t2
        }
    }));
}

const query_t *query_lit(const literal_t *l)
{
    return NEW(((query_t) {
        .type = LIT,
        .u.lit = *l
    }));
}

const query_t *query_neg(const query_t *phi)
{
    return NEW(((query_t) {
        .type = NEG,
        .u.un = (query_unary_t) {
            .phi = phi
        }
    }));
}

const query_t *query_or(const query_t *phi1, const query_t *phi2)
{
    return NEW(((query_t) {
        .type = OR,
        .u.bin = (query_binary_t) {
            .phi1 = phi1,
            .phi2 = phi2
        }
    }));
}

const query_t *query_and(const query_t *phi1, const query_t *phi2)
{
    return NEW(((query_t) {
        .type = AND,
        .u.bin = (query_binary_t) {
            .phi1 = phi1,
            .phi2 = phi2
        }
    }));
}

const query_t *query_impl(const query_t *phi1, const query_t *phi2)
{
    return query_or(query_neg(phi1), phi2);
}

const query_t *query_equiv(const query_t *phi1, const query_t *phi2)
{
    return query_and(query_impl(phi1, phi2), query_impl(phi2, phi1));
}

const query_t *query_exists(var_t x, const query_t *phi)
{
    return NEW(((query_t) {
        .type = EXISTS,
        .u.qtf = (query_quantified_t) {
            .var = x,
            .phi = phi
        }
    }));
}

const query_t *query_forall(var_t x, const query_t *phi)
{
    return NEW(((query_t) {
        .type = FORALL,
        .u.qtf = (query_quantified_t) {
            .var = x,
            .phi = phi
        }
    }));
}

const query_t *query_act(term_t n, const query_t *phi)
{
    return NEW(((query_t) {
        .type = ACT,
        .u.act = (query_action_t) {
            .n = n,
            .phi = phi
        }
    }));
}

const query_t *query_eval(
        int (*n_vars)(void *),
        stdset_t (*names)(void *),
        bool (*eval)(const stdvec_t *, const splitset_t *, void *),
        void *arg)
{
    return NEW(((query_t) {
        .type = EVAL,
        .u.eval = (query_eval_t) {
            .n_vars = n_vars,
            .names = names,
            .eval = eval,
            .arg = arg,
            //.context_z = stdvec_init_with_size(0),
            .sign = true
        }
    }));
}

