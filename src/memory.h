// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab

#include <stddef.h>

#define NEW(expr)    ({ typeof(expr) *ptr = eslmalloc(sizeof(expr)); *ptr = expr; ptr; })

void *eslmalloc(size_t size);
void *eslrealloc(void *mem, size_t size);
void eslfree(void *mem);
void eslfreethread(void);

