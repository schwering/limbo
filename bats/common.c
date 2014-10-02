// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * schwering@kbsg.rwth-aachen.de
 */
#include "common.h"

void print_stdname(stdname_t name)
{
    printf("%s", stdname_to_string(name));
}

void print_term(term_t term)
{
    if (IS_VARIABLE(term)) {
        printf("x%ld", -1 * term);
    } else if (IS_STDNAME(term)) {
        print_stdname(term);
    } else {
        printf("%ld", term);
    }
}

void print_pred(pred_t name)
{
    printf("%s", pred_to_string(name));
}

void print_z(const stdvec_t *z)
{
    printf("[");
    for (int i = 0; i < stdvec_size(z); ++i) {
        if (i > 0) printf(",");
        print_term(stdvec_get(z, i));
    }
    printf("]");
}

void print_literal(const literal_t *l)
{
    if (stdvec_size(literal_z(l)) > 0) {
        print_z(literal_z(l));
    }
    if (!literal_sign(l)) {
        printf("~");
    }
    print_pred(literal_pred(l));
    if (stdvec_size(literal_args(l)) > 0) {
        printf("(");
        for (int i = 0; i < stdvec_size(literal_args(l)); ++i) {
            if (i > 0) printf(",");
            print_term(stdvec_get(literal_args(l), i));
        }
        printf(")");
    }
}

void print_clause(const clause_t *c)
{
    printf("[ ");
    for (int i = 0; i < clause_size(c); ++i) {
        //if (i > 0) printf(" v ");
        if (i > 0) printf(", ");
        print_literal(clause_get(c, i));
    }
    //printf("\n");
    printf(" ]\n");
}

void print_setup(const setup_t *setup)
{
    printf("Setup:\n");
    printf("---------------\n");
    for (int i = 0; i < setup_size(setup); ++i) {
        print_clause(setup_get(setup, i));
    }
    printf("---------------\n");
}

void print_pel(const pelset_t *pel)
{
    printf("PEL:\n");
    printf("---------------\n");
    for (int i = 0; i < pelset_size(pel); ++i) {
        print_literal(pelset_get(pel, i));
        printf("\n");
    }
    printf("---------------\n");
}

// Dirty hack: copy of the types from query.h for print_query().
// Alternatively, we could copy this stuff to from query.c to query.h.
// Alternatively, we could move print_query() to query.[hc].
// But I think we can delete print_query() at some time anyway.

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

void print_query(const query_t *phi)
{
    switch (phi->type) {
        case EQ: {
            printf("(");
            print_term(phi->u.eq.t1);
            printf(" == ");
            print_term(phi->u.eq.t2);
            printf(")");
            break;
        }
        case NEQ: {
            printf("(");
            print_term(phi->u.eq.t1);
            printf(" /= ");
            print_term(phi->u.eq.t2);
            printf(")");
            break;
        }
        case LIT: {
            print_literal(&phi->u.lit);
            break;
        }
        case OR: {
            printf("(");
            print_query(phi->u.bin.phi1);
            printf(" v ");
            print_query(phi->u.bin.phi2);
            printf(")");
            break;
        }
        case AND: {
            printf("(");
            print_query(phi->u.bin.phi1);
            printf(" ^ ");
            print_query(phi->u.bin.phi2);
            printf(")");
            break;
        }
        case NEG: {
            printf("~");
            print_query(phi->u.un.phi);
            break;
        }
        case EXISTS: {
            printf("(E ");
            print_term(phi->u.qtf.var);
            printf(") ");
            print_query(phi->u.qtf.phi);
            break;
        }
        case FORALL: {
            printf("(");
            print_term(phi->u.qtf.var);
            printf(") ");
            print_query(phi->u.qtf.phi);
            break;
        }
        case ACT: {
            printf("[");
            print_term(phi->u.act.n);
            printf("] ");
            print_query(phi->u.act.phi);
            break;
        }
        case EVAL: {
            printf("<eval>");
            break;
        }
    }
}

