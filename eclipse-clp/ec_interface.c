// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include <eclipse-clp/eclipse.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <map.h>
#include <query.h>
#include "../bats/common.h" // one of the BAT *.so libraries must be loaded dynamically through load/1.

#define NEGATION    "~"
#define CONJUNCTION "^"
#define DISJUNCTION "v"
#define IMPLICATION "->"
#define EQUIVALENCE "<->"
#define EXISTS	    "exists"
#define FORALL	    "forall"
#define ACTION	    ":"

static int pword_ptr_compare(const pword *l, const pword *r)
{
    return ec_compare(*l, *r);
}

MAP_DECL(evarmap, pword *, var_t);
MAP_IMPL(evarmap, pword *, var_t, pword_ptr_compare);

MAP_DECL(estdmap, pword *, stdname_t);
MAP_IMPL(estdmap, pword *, stdname_t, pword_ptr_compare);

static var_t create_var(pword ec_var, evarmap_t *varmap)
{
    if (evarmap_contains(varmap, &ec_var)) {
        return evarmap_lookup(varmap, &ec_var);
    }
    pword *copy = malloc(sizeof(pword));
    *copy = ec_var;
    const var_t var = -1 - evarmap_size(varmap);
    evarmap_add(varmap, copy, var);
    return var;
}

static void destroy_var(pword ec_var, evarmap_t *varmap)
{
    const int i = evarmap_find(varmap, &ec_var);
    assert(i >= 0);
    const evarmap_kv_t *kv = evarmap_get(varmap, i);
    assert(kv != NULL);
    const pword *copy = kv->key;
    evarmap_remove(varmap, kv->key);
    free((pword *) copy);
}

static void destroy_all_vars(evarmap_t *varmap)
{
    while (evarmap_size(varmap) > 0) {
        const evarmap_kv_t *kv = evarmap_get(varmap, 0);
        const pword *copy = kv->key;
        evarmap_remove(varmap, kv->key);
        free((pword *) copy);
    }
    evarmap_clear(varmap);
}

static void destroy_all_stdnames(estdmap_t *stdmap)
{
    while (estdmap_size(stdmap) > 0) {
        const estdmap_kv_t *kv = estdmap_get(stdmap, 0);
        const pword *copy = kv->key;
        estdmap_remove(stdmap, kv->key);
        free((pword *) copy);
    }
    estdmap_clear(stdmap);
}

static term_t build_term(pword ec_term, evarmap_t *varmap, estdmap_t *stdmap)
{
    // maybe it's a variable
    if (evarmap_contains(varmap, &ec_term)) {
        return evarmap_lookup(varmap, &ec_term);
    }
    // maybe we saw the stdname already
    if (estdmap_contains(stdmap, &ec_term)) {
        return estdmap_lookup(stdmap, &ec_term);
    }
    // maybe it's a standard name from the basic action theory
    dident a;
    if (ec_get_atom(ec_term, &a) == 0) {
        const char *s = DidName(a);
        const stdname_t n = string_to_stdname(s);
        if (n >= 0) {
            return n;
        }
    }
    dident f;
    if (ec_get_functor(ec_term, &f) == 0 && DidArity(f) == 0) {
        const char *s = DidName(f);
        const stdname_t n = string_to_stdname(s);
        if (n >= 0) {
            return n;
        }
    }
    // otherwise map the name to a new, otherwise unused stdname_t
    pword *copy = malloc(sizeof(pword));
    *copy = ec_term;
    const stdname_t n = MAX_STD_NAME + 1 + estdmap_size(stdmap);
    estdmap_add(stdmap, copy, n);
    return n;
}

#define ARG_FORMULA(ec_alpha, beta, i) \
        pword ec_##beta;\
        ec_get_arg(i, ec_alpha, &ec_##beta);\
        const query_t *beta = build_query(ec_##beta, varmap, stdmap);

#define ARG_VAR(ec_alpha, var, i) \
        pword ec_##var;\
        ec_get_arg(i, ec_alpha, &ec_##var);\
        const var_t var = create_var(ec_##var, varmap);

#define ARG_TERM(ec_alpha, term, i) \
        pword ec_##term;\
        ec_get_arg(i, ec_alpha, &ec_##term);\
        const term_t term = build_term(ec_##term, varmap, stdmap);

static const query_t *build_query(pword ec_alpha, evarmap_t *varmap, estdmap_t *stdmap)
{
    dident f;
    dident a;
    const bool is_functor = ec_get_functor(ec_alpha, &f) == 0;
    const bool is_atom = ec_get_atom(ec_alpha, &a) == 0;
    if (is_functor && !strcmp(DidName(f), NEGATION) && DidArity(f) == 1) {
        ARG_FORMULA(ec_alpha, beta, 1);
        return query_neg(beta);
    } else if (is_functor && !strcmp(DidName(f), DISJUNCTION) && DidArity(f) == 2) {
        ARG_FORMULA(ec_alpha, beta1, 1);
        ARG_FORMULA(ec_alpha, beta2, 2);
        return query_or(beta1, beta2);
    } else if (is_functor && !strcmp(DidName(f), CONJUNCTION) && DidArity(f) == 2) {
        ARG_FORMULA(ec_alpha, beta1, 1);
        ARG_FORMULA(ec_alpha, beta2, 2);
        return query_and(beta1, beta2);
    } else if (is_functor && !strcmp(DidName(f), IMPLICATION) && DidArity(f) == 2) {
        ARG_FORMULA(ec_alpha, beta1, 1);
        ARG_FORMULA(ec_alpha, beta2, 2);
        return query_impl(beta1, beta2);
    } else if (is_functor && !strcmp(DidName(f), EQUIVALENCE) && DidArity(f) == 2) {
        ARG_FORMULA(ec_alpha, beta1, 1);
        ARG_FORMULA(ec_alpha, beta2, 2);
        return query_equiv(beta1, beta2);
    } else if (is_functor && !strcmp(DidName(f), EXISTS) && DidArity(f) == 2) {
        ARG_VAR(ec_alpha, var, 1);
        ARG_FORMULA(ec_alpha, beta, 2);
        destroy_var(ec_var, varmap);
        return query_exists(var, beta);
    } else if (is_functor && !strcmp(DidName(f), FORALL) && DidArity(f) == 2) {
        ARG_VAR(ec_alpha, var, 1);
        ARG_FORMULA(ec_alpha, beta, 2);
        destroy_var(ec_var, varmap);
        return query_forall(var, beta);
    } else if (is_functor && !strcmp(DidName(f), ACTION) && DidArity(f) == 2) {
        ARG_TERM(ec_alpha, term, 1);
        ARG_FORMULA(ec_alpha, beta, 2);
        return query_act(term, beta);
    } else if (is_functor) {
        const stdvec_t z = stdvec_init_with_size(0);
        const pred_t p = string_to_pred(DidName(f));
        const bool sign = true;
        stdvec_t args = stdvec_init_with_size(DidArity(f));
        for (int i = 1; i <= DidArity(f); ++i) {
            ARG_TERM(ec_alpha, term, i);
            stdvec_append(&args, term);
        }
        const literal_t l = literal_init(&z, sign, p, &args);
        return query_lit(&l);
    } else if (is_atom) {
        const stdvec_t z = stdvec_init_with_size(0);
        const pred_t p = string_to_pred(DidName(a));
        const bool sign = true;
        const stdvec_t args = stdvec_init_with_size(0);
        const literal_t l = literal_init(&z, sign, p, &args);
        return query_lit(&l);
    } else {
        assert(false);
        return NULL;
    }
}

int p_holds()
{
    //univ_clauses_t static_bat = univ_clauses_init();
    //box_univ_clauses_t dynamic_bat = box_univ_clauses_init();
    //DECL_ALL_CLAUSES(&dynamic_bat, &static_bat, NULL);
    pword ec_alpha = ec_arg(1);
    evarmap_t varmap = evarmap_init();
    estdmap_t stdmap = estdmap_init();
    const query_t *alpha = build_query(ec_alpha, &varmap, &stdmap);
    print_query(alpha);
    printf("\n");
    destroy_all_vars(&varmap);
    destroy_all_stdnames(&stdmap);
    return 0;
}

