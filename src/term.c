// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "term.h"
#include <stdlib.h>

static int stdname_compar(stdname_t l, stdname_t r)
{
    return l - r;
}


MAP_IMPL(varmap, var_t, stdname_t, stdname_compar);

int var_compar(var_t l, var_t r)
{
    return l - r;
}

SET_IMPL(varset, var_t, var_compar);

VECTOR_IMPL(stdvec, stdname_t, NULL);

SET_IMPL(stdvecset, stdvec_t *, stdvec_cmp);

