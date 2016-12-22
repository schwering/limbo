// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering
//
// Context objects store and create symbols, terms, and allow for textual
// representation, and encapsulate a Solver object.
//
// Results are announced through the LogPredicate functor, which needs to
// implement operator() for the structs defined in Logger. Logger itself is a
// minimal implementation of a LogPredicate, which ignores all log data.

#ifndef LELA_FORMAT_PDL_CONTEXT_H_
#define LELA_FORMAT_PDL_CONTEXT_H_

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

struct Logger {
  struct LogData {};

  struct RegisterData {
    explicit RegisterData(const std::string& id) : id(id) {}
    const std::string id;
  };

  struct RegisterSortData : public RegisterData {
    explicit RegisterSortData(const std::string& id) : RegisterData(id) {}
  };

  struct RegisterNameData : public RegisterData {
    RegisterNameData(const std::string& id, const std::string& sort_id) : RegisterData(id), sort_id(sort_id) {}
    const std::string sort_id;
  };

  struct RegisterVariableData : public RegisterData {
    RegisterVariableData(const std::string& id, const std::string& sort_id) : RegisterData(id), sort_id(sort_id) {}
    const std::string sort_id;
  };

  struct RegisterFunctionData : public RegisterData {
    RegisterFunctionData(const std::string& id, Symbol::Arity arity, const std::string& sort_id)
        : RegisterData(id), arity(arity), sort_id(sort_id) {}
    Symbol::Arity arity;
    const std::string sort_id;
  };

  struct RegisterFormulaData : public RegisterData {
    RegisterFormulaData(const std::string& id, const Formula& phi) : RegisterData(id), phi(phi.Clone()) {}
    const Formula::Ref phi;
  };

  struct AddClauseData : public LogData {
    explicit AddClauseData(const Clause& c) : c(c) {}
    const Clause c;
  };

  struct QueryData : public LogData {
    QueryData(Solver::split_level k, const Setup* s, const Formula& phi, bool yes) :
        k(k), s(s), phi(phi.Clone()), yes(yes) {}
    const Solver::split_level k;
    const Setup* const s;
    const Formula::Ref phi;
    const bool yes;
  };

  struct EntailmentData : public QueryData {
    EntailmentData(Solver::split_level k, const Setup* s, const Formula& phi, bool yes)
        : QueryData(k, s, phi, yes) {}
  };

  struct ConsistencyData : public QueryData {
    ConsistencyData(Solver::split_level k, const Setup* s, const Formula& phi, bool yes)
        : QueryData(k, s, phi, yes) {}
  };

  void operator()(const LogData& d) const {}
};

template<typename LogPredicate>
class Context {
 public:
  explicit Context(LogPredicate p = LogPredicate()) : logger_(p), solver_(&sf_, &tf_) {}

  Symbol::Sort CreateSort() {
    return sf()->CreateSort();
  }

  Term CreateVariable(Symbol::Sort sort) {
    return tf()->CreateTerm(sf()->CreateVariable(sort));
  }

  Term CreateName(Symbol::Sort sort) {
    return tf()->CreateTerm(sf()->CreateName(sort));
  }

  Symbol CreateFunction(Symbol::Sort sort, Symbol::Arity arity) {
    return sf()->CreateFunction(sort, arity);
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
    return *it->second;
  }

  void RegisterSort(const std::string& id) {
    using lela::format::output::operator<<;
    const Symbol::Sort sort = CreateSort();
    lela::format::output::RegisterSort(sort, "");
    sorts_[id] = sort;
    logger_(Logger::RegisterSortData(id));
  }

  void RegisterVariable(const std::string& id, const std::string& sort_id) {
    using lela::format::output::operator<<;
    if (IsRegisteredVariable(id))
      throw std::domain_error(id);
    const Symbol::Sort sort = LookupSort(sort_id);
    const Term var = CreateVariable(sort);
    vars_[id] = var;
    lela::format::output::RegisterSymbol(var.symbol(), id);
    logger_(Logger::RegisterVariableData(id, sort_id));
  }

  void RegisterName(const std::string& id, const std::string& sort_id) {
    using lela::format::output::operator<<;
    if (IsRegisteredName(id))
      throw std::domain_error(id);
    const Symbol::Sort sort = LookupSort(sort_id);
    const Term name = CreateName(sort);
    names_[id] = name;
    lela::format::output::RegisterSymbol(name.symbol(), id);
    logger_(Logger::RegisterNameData(id, sort_id));
  }

  void RegisterFunction(const std::string& id, int arity, const std::string& sort_id) {
    using lela::format::output::operator<<;
    if (IsRegisteredFunction(id))
      throw std::domain_error(id);
    const Symbol::Sort sort = sorts_[sort_id];
    funs_.emplace(id, CreateFunction(sort, arity));
    Symbol s = funs_.find(id)->second;
    lela::format::output::RegisterSymbol(s, id);
    logger_(Logger::RegisterFunctionData(id, arity, sort_id));
  }

  void RegisterFormula(const std::string& id, const Formula& phi) {
    using lela::format::output::operator<<;
    const auto it = formulas_.find(id);
    const bool contained = it != formulas_.end();
    if (contained) {
      formulas_.erase(it);
    }
    formulas_.emplace(id, phi.Clone());
    logger_(Logger::RegisterFormulaData(id, phi));
  }

  void AddClause(const Clause& c) {
    using lela::format::output::operator<<;
    solver_.AddClause(c);
    logger_(Logger::AddClauseData(c));
  }

  Solver* solver() { return &solver_; }
  const Solver& solver() const { return solver_; }

  Symbol::Factory* sf() { return &sf_; }
  Term::Factory* tf() { return &tf_; }

  const LogPredicate& logger() const { return logger_; }

 private:
  LogPredicate                        logger_;
  std::map<std::string, Symbol::Sort> sorts_;
  std::map<std::string, Term>         vars_;
  std::map<std::string, Term>         names_;
  std::map<std::string, Symbol>       funs_;
  std::map<std::string, Formula::Ref> formulas_;
  Symbol::Factory sf_;
  Term::Factory tf_;
  Solver solver_;
};

}  // namespace pdl
}  // namespace format
}  // namespace lela

#endif  // LELA_FORMAT_PDL_CONTEXT_H_

