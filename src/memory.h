// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * Wrappers for memory allocation.
 *
 * Additionally there's a macro NEW for allocation plus initialization.
 */
#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <gc.h>

#define MALLOC(size)         GC_MALLOC(size)
#define REALLOC(mem, size)   GC_REALLOC(mem, size)
#define FREE(mem)            GC_FREE(mem)

#define NEW(expr) \
    ({\
        typeof(expr) *_NEW_ptr = MALLOC(sizeof(expr));\
        *_NEW_ptr = expr;\
        _NEW_ptr;\
    })

#endif

