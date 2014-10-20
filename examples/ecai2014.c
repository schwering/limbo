// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * This is an implementation of (most of the) queries proven in the ESB paper.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#include <stdio.h>
#include <ecai2014.h>

#define ASSERT(phi)     ((void) ((phi) || printf("Condition failed: " #phi)))

int main(int argc, char *argv[])
{
    univ_clauses_t static_bat;
    belief_conds_t belief_conds;
    box_univ_clauses_t dynamic_bat;
    init_bat(&dynamic_bat, &static_bat, &belief_conds);
    const int k = 2;
    context_t ctx1 = bcontext_init(&static_bat, &belief_conds, &dynamic_bat, k);

    ASSERT(bsetup_size(&ctx1.u.b.setups) == 3);

    // Property 1
    const query_t *phi1 = Q(N(Z(), L1, A()));
    ASSERT(query_entailed(&ctx1, false, phi1, k));

    // Property 2
    const query_t *phi2 = query_and(Q(P(Z(), L1, A())), Q(P(Z(), R1, A())));
    context_t ctx2 = context_copy(&ctx1);
    context_add_action(&ctx2, SL, true);
    ASSERT(query_entailed(&ctx2, false, phi2, k));
    ASSERT(!query_entailed(&ctx1, false, phi2, k)); // sensing really really is required

    // Property 3
    const query_t *phi3 = Q(N(Z(), R1, A()));
    context_t ctx3 = context_copy(&ctx2);
    context_add_action(&ctx3, SR1, false);
    ASSERT(query_entailed(&ctx3, false, phi3, k));
    ASSERT(!query_entailed(&ctx2, false, phi3, k)); // sensing really really is required

    // Property 5
    const query_t *phi5a = Q(P(Z(), L1, A()));
    const query_t *phi5b = Q(N(Z(), L1, A()));
    ASSERT(!query_entailed(&ctx3, false, phi5a, k));
    ASSERT(!query_entailed(&ctx3, false, phi5b, k));

    // Property 6
    const query_t *phi6 = Q(P(Z(), R1, A()));
    context_t ctx4 = context_copy(&ctx3);
    context_add_action(&ctx4, LV, true);
    ASSERT(query_entailed(&ctx4, false, phi6, k));
    ASSERT(!query_entailed(&ctx3, false, phi6, k)); // sensing really really is required

    // Property 7
    const query_t *phi7 = Q(P(Z(), L1, A()));
    context_t ctx5 = context_copy(&ctx4);
    context_add_action(&ctx5, SL, true);
    ASSERT(query_entailed(&ctx5, false, phi7, k));
    ASSERT(query_entailed(&ctx4, false, phi6, k)); // sensing really really is required

    printf("Example from the ECAI-2014 paper works\n");
    return 0;
}
