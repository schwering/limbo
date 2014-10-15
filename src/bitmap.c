// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "bitmap.h"
#include <stdlib.h>

#define MIN(x,y)    ((x) < (y) ? (x) : (y))
#define MAX(x,y)    ((x) > (y) ? (x) : (y))

VECTOR_IMPL(bitmap, bool, NULL);

bitmap_t bitmap_and(const bitmap_t *l, const bitmap_t *r)
{
    int common = MIN(bitmap_size(l), bitmap_size(r));
    bitmap_t bm = bitmap_init_with_size(common);
    for (int i = 0; i < common; ++i) {
        bitmap_insert(&bm, i, bitmap_get(l, i) & bitmap_get(r, i));
    }
    return bm;
}

bitmap_t bitmap_or(const bitmap_t *l, const bitmap_t *r)
{
    int common = MIN(bitmap_size(l), bitmap_size(r));
    bitmap_t bm = bitmap_init_with_size(common);
    for (int i = 0; i < common; ++i) {
        bitmap_insert(&bm, i, bitmap_get(l, i) | bitmap_get(r, i));
    }
    for (int i = common; i < bitmap_size(l); ++i) {
        bitmap_insert(&bm, i, bitmap_get(l, i));
    }
    for (int i = common; i < bitmap_size(r); ++i) {
        bitmap_insert(&bm, i, bitmap_get(r, i));
    }
    return bm;
}

