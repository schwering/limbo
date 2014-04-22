// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "belief.h"
#include "memory.h"
#include <assert.h>

struct belief_cond {
    const clause_t *neg_phi;
    const clause_t *psi;
};

static int belief_cond_cmp(const belief_cond_t *l, const belief_cond_t *r)
{
    const int cmp = clause_cmp(l->neg_phi, r->neg_phi);
    if (cmp != 0) {
        return cmp;
    } else {
        return clause_cmp(l->psi, r->psi);
    }
}

SET_IMPL(belief_conds, belief_cond_t *, belief_cond_cmp);

