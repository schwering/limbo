// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "term.h"

MAP_IMPL(varmap, var_t, stdname_t, stdname_compar);

int stdname_compar(stdname_t l, stdname_t r)
{
    return l - r;
}

VECTOR_IMPL(stdvec, stdname_t, NULL);

void stdvec_prepend_copy(stdvec_t *dst, stdname_t n, const stdvec_t *src)
{
    stdvec_init_with_size(dst, stdvec_size(src) + 1);
    stdvec_append(dst, n);
    stdvec_append_all(dst, src);
}

void stdvec_copy_append(stdvec_t *dst, const stdvec_t *src, stdname_t n)
{
    stdvec_init_with_size(dst, stdvec_size(src) + 1);
    stdvec_append_all(dst, src);
    stdvec_append(dst, n);
}

