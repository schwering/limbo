// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "term.h"
#include <stdlib.h>

MAP_IMPL(varmap, var_t, stdname_t, NULL);

SET_IMPL(varset, var_t, NULL);

SET_IMPL(stdset, stdname_t, NULL);

VECTOR_IMPL(stdvec, stdname_t, NULL);

SET_IMPL(stdvecset, stdvec_t *, stdvec_cmp);

