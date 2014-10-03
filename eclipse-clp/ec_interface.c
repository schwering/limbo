// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * ECLiPSe-CLP interface to ESBL.
 * The following defines external predicates:
 * kcontext/1 (function p_kcontext()) interfaces kcontext_init().
 * bcontext/2 (function p_bcontext()) interfaces bcontext_init().
 * executed/3 (function p_executed()) interfaces context_add_actions().
 * holds/3 (function p_holds()) interfaces query_entailed().
 *
 * From ECLiPSe-CLP, the interface can be loaded dynamically as follows:
 *   :- load('bats/libBAT-KR2014.so'). % or some other BAT shared library
 *   :- load('eclipse-clp/libEclipseESBL.so').
 *   :- external(kcontext/1, p_kcontext).
 *   :- external(bcontext/2, p_bcontext).
 *   :- external(executed/3, p_executed).
 *   :- external(holds/3, p_holds).
 * It is not possible to handle more than BAT.
 *
 * Then, kcontext/1 or bcontext/2 can used to create a context. It's customary
 * to save it an in non-logical variable:
 *   :- kcontext(Ctx), setval(ctx, Ctx).
 *
 * We can evaluate queries against this context, 
 *   :- getval(ctx, Ctx), holds(Ctx, 1, forward : (d1 v d2)).
 *
 * Now we can feed back action execution and their sensing results to the
 * context:
 *   :- getval(ctx, Ctx), executed(Ctx, forward, true).
 *   :- getval(ctx, Ctx), executed(Ctx, sonar, true).
 *
 * Subsequent queries are evaluated in situation [forward.sonar] where both
 * actions had a positive sensing result:
 *   :- getval(ctx, Ctx), holds(Ctx, 1, d1)
 *
 * The set of queries is the least set such that
 *   P(T1,...,TK)           (predicate)
 *   ~ Alpha                (negation)
 *   (Alpha1 ^ Alpha2)      (conjunction)
 *   (Alpha1 v Alpha2)      (disjunction)
 *   (Alpha1 -> Alpha2)     (implication)
 *   (Alpha1 <-> Alpha2)    (equivalence)
 *   exists(V, Alpha)       (existential)
 *   forall(V, Alpha)       (universal)
 *   (A : Alpha)            (action)
 * where P(T1,...,Tk) is a Prolog literal and P usually exactly matches a
 * predicate from the BAT; Alpha, Alpha1, Alpha2 are queries; V are arbitrary
 * Prolog terms that represent variables; A is a ground Prolog atom that
 * represents an action and usually exactly matches a standard name from the
 * BAT.
 * When P does not match a predicate symbol from the BAT, we interpret it as a
 * new predicate symbol different from all other predicate symbols in the BAT
 * and the query.
 * When A or any ground T1,...,Tk does not match a standard name from the BAT,
 * we interpret it as a new standard name different from all standard names in
 * the BAT and the query.
 */
#include <eclipse-clp/eclipse.h>

#define NO_GC

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

MAP_DECL(epredmap, char *, pred_t);
MAP_IMPL(epredmap, char *, pred_t, strcmp);

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

static void destroy_all_preds(epredmap_t *predmap)
{
    while (epredmap_size(predmap) > 0) {
        const epredmap_kv_t *kv = epredmap_get(predmap, 0);
        const char *copy = kv->key;
        epredmap_remove(predmap, kv->key);
        free((char *) copy);
    }
    epredmap_clear(predmap);
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
        if (0 <= n && n <= MAX_STD_NAME) {
            return n;
        }
    }
    dident f;
    if (ec_get_functor(ec_term, &f) == 0 && DidArity(f) == 0) {
        const char *s = DidName(f);
        const stdname_t n = string_to_stdname(s);
        if (0 <= n && n <= MAX_STD_NAME) {
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

static pred_t build_pred(dident f, epredmap_t *predmap)
{
    // maybe we saw the predicate already
    if (epredmap_contains(predmap, DidName(f))) {
        return epredmap_lookup(predmap, DidName(f));
    }
    // maybe it's a standard name from the basic action theory
    {
        const pred_t p = string_to_pred(DidName(f));
        if (p <= MAX_PRED) {
            return p;
        }
    }
    // otherwise map the name to a new, otherwise unused pred_t
    char *name = malloc((strlen(DidName(f)) + 1) * sizeof(char));
    strcpy(name, DidName(f));
    const pred_t p = MAX_PRED + 1 + epredmap_size(predmap);
    epredmap_add(predmap, name, p);
    return p;
}

#define ARG_FORMULA(ec_alpha, beta, i) \
        pword ec_##beta;\
        if (ec_get_arg(i, ec_alpha, &ec_##beta) != 0) {\
            return NULL;\
        }\
        const query_t *beta = build_query(ec_##beta, varmap, stdmap, predmap);\
        if (beta == NULL) {\
            return NULL;\
        }

#define ARG_VAR(ec_alpha, var, i) \
        pword ec_##var;\
        if (ec_get_arg(i, ec_alpha, &ec_##var) != 0) {\
            return NULL;\
        }\
        const var_t var = create_var(ec_##var, varmap);

#define ARG_TERM(ec_alpha, term, i) \
        pword ec_##term;\
        if (ec_get_arg(i, ec_alpha, &ec_##term) != 0) {\
            return NULL;\
        }\
        const term_t term = build_term(ec_##term, varmap, stdmap);

static const query_t *build_query(pword ec_alpha, evarmap_t *varmap,
        estdmap_t *stdmap, epredmap_t *predmap)
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
        const pred_t p = build_pred(f, predmap);
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
        const pred_t p = build_pred(a, predmap);
        const bool sign = true;
        const stdvec_t args = stdvec_init_with_size(0);
        const literal_t l = literal_init(&z, sign, p, &args);
        return query_lit(&l);
    } else {
        assert(false);
        return NULL;
    }
}

static bool bat_initialized = false;
static box_univ_clauses_t dynamic_bat;
static univ_clauses_t static_bat;
static belief_conds_t belief_conds;

static bool global_context_initialized = false;
static context_t global_context;

void init_bat_once()
{
    if (!bat_initialized) {
        init_bat(&dynamic_bat, &static_bat, &belief_conds);
        bat_initialized = true;
    }
}

void free_context(t_ext_ptr data)
{
    context_t *ctx = data;
    free(ctx);
}

t_ext_ptr copy_context(t_ext_ptr old_data)
{
    const context_t *old_ctx = old_data;
    context_t new_ctx = context_copy(old_ctx);
    t_ext_ptr new_data = malloc(sizeof(new_ctx));
    memcpy(new_data, &new_ctx, sizeof(new_ctx));
    return new_data;
}

static const t_ext_type context_method_table = {
    .free = free_context,
    .copy = copy_context,
    .mark_dids = NULL,
    .string_size = NULL,
    .to_string = NULL,
    .equal = NULL,
    .remote_copy = copy_context,
    .get = NULL,
    .set = NULL
};

int p_kcontext()
{
    pword ec_var = ec_arg(1);

    init_bat_once();
    context_t ctx = kcontext_init(&static_bat, &dynamic_bat, Z(), SF());

    t_ext_ptr data = malloc(sizeof(ctx));
    memcpy(data, &ctx, sizeof(ctx));
    pword ec_ctx = ec_handle(&context_method_table, data);

    return ec_unify(ec_ctx, ec_var);
}

int p_bcontext()
{
    pword ec_k = ec_arg(1);
    pword ec_var = ec_arg(2);

    long k;
    if (ec_get_long(ec_k, &k) != 0) {
        return TYPE_ERROR;
    }

    init_bat_once();
    context_t ctx = bcontext_init(&static_bat, &belief_conds, &dynamic_bat, k, Z(), SF());

    t_ext_ptr data = malloc(sizeof(ctx));
    memcpy(data, &ctx, sizeof(ctx));
    pword ec_ctx = ec_handle(&context_method_table, data);

    return ec_unify(ec_ctx, ec_var);
}

int p_executed()
{
    pword ec_ctx = ec_arg(1);
    pword ec_action = ec_arg(2);
    pword ec_result = ec_arg(3);

    t_ext_ptr data;
    if (ec_get_handle(ec_ctx, &context_method_table, &data) != 0) {
        return TYPE_ERROR;
    }
    context_t *ctx = data;

    dident a;

    if (ec_get_atom(ec_action, &a) != 0) {
        return TYPE_ERROR;
    }
    const stdname_t action = string_to_stdname(DidName(a));
    if (action < 0) {
        return TYPE_ERROR;
    }

    if (ec_get_atom(ec_result, &a) != 0) {
        return TYPE_ERROR;
    }
    bool result;
    if (!strcmp(DidName(a), "true")) {
        result = true;
    } else if (!strcmp(DidName(a), "false")) {
        result = false;
    } else {
        return TYPE_ERROR;
    }

    CONTEXT_ADD_ACTIONS(ctx, {action, result});
    return PSUCCEED;
}

int p_holds()
{
    pword ec_ctx = ec_arg(1);
    pword ec_k = ec_arg(2);
    pword ec_alpha = ec_arg(3);

    t_ext_ptr data;
    if (ec_get_handle(ec_ctx, &context_method_table, &data) != 0) {
        return TYPE_ERROR;
    }
    context_t *ctx = data;

    long k;
    if (ec_get_long(ec_k, &k) != 0) {
        return TYPE_ERROR;
    }

    evarmap_t varmap = evarmap_init();
    estdmap_t stdmap = estdmap_init();
    epredmap_t predmap = epredmap_init();
    const query_t *alpha = build_query(ec_alpha, &varmap, &stdmap, &predmap);
    if (alpha == NULL) {
        return TYPE_ERROR;
    }

    print_query(alpha);
    printf("\n");

    bool holds = query_entailed(ctx, false, alpha, k);

    destroy_all_stdnames(&stdmap);
    destroy_all_vars(&varmap);
    destroy_all_preds(&predmap);
    return holds ? PSUCCEED : PFAIL;
}

