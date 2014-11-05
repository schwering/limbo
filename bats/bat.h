// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef BATS_BAT_H_
#define BATS_BAT_H_

#include <cassert>
#include <map>
#include <string>
#include "atom.h"
#include "setup.h"
#include "term.h"

namespace bats {

using namespace esbl;

class BAT {
  void Init() {
    InitNameToStringMap();
    InitPredToStringMap();
    InitStringToNameMap();
    InitStringToPredMap();
    InitSetup();
  }

  virtual Term::Id max_std_name() const = 0;
  virtual Atom::PredId max_pred() const = 0;

  virtual void InitNameToStringMap() = 0;
  virtual void InitPredToStringMap() = 0;
  virtual void InitStringToNameMap() = 0;
  virtual void InitStringToPredMap() = 0;

  virtual void InitSetup() = 0;

 protected:
  Term::Factory tf_;
  std::map<StdName, const char*> name_to_string_;
  std::map<Atom::PredId, const char*> pred_to_string_;
  std::map<std::string, StdName> string_to_name_;
  std::map<std::string, Atom::PredId> string_to_pred_;
  Setup setup_;
};

#if 0
// The follow declarations are defined in the generated *.c file.
extern const stdname_t MAX_STD_NAME;
extern const pred_t MAX_PRED;
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
void print_query(const query_t *phi);
#endif

}

#endif  // BATS_BAT_H_

