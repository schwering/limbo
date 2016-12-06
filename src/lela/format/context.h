// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering
//
// Context objects store and create symbols, terms, and allow for textual
// representation, and encapsulate a Solver object.

#ifndef LELA_FORMAT_CONTEXT_H_
#define LELA_FORMAT_CONTEXT_H_

#include <map>
#include <iostream>
#include <string>
#include <stdexcept>
#include <utility>

#include <lela/term.h>
#include <lela/formula.h>
#include <lela/solver.h>
#include <lela/format/output.h>

namespace lela {
namespace format {
namespace pdl {

class Context {
 public:
  Symbol::Sort CreateSort() {
    return solver_.sf()->CreateSort();
  }

  Term CreateVariable(Symbol::Sort sort) {
    return solver_.tf()->CreateTerm(solver_.sf()->CreateVariable(sort));
  }

  Term CreateName(Symbol::Sort sort) {
    return solver_.tf()->CreateTerm(solver_.sf()->CreateName(sort));
  }

  Symbol CreateFunction(Symbol::Sort sort, Symbol::Arity arity) {
    return solver_.sf()->CreateFunction(sort, arity);
  }

  bool IsRegisteredSort(const std::string& id) const {
    return sorts_.find(id) != sorts_.end();
  }

  bool IsRegisteredVariable(const std::string& id) const {
    return vars_.find(id) != vars_.end();
  }

  bool IsRegisteredName(const std::string& id) const {
    return names_.find(id) != names_.end();
  }

  bool IsRegisteredFunction(const std::string& id) const {
    return funs_.find(id) != funs_.end();
  }

  bool IsRegisteredFormula(const std::string& id) const {
    return formulas_.find(id) != formulas_.end();
  }

  bool IsRegisteredTerm(const std::string& id) const {
    return IsRegisteredVariable(id) || IsRegisteredName(id) || IsRegisteredFunction(id);
  }

  Symbol::Sort LookupSort(const std::string& id) const {
    const auto it = sorts_.find(id);
    if (it == sorts_.end())
      throw std::domain_error(id);
    return it->second;
  }

  Term LookupVariable(const std::string& id) const {
    const auto it = vars_.find(id);
    if (it == vars_.end())
      throw std::domain_error(id);
    return it->second;
  }

  Term LookupName(const std::string& id) const {
    const auto it = names_.find(id);
    if (it == names_.end())
      throw std::domain_error(id);
    return it->second;
  }

  const Symbol& LookupFunction(const std::string& id) const {
    const auto it = funs_.find(id);
    if (it == funs_.end())
      throw std::domain_error(id);
    return it->second;
  }

  const Formula& LookupFormula(const std::string& id) const {
    const auto it = formulas_.find(id);
    if (it == formulas_.end())
      throw std::domain_error(id);
    return it->second;
  }

  void RegisterSort(const std::string& id) {
    using lela::format::output::operator<<;
    const Symbol::Sort sort = CreateSort();
    lela::format::output::RegisterSort(sort, "");
    sorts_[id] = sort;
    std::cout << "RegisterSort " << id << std::endl;
  }

  void RegisterVariable(const std::string& id, const std::string& sort_id) {
    using lela::format::output::operator<<;
    if (IsRegisteredVariable(id))
      throw std::domain_error(id);
    const Symbol::Sort sort = LookupSort(sort_id);
    const Term var = CreateVariable(sort);
    vars_[id] = var;
    lela::format::output::RegisterSymbol(var.symbol(), id);
    std::cout << "RegisterVar " << id << " -> " << sort_id << std::endl;
  }

  void RegisterName(const std::string& id, const std::string& sort_id) {
    using lela::format::output::operator<<;
    if (IsRegisteredName(id))
      throw std::domain_error(id);
    const Symbol::Sort sort = LookupSort(sort_id);
    const Term name = CreateName(sort);
    names_[id] = name;
    lela::format::output::RegisterSymbol(name.symbol(), id);
    std::cout << "RegisterName " << id << " -> " << sort_id << std::endl;
  }

  void RegisterFunction(const std::string& id, int arity, const std::string& sort_id) {
    using lela::format::output::operator<<;
    if (IsRegisteredFunction(id))
      throw std::domain_error(id);
    const Symbol::Sort sort = sorts_[sort_id];
    funs_.emplace(id, CreateFunction(sort, arity));
    Symbol s = funs_.find(id)->second;
    lela::format::output::RegisterSymbol(s, id);
    std::cout << "RegisterFunction " << id << " / " << arity << " -> " << sort_id << std::endl;
  }

  void RegisterFormula(const std::string& id, const Formula& phi) {
    using lela::format::output::operator<<;
    const auto it = formulas_.find(id);
    const bool contained = it != formulas_.end();
    if (contained) {
      formulas_.erase(it);
    }
    formulas_.emplace(id, phi);
    std::cout << "RegisterFormula " << id << " := " << phi;
    if (contained)
      std::cout << " (was previously " << LookupFormula(id) << ")";
    std::cout << std::endl;
  }

  void AddClause(const Clause& c) {
    using lela::format::output::operator<<;
    std::cout << "Adding clause " << c << std::endl;
    solver_.AddClause(c);
  }

  Solver* solver() { return &solver_; }
  const Solver& solver() const { return solver_; }

  Symbol::Factory* sf() { return solver_.sf(); }
  Term::Factory* tf() { return solver_.tf(); }

 private:
  std::map<std::string, Symbol::Sort> sorts_;
  std::map<std::string, Term>         vars_;
  std::map<std::string, Term>         names_;
  std::map<std::string, Symbol>       funs_;
  std::map<std::string, Formula>      formulas_;
  Solver solver_;
};

}  // namespace pdl
}  // namespace format
}  // namespace lela

#endif  // LELA_FORMAT_CONTEXT_H_

