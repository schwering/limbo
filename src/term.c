// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "term.h"
#include <stdlib.h>

MAP_IMPL(varmap, var_t, term_t, NULL);

term_t varmap_lookup_or_id(const varmap_t *varmap, var_t x)
{
    if (varmap_contains(varmap, x)) {
        return varmap_lookup(varmap, x);
    } else {
        return x;
    }
}

SET_IMPL(varset, var_t, NULL);

SET_IMPL(stdset, stdname_t, NULL);

VECTOR_IMPL(stdvec, stdname_t, NULL);

bool stdvec_is_ground(const stdvec_t *z)
{
    for (int i = 0; i < stdvec_size(z); ++i) {
        const term_t t = stdvec_get(z, i);
        if (IS_VARIABLE(t)) {
            return false;
        }
    }
    return true;
}

SET_IMPL(stdvecset, stdvec_t *, stdvec_cmp);

