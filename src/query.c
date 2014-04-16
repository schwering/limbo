// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "query.h"
#include "memory.h"
#include <assert.h>

enum query_type { EQ, NEQ, LIT, OR, AND, NEG, EX, ACT, EVAL };

typedef struct { stdname_t n1; stdname_t n2; } query_names_t;
typedef struct { const query_t *phi; } query_unary_t;
typedef struct { const query_t *phi1; const query_t *phi2; } query_binary_t;
typedef struct { const query_t *(*phi)(stdname_t x); } query_exists_t;
typedef struct { stdname_t n; const query_t *phi; } query_action_t;
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
        query_names_t neq;
        query_binary_t bin;
        query_unary_t un;
        query_exists_t ex;
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
        case EX:
            return query_n_vars(phi->u.ex.phi(1));
        case ACT:
            return query_n_vars(phi->u.act.phi);
        case EVAL:
            return phi->u.eval.n_vars(phi->u.eval.arg);
    }
    assert(false);
    return -1;
}

static stdset_t query_names(const query_t *phi)
{
    switch (phi->type) {
        case EQ: {
            stdset_t set = stdset_init_with_size(2);
            stdset_add(&set, phi->u.eq.n1);
            stdset_add(&set, phi->u.eq.n2);
            return set;
        }
        case NEQ: {
            stdset_t set = stdset_init_with_size(2);
            stdset_add(&set, phi->u.neq.n1);
            stdset_add(&set, phi->u.neq.n2);
            return set;
        }
        case LIT: {
            const stdvec_t *z = literal_z(&phi->u.lit);
            const stdvec_t *args = literal_args(&phi->u.lit);
            stdset_t set = stdset_init_with_size(
                    stdvec_size(z) + stdvec_size(args));
            for (int i = 0; i < stdvec_size(z); ++i) {
                stdset_add(&set, stdvec_get(z, i));
            }
            for (int i = 0; i < stdvec_size(args); ++i) {
                stdset_add(&set, stdvec_get(args, i));
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
        case EX: {
            const query_t *phi1 = phi->u.ex.phi(1);
            const query_t *phi2 = phi->u.ex.phi(2);
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
    assert(false);
    return stdset_init();
}

static const query_t *query_ground_quantifier(bool existential,
        const query_t *(*phi)(stdname_t n), const stdset_t *hplus, int i)
{
    assert(0 <= i && i <= stdset_size(hplus));
    const query_t *psi1 = phi(stdset_get(hplus, i));
    if (i + 1 == stdset_size(hplus)) {
        return psi1;
    } else {
        const query_t *psi2 = query_ground_quantifier(existential, phi,
                hplus, i + 1);
        query_t *xi = MALLOC(sizeof(query_t));
        *xi = (query_t) {
            .type = existential ? OR : AND,
            .u.bin = (query_binary_t) { .phi1 = psi1, .phi2 = psi2 }
        };
        return xi;
    }
}

static const query_t *query_ennf(
        const stdvec_t *context_z,
        const query_t *phi,
        const stdset_t *hplus);

static const query_t *query_ennf_h(
        const stdvec_t *z,
        const query_t *phi,
        const stdset_t *hplus,
        const bool flip)
{
    // ENNF stands for Extended Negation Normal Form and means
    // (1) actions are moved inwards to the extended literals
    // (2) negations are moved inwards to the extended literals
    // (3) quantifiers are grounded
    //
    // The resulting formula only consists of the following elements:
    // EQ, NEQ, LIT, OR, AND, EVAL
    switch (phi->type) {
        case EQ: {
            if (!flip) {
                return phi;
            } else {
                query_t *psi = MALLOC(sizeof(query_t));
                *psi = (query_t) {
                    .type = NEQ,
                    .u.neq = phi->u.eq
                };
                return psi;
            }
        }
        case NEQ: {
            if (!flip) {
                return phi;
            } else {
                query_t *psi = MALLOC(sizeof(query_t));
                *psi = (query_t) {
                    .type = NEQ,
                    .u.neq = phi->u.eq
                };
                return psi;
            }
        }
        case LIT: {
            if (!flip && stdvec_size(z) == 0) {
                return phi;
            } else {
                query_t *psi = MALLOC(sizeof(query_t));
                *psi = *phi;
                if (flip) {
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
            const query_t *psi1 = query_ennf_h(z, phi->u.bin.phi1, hplus, flip);
            const query_t *psi2 = query_ennf_h(z, phi->u.bin.phi2, hplus, flip);
            query_t *psi = MALLOC(sizeof(query_t));
            *psi = (query_t) {
                .type = (phi->type == OR) != flip ? OR : AND,
                .u.bin = (query_binary_t) { .phi1 = psi1, .phi2 = psi2 }
            };
            return psi;
        }
        case NEG: {
            return query_ennf_h(z, phi->u.un.phi, hplus, !flip);
        }
        case EX: {
            const bool is_existential = !flip;
            const query_t *psi = query_ground_quantifier(is_existential,
                    phi->u.ex.phi, hplus, 0);
            return query_ennf(z, psi, hplus);
        }
        case ACT: {
            const stdvec_t zz = stdvec_copy_append(z, phi->u.act.n);
            return query_ennf_h(&zz, phi->u.act.phi, hplus, flip);
        }
        case EVAL: {
            query_t *psi = MALLOC(sizeof(query_t));
            *psi = *phi;
            psi->u.eval.context_z = stdvec_lazy_copy(z);
            if (flip) {
                psi->u.eval.sign = !psi->u.eval.sign;
            }
            return psi;
        }
    }
    assert(false);
    return NULL;
}

static const query_t *query_ennf(const stdvec_t *context_z, const query_t *phi,
        const stdset_t *hplus)
{
    return query_ennf_h(context_z, phi, hplus, false);
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
        case EX:
        case ACT:
            assert(false);
            return stdvecset_init_with_size(0);
        case EVAL:
            return stdvecset_init_with_size(0);
    }
    assert(false);
    return stdvecset_init_with_size(0);
}

static const query_t *query_simplify(
        const splitset_t *context_sf,
        const query_t *phi,
        bool *truth_value)
{
    // Removes standard name (in)equalities from the formula.
    //
    // The given formula must mention no other elements but:
    // EQ, NEQ, LIT, OR, AND, EVAL
    //
    // The resulting formula only consists of the following elements:
    // LIT, OR, AND
    switch (phi->type)
        case EQ: {
            *truth_value = phi->u.eq.n1 == phi->u.eq.n2;
            return NULL;
        case NEQ:
            *truth_value = phi->u.neq.n1 != phi->u.neq.n2;
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
        case EX:
            assert(false);
            return phi;
        case ACT:
            assert(false);
            return phi;
        case EVAL:
            *truth_value = phi->u.eval.eval(&phi->u.eval.context_z,
                    context_sf, phi->u.eval.arg) == phi->u.eval.sign;
            return NULL;
    }
    assert(false);
}

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
            for (int i = 0; i < cnf_size(&cnf1); ++i) {
                for (int j = 0; j < cnf_size(&cnf2); ++j) {
                    const clause_t *c1 = cnf_get(&cnf1, i);
                    const clause_t *c2 = cnf_get(&cnf2, j);
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
        case EX:
        case ACT:
        case EVAL:
            assert(false);
            break;
    }
    assert(false);
}

static bool query_test_split(
        const setup_t *original_setup,
        const pelset_t *original_pel,
        setup_t *setup,
        pelset_t *pel,
        const stdvec_t *context_z,
        const clause_t *c,
        const int k)
{
    bool r = setup_subsumes(setup, c);
    if (r || k < 0) {
        return r;
    }
    for (int i = 0; i < pelset_size(pel); ++i) {
        const literal_t *l1 = pelset_get(pel, i);
        if ((literal_pred(l1) == SF) != (k == 0)) {
            continue;
        }
        const literal_t l2 = literal_flip(l1);
        pelset_remove_index(pel, i--);
        const clause_t c1 = clause_singleton(l1);
        const clause_t c2 = clause_singleton(&l2);
        const int k1 = (literal_pred(l1) == SF) ? k : k - 1;
        setup_t setup1 = setup_lazy_copy(setup);
        setup_add(&setup1, &c1);
        bool r;
        r = query_test_split(original_setup, original_pel, &setup1, pel,
                context_z, c, k1);
        if (!r) {
            continue;
        }
        setup_t setup2 = setup_lazy_copy(setup);
        setup_add(&setup2, &c2);
        r = query_test_split(original_setup, original_pel, &setup2, pel,
                context_z, c, k1);
        if (r) {
            return true;
        }
    }
    return false;
}

static bool query_test_clause(
        const setup_t *original_setup,
        const pelset_t *original_pel,
        const stdvec_t *context_z,
        const clause_t *c,
        const int k)
{
    setup_t setup = setup_lazy_copy(original_setup);
    // We split SF literals from imaginarily executed actions only when needed
    // in query_test_split(). For that, the PEL needs to contain the relevant SF
    // atoms, though.
    pelset_t pel_and_sf = pelset_lazy_copy(original_pel);
    for (int i = 0; i < clause_size(c); ++i) {
        const stdvec_t *z = literal_z(clause_get(c, i));
        for (int j = 0; j < stdvec_size(z) - 1; ++j) {
            const stdvec_t z_prefix = stdvec_lazy_copy_range(z, 0, j);
            const stdname_t n = stdvec_get(z, j);
            const stdvec_t n_vec = stdvec_singleton(n);
            literal_t *sf = MALLOC(sizeof(literal_t));
            *sf = literal_init(&z_prefix, true, SF, &n_vec);
            if (!setup_would_be_needless_split(&setup, sf)) {
                pelset_add(&pel_and_sf, sf);
            }
        }
    }
    return query_test_split(original_setup, original_pel, &setup,
            &pel_and_sf, context_z, c, k);
}

context_t context_init(
        const univ_clauses_t *static_bat,
        const box_univ_clauses_t *dynamic_bat,
        const stdvec_t *orig_context_z,
        const splitset_t *orig_context_sf)
{
    assert(stdvec_size(orig_context_z) == splitset_size(orig_context_sf));
    stdvec_t *context_z = MALLOC(sizeof(stdvec_t));
    *context_z = stdvec_copy(orig_context_z);
    splitset_t *context_sf = MALLOC(sizeof(splitset_t));
    *context_sf = splitset_copy(orig_context_sf);
    stdset_t query_names = stdset_init_with_size(0);
    stdset_t hplus = bat_hplus(static_bat, dynamic_bat, &query_names, 0);
    stdvecset_t query_zs = stdvecset_singleton(context_z);
    setup_t static_setup = setup_init_static(static_bat, &hplus);
    setup_t dynamic_setup = setup_init_dynamic(dynamic_bat, &hplus, &query_zs);
    setup_t setup = setup_union(&static_setup, &dynamic_setup);
    setup_add_sensing_results(&setup, context_sf);
    //setup_minimize(&setup);
    setup_propagate_units(&setup);
    return (context_t) {
        .static_bat    = static_bat,
        .dynamic_bat   = dynamic_bat,
        .context_z     = context_z,
        .context_sf    = context_sf,
        .query_names   = query_names,
        .query_n_vars  = 0,
        .hplus         = hplus,
        .query_zs      = query_zs,
        .static_setup  = static_setup,
        .dynamic_setup = dynamic_setup,
        .setup         = setup,
        .setup_pel     = setup_pel(&setup)
    };
}

context_t context_copy_with_new_actions(
        const context_t *ctx,
        const stdvec_t *add_context_z,
        const splitset_t *add_context_sf)
{
    assert(stdvec_size(add_context_z) == splitset_size(add_context_sf));
    stdvec_t *context_z = MALLOC(sizeof(stdvec_t));
    *context_z = stdvec_concat(ctx->context_z, add_context_z);
    splitset_t *context_sf = MALLOC(sizeof(splitset_t));
    *context_sf = splitset_union(ctx->context_sf, add_context_sf);
    assert(stdvec_size(context_z) == splitset_size(context_sf));
    stdvecset_t query_zs = stdvecset_singleton(context_z);
    setup_t dynamic_setup = setup_init_dynamic(ctx->dynamic_bat, &ctx->hplus,
            &query_zs);
    setup_t setup = setup_union(&ctx->static_setup, &dynamic_setup);
    setup_add_sensing_results(&setup, add_context_sf);
    //setup_minimize(&setup);
    setup_propagate_units(&setup);
    return (context_t) {
        .static_bat    = ctx->static_bat,
        .dynamic_bat   = ctx->dynamic_bat,
        .context_z     = context_z,
        .context_sf    = context_sf,
        .query_names   = stdset_copy(&ctx->query_names),
        .query_n_vars  = ctx->query_n_vars,
        .hplus         = stdset_copy(&ctx->hplus),
        .query_zs      = query_zs,
        .static_setup  = setup_copy(&ctx->static_setup),
        .dynamic_setup = dynamic_setup,
        .setup         = setup,
        .setup_pel     = setup_pel(&setup)
    };
}

void context_cleanup(context_t *ctx)
{
    stdset_cleanup(&ctx->query_names);
    stdset_cleanup(&ctx->hplus);
    stdvecset_cleanup(&ctx->query_zs);
    setup_cleanup(&ctx->static_setup);
    setup_cleanup(&ctx->dynamic_setup);
    setup_cleanup(&ctx->setup);
    pelset_cleanup(&ctx->setup_pel);
}

bool query_entailed_by_setup(
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
            ctx->hplus = bat_hplus(ctx->static_bat, ctx->dynamic_bat,
                    &ns, nv);
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
            ctx->static_setup = setup_init_static(ctx->static_bat,
                    &ctx->hplus);
            have_new_static_setup = true;
        }
    }
    bool have_new_dynamic_setup = false;
    if (!force_no_update) {
        stdvecset_t zs = query_ennf_zs(phi);
        if (have_new_hplus || !stdvecset_contains_all(&ctx->query_zs, &zs)) {
            ctx->query_zs = stdvecset_init_with_size(stdvecset_size(&zs));
            for (int i = 0; i < stdvecset_size(&zs); ++i) {
                stdvec_t *z = MALLOC(sizeof(stdvec_t));
                *z = stdvec_copy(stdvecset_get(&zs, i));
                stdvecset_add(&ctx->query_zs, z);
            }
            ctx->dynamic_setup = setup_init_dynamic(ctx->dynamic_bat,
                    &ctx->hplus, &ctx->query_zs);
            have_new_dynamic_setup = true;
        }
    }
    if (have_new_static_setup || have_new_dynamic_setup) {
        ctx->setup = setup_union(&ctx->static_setup, &ctx->dynamic_setup);
        //setup_minimize(&ctx->setup);
        setup_propagate_units(&ctx->setup);
        setup_add_sensing_results(&ctx->setup, ctx->context_sf);
        ctx->setup_pel = setup_pel(&ctx->setup);
    }

    cnf_t cnf = query_cnf(phi);
    truth_value = true;
    for (int i = 0; i < cnf_size(&cnf) && truth_value; ++i) {
        const clause_t *c = cnf_get(&cnf, i);
        pelset_t pel = pelset_lazy_copy(&ctx->setup_pel);
        add_pel_of_clause(&pel, c, &ctx->setup);
        truth_value = query_test_clause(&ctx->setup, &pel, ctx->context_z,
                c, k);
    }
    return truth_value;
}

bool query_entailed_by_bat(
        const univ_clauses_t *static_bat,
        const box_univ_clauses_t *dynamic_bat,
        const stdvec_t *context_z,
        const splitset_t *context_sf,
        const query_t *phi,
        int k)
{
    const stdset_t hplus = ({
        const stdset_t ns = query_names(phi);
        const int n_vars = query_n_vars(phi);
        stdset_t hplus = bat_hplus(static_bat, dynamic_bat, &ns, n_vars);
        stdset_add_all(&hplus, &ns);
        hplus;
    });
    bool truth_value;
    phi = query_ennf(context_z, phi, &hplus);
    phi = query_simplify(context_sf, phi, &truth_value);
    if (phi == NULL) {
        return truth_value;
    }
    const setup_t setup = ({
        const stdvecset_t zs = query_ennf_zs(phi);
        setup_t s = setup_init_static_and_dynamic(static_bat, dynamic_bat,
            &hplus, &zs);
        setup_add_sensing_results(&s, context_sf);
        s;
    });
    pelset_t bat_pel = setup_pel(&setup);
    cnf_t cnf = query_cnf(phi);
    truth_value = true;
    for (int i = 0; i < cnf_size(&cnf) && truth_value; ++i) {
        const clause_t *c = cnf_get(&cnf, i);
        pelset_t pel = pelset_lazy_copy(&bat_pel);
        add_pel_of_clause(&pel, c, &setup);
        truth_value = query_test_clause(&setup, &pel, context_z, c, k);
    }
    return truth_value;
}

const query_t *query_eq(stdname_t n1, stdname_t n2)
{
    return NEW(((query_t) {
        .type = EQ,
        .u.eq = (query_names_t) {
            .n1 = n1,
            .n2 = n2
        }
    }));
}

const query_t *query_neq(stdname_t n1, stdname_t n2)
{
    return NEW(((query_t) {
        .type = NEQ,
        .u.neq = (query_names_t) {
            .n1 = n1,
            .n2 = n2
        }
    }));
}

const query_t *query_atom(pred_t p, stdvec_t args)
{
    const stdvec_t z = stdvec_init_with_size(0);
    const bool sign = true;
    return query_lit(z, sign, p, args);;
}

const query_t *query_lit(stdvec_t z, bool sign, pred_t p, stdvec_t args)
{
    return NEW(((query_t) {
        .type = LIT,
        .u.lit = literal_init(&z, sign, p, &args)
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

const query_t *query_exists(const query_t *(phi)(stdname_t x))
{
    return NEW(((query_t) {
        .type = EX,
        .u.ex = (query_exists_t) {
            .phi = phi
        }
    }));
}

const query_t *query_forall(const query_t *(phi)(stdname_t x))
{
    return query_neg(query_exists(phi));
}

const query_t *query_act(stdname_t n, const query_t *phi)
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

