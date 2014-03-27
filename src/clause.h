// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * schwering@kbsg.rwth-aachen.de
 */
#ifndef _CLAUSE_H_
#define _CLAUSE_H_

#include "literal.h"
#include "set.h"
#include "term.h"

SET_DECL(clause, literal_t *);

SET_DECL(box_clause, literal_t *);

typedef bool (*guard_t)(const varmap_t *map);
typedef struct {
    guard_t guard;
    box_clause_t clause;
} guarded_clause_t;

#endif

