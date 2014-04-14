// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "query.h"
#include "memory.h"
#include <assert.h>

enum query_type { EQ, NEQ, LIT, OR, AND, NEG, EX, ACT };

typedef struct { stdname_t n1; stdname_t n2; } query_names_t;
typedef struct { query_t *phi; } query_unary_t;
typedef struct { query_t *phi1; query_t *phi2; } query_binary_t;
typedef struct { query_t *(*phi)(stdname_t x); } query_exists_t;
typedef struct { stdname_t n; query_t *phi; } query_action_t;

struct query {
    enum query_type type;
    union {
        literal_t lit;
        query_names_t eq;
        query_names_t neq;
        query_binary_t or;
        query_binary_t and;
        query_unary_t neg;
        query_exists_t ex;
        query_action_t act;
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
            return query_n_vars(phi->u.or.phi1) +
                query_n_vars(phi->u.or.phi2);
        case AND:
            return query_n_vars(phi->u.and.phi1) +
                query_n_vars(phi->u.and.phi2);
        case NEG:
            return query_n_vars(phi->u.neg.phi);
        case EX: {
            query_t *psi = phi->u.ex.phi(1);
            return query_n_vars(psi);
        }
        case ACT:
            return query_n_vars(phi->u.act.phi);
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
        case OR: {
            stdset_t set1 = query_names(phi->u.or.phi1);
            const stdset_t set2 = query_names(phi->u.or.phi2);
            stdset_add_all(&set1, &set2);
            return set1;
        }
        case AND: {
            stdset_t set1 = query_names(phi->u.and.phi1);
            const stdset_t set2 = query_names(phi->u.and.phi2);
            stdset_add_all(&set1, &set2);
            return set1;
        }
        case NEG: {
            return query_names(phi->u.neg.phi);
        }
        case EX: {
            query_t *phi1 = phi->u.ex.phi(1);
            query_t *phi2 = phi->u.ex.phi(2);
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
    }
    assert(false);
    return stdset_init();
}

static query_t *query_ground_quantifier(bool existential,
        query_t *(*phi)(stdname_t n), const stdset_t *hplus, int i)
{
    assert(0 <= i && i <= stdset_size(hplus));
    query_t *psi1 = phi(stdset_get(hplus, i));
    if (i + 1 == stdset_size(hplus)) {
        return psi1;
    } else {
        query_t *xi = MALLOC(sizeof(query_t));
        query_t *psi2 = query_ground_quantifier(existential, phi, hplus, i + 1);
        if (existential) {
            xi->type = OR;
            xi->u.or = (query_binary_t) { .phi1 = psi1, .phi2 = psi2 };
        } else {
            xi->type = AND;
            xi->u.and = (query_binary_t) { .phi1 = psi1, .phi2 = psi2 };
        }
        return xi;
    }
}

static query_t *query_ennf_h(const stdvec_t *z, query_t *phi,
        const stdset_t *hplus, bool flip)
{
    // ENNF stands for Extended Negation Normal Form and means
    // (1) actions are moved inwards to the extended literals
    // (2) negations are moved inwards to the extended literals
    // (3) quantifiers are grounded
    //
    // The resulting formula only consists of the following elements:
    // EQ, NEQ, LIT, OR, AND
    switch (phi->type) {
        case EQ: {
            if (flip) {
                *phi = (query_t) {
                    .type = NEQ,
                    .u.neq = phi->u.eq
                };
            }
            return phi;
        }
        case NEQ: {
            if (flip) {
                *phi = (query_t) {
                    .type = NEQ,
                    .u.neq = phi->u.eq
                };
            }
            return phi;
        }
        case LIT: {
            if (flip) {
                phi->u.lit = literal_flip(&phi->u.lit);
            }
            if (stdvec_size(z) > 0) {
                phi->u.lit = literal_prepend_all(z, &phi->u.lit);
            }
            return phi;
        }
        case OR: {
            query_t *psi1 = query_ennf_h(z, phi->u.or.phi1, hplus, flip);
            query_t *psi2 = query_ennf_h(z, phi->u.or.phi2, hplus, flip);
            if (flip) {
                phi->type = AND;
                phi->u.and = (query_binary_t) { .phi1 = psi1, .phi2 = psi2 };
            } else {
                phi->type = OR;
                phi->u.or = (query_binary_t) { .phi1 = psi1, .phi2 = psi2 };
            }
            return phi;
        }
        case AND: {
            query_t *psi1 = query_ennf_h(z, phi->u.and.phi1, hplus, flip);
            query_t *psi2 = query_ennf_h(z, phi->u.and.phi2, hplus, flip);
            if (flip) {
                phi->type = OR;
                phi->u.or = (query_binary_t) { .phi1 = psi1, .phi2 = psi2 };
            } else {
                phi->type = AND;
                phi->u.and = (query_binary_t) { .phi1 = psi1, .phi2 = psi2 };
            }
            return phi;
        }
        case NEG: {
            query_t *psi = query_ennf_h(z, phi->u.neg.phi, hplus, !flip);
            return psi;
        }
        case EX: {
            const bool is_existential = !flip;
            query_t *psi = query_ground_quantifier(is_existential,
                    phi->u.ex.phi, hplus, 0);
            return query_ennf_h(z, psi, hplus, flip);
        }
        case ACT: {
            const stdvec_t zz = stdvec_copy_append(z, phi->u.act.n);
            query_t *psi = phi->u.act.phi;
            return query_ennf_h(&zz, psi, hplus, flip);
        }
    }
    assert(false);
    return NULL;
}

static query_t *query_ennf(const stdvec_t *context_z, query_t *phi,
        const stdset_t *hplus)
{
    return query_ennf_h(context_z, phi, hplus, false);
}

static stdvecset_t query_ennf_zs(query_t *phi)
{
    switch (phi->type) {
        case EQ:
        case NEQ:
            return stdvecset_init_with_size(0);
        case LIT: {
            return stdvecset_singleton(literal_z(&phi->u.lit));
        }
        case OR: {
            stdvecset_t l = query_ennf_zs(phi->u.or.phi1);
            stdvecset_t r = query_ennf_zs(phi->u.or.phi2);
            const stdvecset_t s = stdvecset_union(&l, &r);
            return s;
        }
        case AND: {
            stdvecset_t l = query_ennf_zs(phi->u.and.phi1);
            stdvecset_t r = query_ennf_zs(phi->u.and.phi2);
            const stdvecset_t s = stdvecset_union(&l, &r);
            return s;
        }
        case NEG:
            return query_ennf_zs(phi->u.neg.phi);
        case EX:
        case ACT:
            assert(false);
            return stdvecset_init_with_size(0);
    }
    assert(false);
    return stdvecset_init_with_size(0);
}

static query_t *query_simplify(query_t *phi, bool *truth_value)
{
    // Removes standard name (in)equalities from the formula.
    //
    // The given formula must mention no other elements but:
    // EQ, NEQ, LIT, OR, AND
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
            phi->u.or.phi1 = query_simplify(phi->u.or.phi1, truth_value);
            if (phi->u.or.phi1 == NULL && truth_value) {
                return NULL;
            }
            phi->u.or.phi2 = query_simplify(phi->u.or.phi2, truth_value);
            if (phi->u.or.phi2 == NULL && truth_value) {
                return NULL;
            }
            if (phi->u.or.phi1 == NULL) {
                query_t *psi = phi->u.or.phi2;
                return psi;
            } else if (phi->u.or.phi2 == NULL) {
                query_t *psi = phi->u.or.phi1;
                return psi;
            } else {
                return phi;
            }
        case AND:
            phi->u.and.phi1 = query_simplify(phi->u.and.phi1, truth_value);
            if (phi->u.and.phi1 == NULL && !truth_value) {
                return NULL;
            }
            phi->u.and.phi2 = query_simplify(phi->u.and.phi2, truth_value);
            if (phi->u.and.phi2 == NULL && !truth_value) {
                return NULL;
            }
            if (phi->u.and.phi1 == NULL) {
                query_t *psi = phi->u.and.phi2;
                return psi;
            } else if (phi->u.and.phi2 == NULL) {
                query_t *psi = phi->u.and.phi1;
                return psi;
            } else {
                return phi;
            }
        case NEG:
            phi->u.neg.phi = query_simplify(phi->u.neg.phi, truth_value);
            if (phi->u.neg.phi == NULL) {
                return NULL;
            }
            return phi;
        case EX:
            assert(false);
            return phi;
        case ACT:
            assert(false);
            return phi;
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
            cnf_t cnf1 = query_cnf(phi->u.or.phi1);
            cnf_t cnf2 = query_cnf(phi->u.or.phi2);
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
            cnf_t cnf1 = query_cnf(phi->u.or.phi1);
            cnf_t cnf2 = query_cnf(phi->u.or.phi2);
            cnf_t cnf = cnf_union(&cnf1, &cnf2);
            return cnf;
        }
        case EQ:
        case NEQ:
        case NEG:
        case EX:
        case ACT:
            assert(false);
            break;
    }
    assert(false);
}

static stdvecset_t clause_action_sequences_without_context(const clause_t *c,
        const stdvec_t *context_z)
{
    stdvecset_t zs = stdvecset_init();
    for (int i = 0; i < clause_size(c); ++i) {
        const stdvec_t *z = literal_z(clause_get(c, i));
        if (stdvec_is_prefix(z, context_z)) {
            continue;
        }
        for (int j = 0; j < stdvec_size(z); ++j) {
            stdvec_t *z_prefix = MALLOC(sizeof(stdvec_t));
            *z_prefix = stdvec_lazy_copy_range(z, 0, j);
            if (!stdvec_is_prefix(z_prefix, context_z)) {
                stdvecset_add(&zs, z_prefix);
            }
        }
    }
    return zs;
}

static bool query_test_sense(const setup_t *setup, litset_t *split,
        const pelset_t *pel, const clause_t *c, stdvecset_t *zs)
{
    if (setup_subsumes(setup, split, c)) {
        return true;
    }
    if (stdvecset_size(zs) > 0) {
        const stdvec_t *z = stdvecset_remove_index(zs, stdvecset_size(zs) - 1);
        if (stdvec_size(z) > 0) {
            stdvec_t z_prefix = stdvec_lazy_copy(z);
            const stdname_t n = stdvec_remove_last(&z_prefix);
            const stdvec_t n_vec = stdvec_singleton(n);
            const literal_t sf1 = literal_init(&z_prefix, true, SF, &n_vec);
            const literal_t sf2 = literal_flip(&sf1);
            litset_add(split, &sf1);
            bool r = query_test_sense(setup, split, pel, c, zs);
            litset_remove(split, &sf1);
            if (r) {
                litset_add(split, &sf2);
                r &= query_test_sense(setup, split, pel, c, zs);
                litset_remove(split, &sf2);
            }
            return r;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

static bool query_test_split(const setup_t *setup, litset_t *split,
        const pelset_t *pel, const stdvec_t *context_z, const clause_t *c,
        int k)
{
    if (setup_subsumes(setup, split, c)) {
        return true;
    }
    if (k > 0) {
        bool tried = false;
        for (int i = 0; i < pelset_size(pel); ++i) {
            const literal_t l1 = *pelset_get(pel, i);
            const literal_t l2 = literal_flip(&l1);
            if (litset_contains(split, &l1) || litset_contains(split, &l2)) {
                continue;
            }
            tried = true;
            litset_add(split, &l1);
            bool r = query_test_split(setup, split, pel, context_z, c, k-1);
            litset_remove(split, &l1);
            if (!r) {
                continue;
            }
            litset_add(split, &l2);
            r &= query_test_split(setup, split, pel, context_z, c, k-1);
            litset_remove(split, &l2);
            if (r) {
                return true;
            }
        }
        if (!tried) {
            stdvecset_t zs = clause_action_sequences_without_context(c,
                    context_z);
            return query_test_sense(setup, split, pel, c, &zs);
        }
        return false;
    } else {
        stdvecset_t zs = clause_action_sequences_without_context(c, context_z);
        return query_test_sense(setup, split, pel, c, &zs);
    }
}

static bool query_test_clause(const setup_t *setup, const pelset_t *pel,
        const stdvec_t *context_z, const clause_t *c, int k)
{
    litset_t split = litset_init_with_size(k);
    if (setup_subsumes(setup, &split, c)) {
        return true;
    }
    const bool r = query_test_split(setup, &split, pel, context_z, c, k);
    return r;
}

context_t context_init(
        const univ_clauses_t *static_bat,
        const box_univ_clauses_t *dynamic_bat,
        const stdvec_t *context_z,
        const litset_t *context_sf)
{
    stdset_t query_names = stdset_init_with_size(0);
    stdset_t hplus = bat_hplus(static_bat, dynamic_bat, &query_names, 0);
    stdvecset_t query_zs = stdvecset_singleton(context_z);
    setup_t static_setup = setup_init_static(static_bat, &hplus);
    setup_t dynamic_setup = setup_init_dynamic(dynamic_bat, &hplus, &query_zs);
    setup_t setup = setup_union(&static_setup, &dynamic_setup);
    setup_add_sensing_results(&setup, context_sf);
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
        bool force_keep_setup,
        query_t *phi,
        int k)
{
    // update hplus if necessary (needed for query rewriting and for setups)
    bool have_new_hplus = false;
    if (!force_keep_setup) {
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
    phi = query_simplify(phi, &truth_value);
    if (phi == NULL) {
        return truth_value;
    }

    // now update the setups if necessary
    bool have_new_static_setup = false;
    if (!force_keep_setup) {
        if (have_new_hplus) {
            ctx->static_setup = setup_init_static(ctx->static_bat,
                    &ctx->hplus);
            have_new_static_setup = true;
        }
    }
    bool have_new_dynamic_setup = false;
    if (!force_keep_setup) {
        stdvecset_t zs = query_ennf_zs(phi);
        stdvecset_contains_all(&ctx->query_zs, &zs);
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
        setup_add_sensing_results(&ctx->setup, ctx->context_sf);
        ctx->setup_pel = setup_pel(&ctx->setup);
    }

    cnf_t cnf = query_cnf(phi);
    truth_value = true;
    for (int i = 0; i < cnf_size(&cnf) && truth_value; ++i) {
        const clause_t *c = cnf_get(&cnf, i);
        pelset_t pel = pelset_lazy_copy(&ctx->setup_pel);
        add_pel_of_clause(&pel, c);
        truth_value = query_test_clause(&ctx->setup, &pel, ctx->context_z, c, k);
    }
    return truth_value;
}

bool query_entailed_by_bat(
        const univ_clauses_t *static_bat,
        const box_univ_clauses_t *dynamic_bat,
        const stdvec_t *context_z,
        const litset_t *context_sf,
        query_t *phi,
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
    phi = query_simplify(phi, &truth_value);
    if (phi == NULL) {
        return truth_value;
    }
    const setup_t setup = ({
        const stdvecset_t zs = query_ennf_zs(phi);
        setup_t s = setup_init_static_and_dynamic(static_bat, dynamic_bat,
            &hplus, &zs);
        setup_add_sensing_results(&s, context_sf);
        litset_t split = litset_init_with_size(0);
        setup_propagate_units(&s, &split);
        s;
    });
    pelset_t bat_pel = setup_pel(&setup);
    cnf_t cnf = query_cnf(phi);
    truth_value = true;
    for (int i = 0; i < cnf_size(&cnf) && truth_value; ++i) {
        const clause_t *c = cnf_get(&cnf, i);
        pelset_t pel = pelset_lazy_copy(&bat_pel);
        add_pel_of_clause(&pel, c);
        truth_value = query_test_clause(&setup, &pel, context_z, c, k);
    }
    return truth_value;
}

query_t *query_eq(stdname_t n1, stdname_t n2)
{
    return NEW(((query_t) {
        .type = EQ,
        .u.eq = (query_names_t) {
            .n1 = n1,
            .n2 = n2
        }
    }));
}

query_t *query_neq(stdname_t n1, stdname_t n2)
{
    return NEW(((query_t) {
        .type = NEQ,
        .u.neq = (query_names_t) {
            .n1 = n1,
            .n2 = n2
        }
    }));
}

query_t *query_lit(stdvec_t z, bool sign, pred_t p, stdvec_t args)
{
    return NEW(((query_t) {
        .type = LIT,
        .u.lit = literal_init(&z, sign, p, &args)
    }));
}

query_t *query_neg(query_t *phi)
{
    return NEW(((query_t) {
        .type = NEG,
        .u.neg = (query_unary_t) {
            .phi = phi
        }
    }));
}

query_t *query_or(query_t *phi1, query_t *phi2)
{
    return NEW(((query_t) {
        .type = OR,
        .u.or = (query_binary_t) {
            .phi1 = phi1,
            .phi2 = phi2
        }
    }));
}

query_t *query_and(query_t *phi1, query_t *phi2)
{
    return NEW(((query_t) {
        .type = AND,
        .u.and = (query_binary_t) {
            .phi1 = phi1,
            .phi2 = phi2
        }
    }));
}

query_t *query_exists(query_t *(phi)(stdname_t x))
{
    return NEW(((query_t) {
        .type = EX,
        .u.ex = (query_exists_t) {
            .phi = phi
        }
    }));
}

query_t *query_forall(query_t *(phi)(stdname_t x))
{
    return query_neg(query_exists(phi));
}

query_t *query_act(stdname_t n, query_t *phi)
{
    return NEW(((query_t) {
        .type = ACT,
        .u.act = (query_action_t) {
            .n = n,
            .phi = phi
        }
    }));
}

