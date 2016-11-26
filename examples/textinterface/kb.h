// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering
//
// The KB is the context object during parsing where symbols are registered and
// formulas are evaluated.

#ifndef EXAMPLES_CLI_KB_H_
#define EXAMPLES_CLI_KB_H_

#include <map>
#include <iostream>
#include <string>
#include <utility>

#include <lela/term.h>
#include <lela/formula.h>
#include <lela/solver.h>
#include <lela/format/output.h>
#include <lela/format/syntax.h>

struct syntax_error : public std::exception {
  explicit syntax_error(const std::string& id) : id_(id) {}
  const char* what() const noexcept override { return id_.c_str(); }
 private:
  std::string id_;
};
struct redeclared_error : public syntax_error { using syntax_error::syntax_error; };
struct undeclared_error : public syntax_error { using syntax_error::syntax_error; };

struct KB {
  KB() : context_(solver_.sf(), solver_.tf()) {}
  ~KB() {
  }

  bool IsRegisteredSort(const std::string& id) const {
    return sorts_.find(id) != sorts_.end();
  }

  bool IsRegisteredVar(const std::string& id) const {
    return vars_.find(id) != vars_.end();
  }

  bool IsRegisteredName(const std::string& id) const {
    return names_.find(id) != names_.end();
  }

  bool IsRegisteredFun(const std::string& id) const {
    return funs_.find(id) != funs_.end();
  }

  bool IsRegisteredFormula(const std::string& id) const {
    return formulas_.find(id) != formulas_.end();
  }

  bool IsRegistered(const std::string& id) const {
    return IsRegisteredSort(id) || IsRegisteredVar(id) || IsRegisteredName(id) || IsRegisteredFun(id) || IsRegisteredFormula(id);
  }

  lela::Symbol::Sort LookupSort(const std::string& id) const {
    const auto it = sorts_.find(id);
    if (it == sorts_.end())
      throw undeclared_error(id);
    return it->second;
  }

  lela::Term LookupVar(const std::string& id) const {
    const auto it = vars_.find(id);
    if (it == vars_.end())
      throw undeclared_error(id);
    return it->second;
  }

  lela::Term LookupName(const std::string& id) const {
    const auto it = names_.find(id);
    if (it == names_.end())
      throw undeclared_error(id);
    return it->second;
  }

  const lela::Symbol& LookupFun(const std::string& id) const {
    const auto it = funs_.find(id);
    if (it == funs_.end())
      throw undeclared_error(id);
    return it->second;
  }

  const lela::Formula& LookupFormula(const std::string& id) const {
    const auto it = formulas_.find(id);
    if (it == formulas_.end())
      throw undeclared_error(id);
    return it->second;
  }

  void RegisterSort(const std::string& id) {
    const lela::Symbol::Sort sort = context_.NewSort();
    //lela::format::RegisterSort(sort, id);
    lela::format::RegisterSort(sort, "");
    sorts_[id] = sort;
    std::cout << "RegisterSort " << id << std::endl;
  }

  void RegisterVar(const std::string& id, const std::string& sort_id) {
    if (IsRegistered(id))
      throw redeclared_error(id);
    const lela::Symbol::Sort sort = LookupSort(sort_id);
    const lela::Term var = context_.NewVar(sort);
    vars_[id] = var;
    lela::format::RegisterSymbol(var.symbol(), id);
    std::cout << "RegisterVar " << id << " -> " << sort_id << std::endl;
  }

  void RegisterName(const std::string& id, const std::string& sort_id) {
    if (IsRegistered(id))
      throw redeclared_error(id);
    const lela::Symbol::Sort sort = LookupSort(sort_id);
    const lela::Term name = context_.NewName(sort);
    names_[id] = name;
    lela::format::RegisterSymbol(name.symbol(), id);
    std::cout << "RegisterName " << id << " -> " << sort_id << std::endl;
  }

  void RegisterFun(const std::string& id, int arity, const std::string& sort_id) {
    if (IsRegistered(id))
      throw redeclared_error(id);
    const lela::Symbol::Sort sort = sorts_[sort_id];
    funs_.emplace(id, context_.NewFun(sort, arity));
    lela::Symbol s = funs_.find(id)->second;
    lela::format::RegisterSymbol(s, id);
    std::cout << "RegisterFun " << id << " / " << arity << " -> " << sort_id << std::endl;
  }

  void RegisterFormula(const std::string& id, const lela::Formula& phi) {
    if (IsRegistered(id))
      throw redeclared_error(id);
    formulas_.emplace(id, phi);
    using lela::format::operator<<;
    std::cout << "RegisterFormula " << id << " -> " << phi << std::endl;
  }

  lela::Solver& solver() { return solver_; }
  const lela::Solver& solver() const { return solver_; }

 private:
  std::map<std::string, lela::Symbol::Sort> sorts_;
  std::map<std::string, lela::Term>         vars_;
  std::map<std::string, lela::Term>         names_;
  std::map<std::string, lela::Symbol>       funs_;
  std::map<std::string, lela::Formula>      formulas_;
  lela::Solver  solver_;
  lela::format::Context context_;
};

#endif  // EXAMPLES_CLI_KB_H_

