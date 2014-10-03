// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * Wrappers for memory allocation.
 *
 * Additionally there's a macro NEW for allocation plus initialization.
 */
#ifndef _MEMORY_H_
#define _MEMORY_H_

#define NO_GC

#ifndef NO_GC

#include <gc.h>

#define MALLOC(size)         GC_MALLOC(size)
#define REALLOC(mem, size)   GC_REALLOC(mem, size)
#define FREE(mem)            GC_FREE(mem)

#else

#include <stdlib.h>

#define MALLOC(size)         malloc(size)
#define REALLOC(mem, size)   realloc(mem, size)
#define FREE(mem)            free(mem)

#endif

#define NEW(expr) \
    ({\
        typeof(expr) *_NEW_ptr = MALLOC(sizeof(expr));\
        *_NEW_ptr = expr;\
        _NEW_ptr;\
    })

#endif

