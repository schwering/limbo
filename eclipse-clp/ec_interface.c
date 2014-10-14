// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * ECLiPSe-CLP interface to ESBL.
 * The following defines external predicates:
 * kcontext/1 (function p_kcontext()) interfaces kcontext_init().
 * bcontext/2 (function p_bcontext()) interfaces bcontext_init().
 * context_exec/3 (function p_context_exec()) interfaces context_add_actions().
 * context_entails/3 (function p_context_entails()) interfaces query_entailed().
 *
 * From ECLiPSe-CLP, the interface can be loaded dynamically as follows:
 *   :- load('bats/libBAT-KR2014.so'). % or some other BAT shared library
 *   :- load('eclipse-clp/libEclipseESBL.so').
 *   :- external(kcontext/1, p_kcontext).
 *   :- external(bcontext/2, p_bcontext).
 *   :- external(context_exec/3, p_context_exec).
 *   :- external(context_entails/3, p_context_entails).
 * It is not possible to handle more than BAT.
 *
 * Then, kcontext/1 or bcontext/2 can used to create a context. It's customary
 * to save it non-logically:
 *   :- kcontext(Ctx), store_context(id, Ctx).
 *
 * We can evaluate queries against this context, 
 *   :- retrieve_context(id, Ctx), context_entails(Ctx, 1, forward : (d1 v d2)).
 *
 * The first argument in [store|retrieve]_context/2 is an identifier, which must
 * be a Prolog atom. Notice that [store|retrieve]_context/2 differs from
 * [set|get]val/2 in that it does not copy the context. In most scenarios,
 * [store|retrieve]_context/2 is thus what you want, as it copying over
 * [set|get]val/2 and spares you another setval/2 after changing the context.
 *
 * Now we can feed back action execution and their sensing results to the
 * context:
 *   :- retrieve_context(id, Ctx), context_exec(Ctx, forward, true).
 *   :- retrieve_context(id, Ctx), context_exec(Ctx, sonar, true).
 * If we had used [set|get]val/2 instead of [store|retrieve]_context/2, we
 * would have to memorize the new changed context with setval/2 after
 * context_exec/3.
 *
 * Subsequent queries are evaluated in situation [forward.sonar] where both
 * actions had a positive sensing result:
 *   :- retrieve_context(id, Ctx), context_entails(Ctx, 1, d1)
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
 *
 * schwering@kbsg.rwth-aachen.de
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

MAP_DECL(ec_varmap, pword *, var_t);
MAP_IMPL(ec_varmap, pword *, var_t, pword_ptr_compare);

MAP_DECL(ec_stdmap, pword *, stdname_t);
MAP_IMPL(ec_stdmap, pword *, stdname_t, pword_ptr_compare);

MAP_DECL(ec_predmap, char *, pred_t);
MAP_IMPL(ec_predmap, char *, pred_t, strcmp);

MAP_DECL(ec_ctxmap, char *, context_t *);
MAP_IMPL(ec_ctxmap, char *, context_t *, strcmp);

static bool bat_initialized = false;
static box_univ_clauses_t dynamic_bat;
static univ_clauses_t static_bat;
static belief_conds_t belief_conds;

static bool ctxmap_initialized = false;
static ec_ctxmap_t ctxmap;


static var_t create_var(pword ec_var, ec_varmap_t *varmap)
{
    if (ec_varmap_contains(varmap, &ec_var)) {
        return ec_varmap_lookup(varmap, &ec_var);
    }
    pword *copy = malloc(sizeof(pword));
    *copy = ec_var;
    const var_t var = -1 - ec_varmap_size(varmap);
    ec_varmap_add(varmap, copy, var);
    return var;
}

static void destroy_var(pword ec_var, ec_varmap_t *varmap)
{
    const int i = ec_varmap_find(varmap, &ec_var);
    assert(i >= 0);
    const ec_varmap_kv_t *kv = ec_varmap_get(varmap, i);
    assert(kv != NULL);
    const pword *copy = kv->key;
    ec_varmap_remove(varmap, kv->key);
    free((pword *) copy);
}

static void destroy_all_vars(ec_varmap_t *varmap)
{
    while (ec_varmap_size(varmap) > 0) {
        const ec_varmap_kv_t *kv = ec_varmap_get(varmap, 0);
        const pword *copy = kv->key;
        ec_varmap_remove(varmap, kv->key);
        free((pword *) copy);
    }
    ec_varmap_clear(varmap);
}

static void destroy_all_stdnames(ec_stdmap_t *stdmap)
{
    while (ec_stdmap_size(stdmap) > 0) {
        const ec_stdmap_kv_t *kv = ec_stdmap_get(stdmap, 0);
        const pword *copy = kv->key;
        ec_stdmap_remove(stdmap, kv->key);
        free((pword *) copy);
    }
    ec_stdmap_clear(stdmap);
}

static void destroy_all_preds(ec_predmap_t *predmap)
{
    while (ec_predmap_size(predmap) > 0) {
        const ec_predmap_kv_t *kv = ec_predmap_get(predmap, 0);
        const char *copy = kv->key;
        ec_predmap_remove(predmap, kv->key);
        free((char *) copy);
    }
    ec_predmap_clear(predmap);
}

static term_t build_term(pword ec_term, ec_varmap_t *varmap, ec_stdmap_t *stdmap)
{
    // maybe it's a variable
    if (ec_varmap_contains(varmap, &ec_term)) {
        return ec_varmap_lookup(varmap, &ec_term);
    }
    // maybe we saw the stdname already
    if (ec_stdmap_contains(stdmap, &ec_term)) {
        return ec_stdmap_lookup(stdmap, &ec_term);
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
    const stdname_t n = MAX_STD_NAME + 1 + ec_stdmap_size(stdmap);
    ec_stdmap_add(stdmap, copy, n);
    return n;
}

static pred_t build_pred(dident f, ec_predmap_t *predmap)
{
    // maybe we saw the predicate already
    if (ec_predmap_contains(predmap, DidName(f))) {
        return ec_predmap_lookup(predmap, DidName(f));
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
    const pred_t p = MAX_PRED + 1 + ec_predmap_size(predmap);
    ec_predmap_add(predmap, name, p);
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

static const query_t *build_query(pword ec_alpha, ec_varmap_t *varmap,
        ec_stdmap_t *stdmap, ec_predmap_t *predmap)
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
    for (int i = 0; i < ec_ctxmap_size(&ctxmap); ++i) {
        const ec_ctxmap_kv_t *kv = ec_ctxmap_get(&ctxmap, i);
        if (kv->val == ctx) {
            return;
        }
    }
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
    context_t ctx = kcontext_init(&static_bat, &dynamic_bat);

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
    context_t ctx = bcontext_init(&static_bat, &belief_conds, &dynamic_bat, k);

    t_ext_ptr data = malloc(sizeof(ctx));
    memcpy(data, &ctx, sizeof(ctx));
    pword ec_ctx = ec_handle(&context_method_table, data);

    return ec_unify(ec_ctx, ec_var);
}

int p_store_context()
{
    pword ec_id = ec_arg(1);
    pword ec_ctx = ec_arg(2);

    if (!ctxmap_initialized) {
        ctxmap_initialized = true;
        ec_ctxmap_t map = ec_ctxmap_init();
        memcpy(&ctxmap, &map, sizeof(ec_ctxmap_t));
    }

    dident a;
    if (ec_get_atom(ec_id, &a) != 0) {
        return TYPE_ERROR;
    }

    t_ext_ptr data;
    if (ec_get_handle(ec_ctx, &context_method_table, &data) != 0) {
        return TYPE_ERROR;
    }
    context_t *ctx = data;

    char *name = malloc((strlen(DidName(a)) + 1) * sizeof(char));
    strcpy(name, DidName(a));
    ec_ctxmap_add_replace(&ctxmap, name, ctx);

    return PSUCCEED;
}

int p_retrieve_context()
{
    pword ec_id = ec_arg(1);
    pword ec_var = ec_arg(2);

    if (!ctxmap_initialized) {
        return PFAIL;
    }

    dident a;
    if (ec_get_atom(ec_id, &a) != 0) {
        return TYPE_ERROR;
    }

    const context_t *ctx = ec_ctxmap_lookup(&ctxmap, DidName(a));
    if (ctx == NULL) {
        return PFAIL;
    }

    t_ext_ptr data = (context_t *) ctx;
    pword ec_ctx = ec_handle(&context_method_table, data);

    return ec_unify(ec_ctx, ec_var);
}

int p_context_exec()
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

    context_add_action(ctx, action, result);
    return PSUCCEED;
}

int p_context_entails()
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

    ec_varmap_t varmap = ec_varmap_init();
    ec_stdmap_t stdmap = ec_stdmap_init();
    ec_predmap_t predmap = ec_predmap_init();

    const query_t *alpha = build_query(ec_alpha, &varmap, &stdmap, &predmap);
    if (alpha == NULL) {
        return TYPE_ERROR;
    }

    //print_query(alpha); printf("\n");

    //printf("%d | %d\n", setup_size(&ctx->u.k.setup), pelset_size(&ctx->u.k.pel));
    bool holds = query_entailed(ctx, false, alpha, k);

    destroy_all_vars(&varmap);
    destroy_all_stdnames(&stdmap);
    destroy_all_preds(&predmap);
    return holds ? PSUCCEED : PFAIL;
}

