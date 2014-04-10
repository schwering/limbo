// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "setup.h"
#include "memory.h"
#include <assert.h>
#include <stdlib.h>

#define MAX(x,y)    ((x) > (y) ? (x) : (y))
#define SWAP(x,y)   ({ const typeof(x) tmp = x; x = y; y = tmp; })

SET_IMPL(litset, literal_t *, literal_cmp);
SET_IMPL(setup, clause_t *, clause_cmp);
VECTOR_IMPL(univ_clauses, univ_clause_t *, NULL);
VECTOR_IMPL(box_univ_clauses, box_univ_clause_t *, NULL);

const clause_t *clause_empty(void)
{
    static clause_t c;
    static clause_t *ptr = NULL;
    if (ptr == NULL) {
        c = clause_init_with_size(1);
        ptr = &c;
    }
    return ptr;
}

static bool clause_is_empty(const clause_t *c)
{
    return clause_size(c) == 0;
}

static bool clause_is_unit(const clause_t *c)
{
    return clause_size(c) == 1;
}

static const literal_t *clause_unit(const clause_t *c)
{
    assert(clause_is_unit(c));
    return clause_get(c, 0);
}

static const clause_t *clause_resolve(const clause_t *c, const litset_t *u)
{
    clause_t *d = eslmalloc(sizeof(clause_t));
    *d = clause_lazy_copy(c);
    int *indices = eslmalloc(clause_size(c) * sizeof(int));
    int n_indices = 0;
    for (int i = 0; i < litset_size(u); ++i) {
        const literal_t l = literal_flip(litset_get(u, i));
        const int j = clause_find(d, &l);
        if (j != -1) {
            indices[n_indices++] = j;
        }
    }
    clause_remove_all_indices(d, indices, n_indices);
    eslfree(indices);
    return d;
}

static void ground_univ(setup_t *setup, varmap_t *varmap,
        const univ_clause_t *univ_clause, const stdset_t *ns, int i)
{
    if (i < varset_size(&univ_clause->vars)) {
        const var_t var = varset_get(&univ_clause->vars, i);
        for (int j = 0; j < stdset_size(ns); ++j) {
            const stdname_t n = stdset_get(ns, j);
            varmap_add_replace(varmap, var, n);
            ground_univ(setup, varmap, univ_clause, ns, i+1);
        }
    } else {
        const clause_t *c = univ_clause->univ_clause(varmap);
        if (c != NULL) {
            setup_add(setup, c);
        }
    }
}

static void ground_box(setup_t *setup, const stdvec_t *z, const clause_t *c)
{
    clause_t *d = eslmalloc(sizeof(clause_t));
    *d = clause_init_with_size(clause_size(c));
    for (int i = 0; i < clause_size(c); ++i) {
        const literal_t *l = clause_get(c, i);
        const stdvec_t zz = stdvec_concat(z, literal_z(l));
        literal_t *ll = eslmalloc(sizeof(literal_t));
        *ll = literal_init(&zz, literal_sign(l), literal_pred(l),
                literal_args(l));
        const bool added = clause_add(d, ll);
        assert(added);
    }
    const bool added = setup_add(setup, d);
    if (!added) {
        for (int i = 0; i < clause_size(d); ++i) {
            eslfree((literal_t *) clause_get(d, i));
        }
        eslfree(d);
    }
}

stdset_t bat_hplus(
        const box_univ_clauses_t *box_cs,
        const univ_clauses_t *cs,
        const stdset_t *query_names,
        int n_query_vars)
{
    stdset_t names = stdset_copy(query_names);
    int max_vars = n_query_vars;
    for (int i = 0; i < box_univ_clauses_size(box_cs); ++i) {
        const box_univ_clause_t *c = box_univ_clauses_get(box_cs, i);
        stdset_add_all(&names, &c->c.names);
        max_vars = MAX(max_vars, varset_size(&c->c.vars));
    }
    for (int i = 0; i < univ_clauses_size(cs); ++i) {
        const univ_clause_t *c = univ_clauses_get(cs, i);
        stdset_add_all(&names, &c->names);
        max_vars = MAX(max_vars, varset_size(&c->vars));
    }
    stdname_t max_name = 0;
    for (int i = 0; i < stdset_size(&names); ++i) {
        max_name = MAX(max_name, stdset_get(&names, i));
    }
    assert(max_name < STDNAME_MAX);
    for (int i = 0; i < max_vars; ++i) {
        assert(max_name + i + 1 < STDNAME_MAX);
        stdset_add(&names, max_name + i + 1);
    }
    return names;
}

setup_t setup_ground_clauses(
        const box_univ_clauses_t *dynamic_bat,
        const univ_clauses_t *static_bat,
        const stdset_t *hplus,
        const stdvecset_t *query_zs)
{
    setup_t setup = setup_init();
    // first ground the static universally quantified clauses
    for (int i = 0; i < univ_clauses_size(static_bat); ++i) {
        const univ_clause_t *c = univ_clauses_get(static_bat, i);
        varmap_t varmap = varmap_init_with_size(varset_size(&c->vars));
        ground_univ(&setup, &varmap, c, hplus, 0);
    }
    setup_t box_clauses = setup_init();
    // first ground the universal quantifiers
    // this is a bit complicated because we need to compute all possible
    // assignment of variables (elements from univ_clauses[i].vars) and
    // standard names (elements of hplus) for all i
    for (int i = 0; i < box_univ_clauses_size(dynamic_bat); ++i) {
        const box_univ_clause_t *c = box_univ_clauses_get(dynamic_bat, i);
        varmap_t varmap = varmap_init_with_size(varset_size(&c->c.vars));
        ground_univ(&box_clauses, &varmap, &c->c, hplus, 0);
    }
    // ground each box by substituting it with the prefixes of all action
    // sequences
    for (int i = 0; i < setup_size(&box_clauses); ++i) {
        const clause_t *c = setup_get(&box_clauses, i);
        for (int j = 0; j < stdvecset_size(query_zs); ++j) {
            const stdvec_t *z = stdvecset_get(query_zs, j);
            for (int k = 0; k <= stdvec_size(z); ++k) {
                const stdvec_t z_prefix = stdvec_lazy_copy_range(z, 0, k);
                ground_box(&setup, &z_prefix, c);
            }
        }
    }
    return setup;
}

static void clause_pel(const clause_t *c, pelset_t *pel)
{
    for (int i = 0; i < clause_size(c); ++i) {
        const literal_t *l = clause_get(c, i);
        if (!literal_sign(l)) {
            literal_t *ll = eslmalloc(sizeof(literal_t));
            *ll = literal_flip(l);
            const bool added = pelset_add(pel, ll);
            if (!added) {
                eslfree(ll);
            }
        } else {
            pelset_add(pel, l);
        }
    }
}

pelset_t setup_pel(const setup_t *setup)
{
    pelset_t pel = pelset_init();
    for (int i = 0; i < setup_size(setup); ++i) {
        clause_pel(setup_get(setup, i), &pel);
    }
    return pel;
}

static void add_unit_clauses_from_setup(litset_t *ls, const setup_t *setup)
{
    for (int i = 0; i < setup_size(setup); ++i) {
        const clause_t *c = setup_get(setup, i);
        if (clause_is_empty(c)) {
            continue;
        } else if (clause_is_unit(c)) {
            litset_add(ls, clause_unit(c));
        } else {
            // Clauses are sets, which are again stored as vectors, and
            // vector_cmp()'s first ordering criterion is the length.
            // Thus, a unit clause is shorter than all non-empty and non-unit
            // clauses. Therefore we only need to iterate over the setup until
            // we find the first non-empty and non-unit clause and can then
            // stop.
            break;
        }
    }
}

setup_t setup_propagate_units(const setup_t *setup, const litset_t *split)
{
    litset_t units = litset_lazy_copy(split);
    add_unit_clauses_from_setup(&units, setup);
    setup_t s = setup_lazy_copy(setup);
    // XXX I think we don't have to add the split literals to the setup.
    // Otherwise we have to add all elements from split to &s.
    bool new_units;
    do {
        new_units = false;
        clause_t const **new_cs = eslmalloc(setup_size(&s) * sizeof(clause_t *));
        int *old_cs = eslmalloc(setup_size(&s) * sizeof(int));
        int n = 0;
        for (int i = 0; i < setup_size(&s); ++i) {
            const clause_t *c = setup_get(&s, i);
            const clause_t *d = clause_resolve(c, &units);
            if (!clause_eq(c, d)) {
                new_cs[n] = d;
                old_cs[n] = i;
                ++n;
                if (clause_is_unit(d)) {
                    new_units |= litset_add(&units, clause_unit(d));
                }
            }
        }
        setup_remove_all_indices(&s, old_cs, n);
        for (int i = 0; i < n; ++i) {
            const clause_t *d = new_cs[i];
            const bool added = setup_add(&s, d);
            if (!added) {
                eslfree((clause_t *) d);
            }
        }
        eslfree(old_cs);
        eslfree(new_cs);
    } while (new_units);
    return s;
}

static bool setup_subsumes(const setup_t *setup, const litset_t *split,
        const clause_t *c)
{
    const setup_t s = setup_propagate_units(setup, split);
    if (setup_size(&s) > 0 && clause_size(setup_get(&s, 0)) == 0) {
        return true;
    }
    for (int i = 0; i < setup_size(&s); ++i) {
        if (clause_contains_all(c, setup_get(&s, i))) {
            return true;
        }
    }
    return false;
}

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
            int n = query_n_vars(psi);
            eslfree(psi);
            return n;
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
            eslfree(phi1);
            eslfree(phi2);
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
        query_t *xi = eslmalloc(sizeof(query_t));
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

static query_t *query_ennf_h(query_t *phi, const stdset_t *hplus,
        bool flip, const stdvec_t *z)
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
            query_t *psi1 = query_ennf_h(phi->u.or.phi1, hplus, flip, z);
            query_t *psi2 = query_ennf_h(phi->u.or.phi2, hplus, flip, z);
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
            query_t *psi1 = query_ennf_h(phi->u.and.phi1, hplus, flip, z);
            query_t *psi2 = query_ennf_h(phi->u.and.phi2, hplus, flip, z);
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
            query_t *psi = query_ennf_h(phi->u.neg.phi, hplus, !flip, z);
            eslfree(phi);
            return psi;
        }
        case EX: {
            const bool is_existential = !flip;
            query_t *psi = query_ground_quantifier(is_existential,
                    phi->u.ex.phi, hplus, 0);
            eslfree(phi);
            return query_ennf_h(psi, hplus, flip, z);
        }
        case ACT: {
            const stdvec_t zz = stdvec_copy_append(z, phi->u.act.n);
            query_t *psi = phi->u.act.phi;
            eslfree(phi);
            return query_ennf_h(psi, hplus, flip, &zz);
        }
    }
    assert(false);
    return NULL;
}

static query_t *query_ennf(query_t *phi, const stdset_t *hplus)
{
    const stdvec_t z = stdvec_init_with_size(0);
    return query_ennf_h(phi, hplus, false, &z);
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
            stdvecset_cleanup(&l);
            stdvecset_cleanup(&r);
            return s;
        }
        case AND: {
            stdvecset_t l = query_ennf_zs(phi->u.and.phi1);
            stdvecset_t r = query_ennf_zs(phi->u.and.phi2);
            const stdvecset_t s = stdvecset_union(&l, &r);
            stdvecset_cleanup(&l);
            stdvecset_cleanup(&r);
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
            eslfree(phi);
            return NULL;
        case NEQ:
            *truth_value = phi->u.neq.n1 != phi->u.neq.n2;
            eslfree(phi);
            return NULL;
        case LIT:
            return phi;
        case OR:
            phi->u.or.phi1 = query_simplify(phi->u.or.phi1, truth_value);
            if (phi->u.or.phi1 == NULL && truth_value) {
                query_free(phi);
                return NULL;
            }
            phi->u.or.phi2 = query_simplify(phi->u.or.phi2, truth_value);
            if (phi->u.or.phi2 == NULL && truth_value) {
                query_free(phi);
                return NULL;
            }
            if (phi->u.or.phi1 == NULL) {
                query_t *psi = phi->u.or.phi2;
                eslfree(phi);
                return psi;
            } else if (phi->u.or.phi2 == NULL) {
                query_t *psi = phi->u.or.phi1;
                eslfree(phi);
                return psi;
            } else {
                return phi;
            }
        case AND:
            phi->u.and.phi1 = query_simplify(phi->u.and.phi1, truth_value);
            if (phi->u.and.phi1 == NULL && !truth_value) {
                query_free(phi);
                return NULL;
            }
            phi->u.and.phi2 = query_simplify(phi->u.and.phi2, truth_value);
            if (phi->u.and.phi2 == NULL && !truth_value) {
                query_free(phi);
                return NULL;
            }
            if (phi->u.and.phi1 == NULL) {
                query_t *psi = phi->u.and.phi2;
                eslfree(phi);
                return psi;
            } else if (phi->u.and.phi2 == NULL) {
                query_t *psi = phi->u.and.phi1;
                eslfree(phi);
                return psi;
            } else {
                return phi;
            }
        case NEG:
            phi->u.neg.phi = query_simplify(phi->u.neg.phi, truth_value);
            if (phi->u.neg.phi == NULL) {
                eslfree(phi);
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
            literal_t *l = eslmalloc(sizeof(literal_t));
            *l = phi->u.lit;
            clause_t *c = eslmalloc(sizeof(clause_t));
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
                    clause_t *c = eslmalloc(sizeof(clause_t));
                    *c = clause_union(c1, c2);
                    const bool added = cnf_add(&cnf, c);
                    if (!added) {
                        eslfree(c);
                    }
                }
            }
            for (int i = 0; i < cnf_size(&cnf1); ++i) {
                eslfree(cnf_get_unsafe(&cnf1, i));
            }
            for (int i = 0; i < cnf_size(&cnf2); ++i) {
                eslfree(cnf_get_unsafe(&cnf2, i));
            }
            cnf_cleanup(&cnf1);
            cnf_cleanup(&cnf2);
            return cnf;
        }
        case AND: {
            cnf_t cnf1 = query_cnf(phi->u.or.phi1);
            cnf_t cnf2 = query_cnf(phi->u.or.phi2);
            cnf_t cnf = cnf_union(&cnf1, &cnf2);
            cnf_cleanup(&cnf1);
            cnf_cleanup(&cnf2);
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

static stdvecset_t clause_action_sequences(const clause_t *c)
{
    stdvecset_t zs = stdvecset_init();
    for (int i = 0; i < clause_size(c); ++i) {
        const stdvec_t *z = literal_z(clause_get(c, i));
        for (int j = 0; j < stdvec_size(z); ++j) {
            stdvec_t *z_prefix = eslmalloc(sizeof(stdvec_t));
            *z_prefix = stdvec_lazy_copy_range(z, 0, j);
            const bool added = stdvecset_add(&zs, z_prefix);
            if (!added) {
                eslfree(z_prefix);
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
        const pelset_t *pel, const clause_t *c, int k)
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
            bool r = query_test_split(setup, split, pel, c, k-1);
            litset_remove(split, &l1);
            if (!r) {
                continue;
            }
            litset_add(split, &l2);
            r &= query_test_split(setup, split, pel, c, k-1);
            litset_remove(split, &l2);
            if (r) {
                return true;
            }
        }
        if (!tried) {
            stdvecset_t zs = clause_action_sequences(c);
            return query_test_sense(setup, split, pel, c, &zs);
        }
        return false;
    } else {
        stdvecset_t zs = clause_action_sequences(c);
        return query_test_sense(setup, split, pel, c, &zs);
    }
}

static bool query_test_clause(const setup_t *setup, const pelset_t *pel,
        const clause_t *c, int k)
{
    litset_t split = litset_init_with_size(k);
    if (setup_subsumes(setup, &split, c)) {
        return true;
    }
    const bool r = query_test_split(setup, &split, pel, c, k);
    litset_cleanup(&split);
    return r;
}

bool query_test(
        const box_univ_clauses_t *dynamic_bat,
        const univ_clauses_t *static_bat,
        const litset_t *sensing_results,
        query_t *phi,
        int k)
{
    const stdset_t hplus = ({
        const stdset_t ns = query_names(phi);
        const int n_vars = query_n_vars(phi);
        stdset_t hplus = bat_hplus(dynamic_bat, static_bat, &ns, n_vars);
        stdset_add_all(&hplus, &ns);
        hplus;
    });
    bool truth_value;
    phi = query_ennf(phi, &hplus);
    phi = query_simplify(phi, &truth_value);
    if (phi == NULL) {
        return truth_value;
    }
    const stdvecset_t zs = query_ennf_zs(phi);
    const setup_t setup = ({
        setup_t s = setup_ground_clauses(dynamic_bat, static_bat, &hplus, &zs);
        for (int i = 0; i < litset_size(sensing_results); ++i) {
            const literal_t *l = litset_get(sensing_results, i);
            clause_t *c = eslmalloc(sizeof(clause_t));
            *c = clause_singleton(l);
            setup_add(&s, c);
        }
        litset_t split = litset_init_with_size(0);
        s = setup_propagate_units(&s, &split);
        s;
    });
    pelset_t bat_pel = setup_pel(&setup);
    cnf_t cnf = query_cnf(phi);
    query_free(phi);
    truth_value = true;
    for (int i = 0; i < cnf_size(&cnf) && truth_value; ++i) {
        const clause_t *c = cnf_get(&cnf, i);
        pelset_t pel = pelset_lazy_copy(&bat_pel);
        clause_pel(c, &pel);
        truth_value = query_test_clause(&setup, &pel, c, k);
        pelset_cleanup(&pel);
    }
    cnf_cleanup(&cnf);
    pelset_cleanup(&bat_pel);
    return truth_value;
}

void query_free(query_t *phi)
{
    switch (phi->type) {
        case EQ:
        case NEQ:
        case LIT:
            eslfree(phi);
            break;
        case OR:
            query_free(phi->u.or.phi1);
            query_free(phi->u.or.phi2);
            eslfree(phi);
            break;
        case AND:
            query_free(phi->u.and.phi1);
            query_free(phi->u.and.phi2);
            eslfree(phi);
            break;
        case NEG:
            query_free(phi->u.neg.phi);
            eslfree(phi);
            break;
        case EX:
            eslfree(phi);
            break;
        case ACT:
            query_free(phi->u.act.phi);
            eslfree(phi);
            break;
    }
}

