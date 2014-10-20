// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * A simple bitmap implementation.
 * Could be space-optimized.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#ifndef _BITMAP_H_
#define _BITMAP_H_

#include "vector.h"
#include <stdbool.h>

VECTOR_DECL(bitmap, bool);

bitmap_t bitmap_and(const bitmap_t *l, const bitmap_t *r);
bitmap_t bitmap_or(const bitmap_t *l, const bitmap_t *r);

#endif

