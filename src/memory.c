// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "memory.h"

pthread_key_t thread_pool_key;
pthread_once_t thread_pool_once = PTHREAD_ONCE_INIT;

static void destroy_pool(void *p)
{
    neddestroypool((nedpool *) p);
}

void create_pool(void)
{
    const size_t capacity = 0;
    const int threads = 1;
    nedpool *p = nedcreatepool(capacity, threads);
    pthread_key_create(&thread_pool_key, &destroy_pool);
    pthread_setspecific(thread_pool_key, p);
}

void thread_pool_init_once(void)
{
    pthread_once(&thread_pool_once, &create_pool);
}

