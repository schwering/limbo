// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * Wrappers for memory allocation.
 * They either use Boehm GC or nedmalloc.
 *
 * nedmalloc allows us to create memory pools which can be freed all at once.
 * Since we allocate memory for setups and clauses all over the handling of a
 * query, these pools drastically simplify things as we can just destroy the
 * pool when the query is answered.
 * The idea is that each query is answered in a single thread and one
 * memory pool is created per thread. Thus we can use thread-local variables
 * to access the pool and can hide everything behind a standard-looking memory
 * allocation API.
 *
 * Additionally there's a macro NEW for allocation plus initialization.
 */
#ifndef _MEMORY_H_
#define _MEMORY_H_

#if defined(USE_BOEHM_GC)

    #include <gc.h>

    #define MALLOC(size)         GC_MALLOC(size)
    #define REALLOC(mem, size)   GC_REALLOC(mem, size)
    #define FREE(mem)            GC_FREE(mem)

#elif defined(USE_NEDMALLOC)

    #include <pthread.h>
    #include <stddef.h>
    #include "ext/nedmalloc.h"

    extern pthread_key_t thread_pool_key;
    void thread_pool_init_once(void);

    #define THREAD_POOL   (thread_pool_init_once(),\
                           (nedpool *) pthread_getspecific(thread_pool_key))

    #define MALLOC(size)         nedpmalloc2(THREAD_POOL, size, 0, 0)
    #define REALLOC(mem, size)   nedprealloc2(THREAD_POOL, mem, size, 0, 0)
    #define FREE(mem)            nedpfree2(THREAD_POOL, mem, 0)

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

