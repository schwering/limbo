// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include <stdio.h>

#define ck_assert(expr) 	if (!(expr)) { printf("%s:%d failed %s\n", __FILE__, __LINE__, #expr); }

#include "ex_bat.h"

stdvec_t *empty_vec = NULL;
stdvec_t *f_vec = NULL;
stdvec_t *s_vec = NULL;
literal_t *sensing_forward = NULL;
literal_t *sensing_sonar = NULL;
stdvec_t *context_z_1 = NULL;
splitset_t *context_sf_1 = NULL;
stdvec_t *context_z_2 = NULL;
splitset_t *context_sf_2 = NULL;

context_t make_context(void)
{
    if (empty_vec == NULL) {
        empty_vec = NEW(stdvec_init());
        f_vec = NEW(stdvec_singleton(FORWARD));
        s_vec = NEW(stdvec_singleton(SONAR));
        sensing_forward = NEW((literal_init(empty_vec, true, SF, f_vec)));
        sensing_sonar = NEW((literal_init(f_vec, true, SF, s_vec)));
        context_z_1 = NEW(stdvec_init_with_size(0));
        context_z_2 = NEW(stdvec_init_with_size(2));
        context_sf_1 = NEW(splitset_init_with_size(0));
        context_sf_2 = NEW(splitset_init_with_size(2));
        stdvec_append(context_z_2, FORWARD);
        stdvec_append(context_z_2, SONAR);
        splitset_add(context_sf_2, sensing_forward);
        splitset_add(context_sf_2, sensing_sonar);
    }

    univ_clauses_t *static_bat = MALLOC(sizeof(univ_clauses_t));
    box_univ_clauses_t *dynamic_bat = MALLOC(sizeof(box_univ_clauses_t));
    make_bat(static_bat, dynamic_bat);

    return context_init(static_bat, dynamic_bat, context_z_1, context_sf_1);
}

void run(context_t *ctx)
{
    //printf("Q0\n");
    const query_t *phi0 =
        query_and(
            query_lit(*empty_vec, false, D(0), *empty_vec),
            query_lit(*empty_vec, false, D(1), *empty_vec));
    ck_assert(query_entailed_by_setup(ctx, false, phi0, 0));

    //printf("Q1\n");
    const query_t *phi1 =
        query_neg(
            query_or(
                query_atom(D(0), *empty_vec),
                query_atom(D(1), *empty_vec)));
    ck_assert(query_entailed_by_setup(ctx, false, phi1, 0));

    //printf("Q2\n");
    const query_t *phi3 =
        query_act(FORWARD,
            query_or(
                query_atom(D(1), *empty_vec),
                query_atom(D(2), *empty_vec)));
    ck_assert(query_entailed_by_setup(ctx, false, phi3, 1));

    //printf("Q3\n");
    const query_t *phi2 =
        query_act(FORWARD,
            query_or(
                query_atom(D(1), *empty_vec),
                query_atom(D(2), *empty_vec)));
    ck_assert(!query_entailed_by_setup(ctx, false, phi2, 0));

    context_t ctx2 = context_copy_with_new_actions(ctx, context_z_2, context_sf_2);

    //printf("Q4\n");
    const query_t *phi4 =
        query_or(
            query_atom(D(0), *empty_vec),
            query_atom(D(1), *empty_vec));
    ck_assert(query_entailed_by_setup(&ctx2, false, phi4, 1));

    //printf("Q5\n");
    const query_t *phi5 = query_atom(D(0), *empty_vec);
    ck_assert(!query_entailed_by_setup(&ctx2, false, phi5, 1));

    //printf("Q6\n");
    const query_t *phi6 = query_atom(D(1), *empty_vec);
    ck_assert(query_entailed_by_setup(&ctx2, false, phi6, 1));

    //printf("Q7\n");
    const query_t *phi7 =
        query_act(SONAR,
            query_or(
                query_atom(D(0), *empty_vec),
                query_atom(D(1), *empty_vec)));
    ck_assert(query_entailed_by_setup(&ctx2, false, phi7, 1));

    //printf("Q8\n");
    const query_t *phi8 =
        query_act(SONAR,
            query_act(SONAR,
                query_or(
                    query_atom(D(0), *empty_vec),
                    query_atom(D(1), *empty_vec))));
    ck_assert(query_entailed_by_setup(&ctx2, false, phi8, 1));

    //printf("Q9\n");
    const query_t *phi9 =
        query_act(FORWARD,
            query_or(
                query_atom(D(0), *empty_vec),
                query_atom(D(1), *empty_vec)));
    ck_assert(query_entailed_by_setup(&ctx2, false, phi9, 1));

    //printf("Q10\n");
    const query_t *phi10 =
        query_act(FORWARD,
            query_act(FORWARD,
                query_atom(D(0), *empty_vec)));
    ck_assert(query_entailed_by_setup(&ctx2, false, phi10, 1));
}

int main(int argc, char *argv[])
{
    context_t ctx = make_context();
    for (int i = 0; i < 400; ++i) {
        //printf("%d:\n", i);
        run(&ctx);
    }
    return 0;
}

