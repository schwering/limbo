// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "set.h"
#include <stdlib.h>
#include <string.h>

set_t *set_new(compar_t compar)
{
    set_t *set = malloc(sizeof(set_t));
    set_init(set, compar);
    return set;
}

void set_init(set_t *set, compar_t compar)
{
    vector_init(&set->vec);
    set->compar = compar;
}

static int search(const set_t *set, const void *obj)
{
    int lo = 0;
    int hi = vector_size(&set->vec);
    while (lo <= hi) {
        const int i = (lo + hi) / 2;
        const int cmp = set->compar(obj, vector_get(&set->vec, i));
        if (cmp == 0) { // found
            return i;
        } else if (cmp < 0) { // left half
            hi = i - 1;
        } else { // right half
            lo = i + 1;
        }
    }
    return -1;
}

static int insert_pos(const set_t *set, const void *obj)
{
    int lo = 0;
    int hi = vector_size(&set->vec);
    while (lo <= hi) {
        const int i = (lo + hi) / 2;
        if (set->compar(obj, vector_get(&set->vec, i)) <= 0) { // left half or found
            if (i == 0 || !(set->compar(obj, vector_get(&set->vec, i-1)) <= 0)) { // found
                return i;
            } else { // left half
                hi = i - 1;
            }
        } else { // right half
            lo = i + 1;
        }
    }
    return vector_size(&set->vec);
}

const void *set_get(const set_t *set, int index)
{
    return vector_get(&set->vec, index);
}

int set_size(const set_t *set)
{
    return vector_size(&set->vec);
}

bool set_eq(const set_t *set1, const set_t *set2)
{
    return set1->compar == set2->compar && vector_eq(&set1->vec, &set2->vec, set1->compar);
}

void set_add(set_t *set, const void *elem)
{

}

