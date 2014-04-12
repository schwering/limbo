// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * Wrappers for memory allocation.
 *
 * We use nedmalloc because it allows us to create memory pools which can be
 * freed all at once. Since we allocate memory for setups and clauses all over
 * the handling of a query, these pools drastically simplify things as we can
 * just destroy the pool when the query is answered.
 *
 * The idea is that each query is answered in a single thread and one
 * memory pool is created per thread. Thus we can use thread-local variables
 * to access the pool and can hide everything behind a standard-looking memory
 * allocation API.
 *
 * Additionally there's a macro NEW for allocation plus initialization.
 */
#ifndef _MEMORY_H_
#define _MEMORY_H_

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

#define NEW(expr) \
    ({\
        typeof(expr) *ptr = MALLOC(sizeof(expr));\
        *ptr = expr;\
        ptr;\
    })

#endif

