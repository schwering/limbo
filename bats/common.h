// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * Common utilities for BATs.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <belief.h>
#include <memory.h>
#include <query.h>
#include <util.h>

// The follow declarations are defined in the generated *.c file.
extern const stdname_t MAX_STD_NAME;
extern const char *stdname_to_string(stdname_t val);
extern const char *pred_to_string(pred_t val);
extern stdname_t string_to_stdname(const char *str);
extern pred_t string_to_pred(const char *str);
extern void init_bat(box_univ_clauses_t *dynamic_bat, univ_clauses_t *static_bat, belief_conds_t *belief_conds);

void print_stdname(stdname_t name);
void print_term(term_t term);
void print_pred(pred_t name);
void print_z(const stdvec_t *z);
void print_literal(const literal_t *l);
void print_clause(const clause_t *c);
void print_setup(const setup_t *setup);
void print_pel(const pelset_t *pel);
void print_query(const query_t *phi);

