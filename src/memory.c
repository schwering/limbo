// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "memory.h"
#include "ext/nedmalloc.h"
#include <pthread.h>
#include <stdio.h>

static pthread_key_t key;

static void make_key(void)
{
    pthread_key_create(&key, &neddestroypool);
    const size_t capacity = 0;
    const int threads = 1;
    nedpool *p = nedcreatepool(capacity, threads);
    pthread_setspecific(key, p);
}

static inline nedpool *thread_pool(void)
{
    static pthread_once_t key_once = PTHREAD_ONCE_INIT;
    pthread_once(&key_once, &make_key);
    return (nedpool *) pthread_getspecific(key);
}

void *eslmalloc(size_t size)
{
    return nedpmalloc2(thread_pool(), size, 0, 0);
}

void *eslrealloc(void *mem, size_t size)
{
    return nedprealloc2(thread_pool(), mem, size, 0, 0);
}

void eslfree(void *mem)
{
    nedpfree2(thread_pool(), mem, 0);
}

void eslfreethread(void)
{
    neddestroypool(thread_pool());
}

