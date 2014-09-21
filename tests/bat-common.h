// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * The example BAT from Morri & Co's AIJ paper on belief revision.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#include <assert.h>
#include <stdio.h>
#include "../src/belief.h"
#include "../src/memory.h"
#include "../src/query.h"
#include "../src/util.h"

static void print_stdname(stdname_t name);
static void print_pred(pred_t name);

static void print_z(const stdvec_t *z)
{
    printf("[");
    for (int i = 0; i < stdvec_size(z); ++i) {
        if (i > 0) printf(",");
        print_stdname(stdvec_get(z, i));
    }
    printf("]");
}

static void print_literal(const literal_t *l)
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
            print_stdname(stdvec_get(literal_args(l), i));
        }
        printf(")");
    }
}

static void print_clause(const clause_t *c)
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

static void print_setup(const setup_t *setup)
{
    printf("Setup:\n");
    printf("---------------\n");
    for (int i = 0; i < setup_size(setup); ++i) {
        print_clause(setup_get(setup, i));
    }
    printf("---------------\n");
}

static void print_pel(const pelset_t *pel)
{
    printf("PEL:\n");
    printf("---------------\n");
    for (int i = 0; i < pelset_size(pel); ++i) {
        print_literal(pelset_get(pel, i));
        printf("\n");
    }
    printf("---------------\n");
}


