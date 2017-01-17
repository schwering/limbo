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
#include <lela/modal.h>
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

  struct RegisterMetaVariableData : public RegisterData {
    RegisterMetaVariableData(const std::string& id, const Term t) : RegisterData(id), term(t) {}
    const Term term;
  };

  struct RegisterFormulaData : public RegisterData {
    RegisterFormulaData(const std::string& id, const Formula& phi) : RegisterData(id), phi(phi.Clone()) {}
    const Formula::Ref phi;
  };

  struct UnregisterData : public LogData {
    explicit UnregisterData(const std::string& id) : id(id) {}
    const std::string id;
  };

  struct UnregisterMetaVariableData : public UnregisterData {
    UnregisterMetaVariableData(const std::string& id) : UnregisterData(id) {}
  };

  struct AddToKbData : public LogData {
    AddToKbData(const Formula& alpha, bool ok) : alpha(alpha), ok(ok) {}
    const Formula& alpha;
    const bool ok;
  };

  struct QueryData : public LogData {
    QueryData(const KnowledgeBase& kb, const Formula& phi, bool assume_consistent, bool yes) :
        kb(kb), phi(phi.Clone()), assume_consistent(assume_consistent), yes(yes) {}
    const KnowledgeBase& kb;
    const Formula::Ref phi;
    const bool assume_consistent;
    const bool yes;
  };

  void operator()(const LogData& d) const {}
};

template<typename LogPredicate>
class Context {
 public:
  explicit Context(LogPredicate p = LogPredicate()) : logger_(p), kb_(&sf_, &tf_) {}

  typedef std::function<void(Context* ctx, const std::vector<Term>& args)> Callback;

  Symbol::Sort CreateSort() { return sf()->CreateSort(); }
  Term CreateVariable(Symbol::Sort sort) { return tf()->CreateTerm(sf()->CreateVariable(sort)); }
  Term CreateName(Symbol::Sort sort) { return tf()->CreateTerm(sf()->CreateName(sort)); }
  Symbol CreateFunction(Symbol::Sort sort, Symbol::Arity arity) { return sf()->CreateFunction(sort, arity); }

  bool IsRegisteredSort(const std::string& id) const { return sorts_.Registered(id); }
  bool IsRegisteredVariable(const std::string& id) const { return vars_.Registered(id); }
  bool IsRegisteredName(const std::string& id) const { return names_.Registered(id); }
  bool IsRegisteredFunction(const std::string& id) const { return funs_.Registered(id); }
  bool IsRegisteredMetaVariable(const std::string& id) const { return meta_vars_.Registered(id); }
  bool IsRegisteredFormula(const std::string& id) const { return formulas_.Registered(id); }
  bool IsRegisteredCallback(const std::string& id) const { return callbacks_.Registered(id); }
  bool IsRegisteredTerm(const std::string& id) const {
    return IsRegisteredVariable(id) || IsRegisteredName(id) || IsRegisteredFunction(id) || IsRegisteredMetaVariable(id);
  }

  Symbol::Sort LookupSort(const std::string& id) const { return sorts_.Find(id); }
  Term LookupVariable(const std::string& id) const { return vars_.Find(id); }
  Term LookupName(const std::string& id) const { return names_.Find(id); }
  Symbol LookupFunction(const std::string& id) const { return funs_.Find(id); }
  Term LookupMetaVariable(const std::string& id) const { return meta_vars_.Find(id); }
  const Formula& LookupFormula(const std::string& id) const { return *formulas_.Find(id); }
  Callback LookupCallback(const std::string& id) const { return callbacks_.Find(id); }

  void RegisterSort(const std::string& id) {
    const Symbol::Sort sort = CreateSort();
    lela::format::output::RegisterSort(sort, "");
    sorts_.Register(id, sort);
    logger_(Logger::RegisterSortData(id));
  }

  void RegisterVariable(const std::string& id, const std::string& sort_id) {
    if (IsRegisteredVariable(id))
      throw std::domain_error(id);
    const Symbol::Sort sort = LookupSort(sort_id);
    const Term var = CreateVariable(sort);
    vars_.Register(id, var);
    lela::format::output::RegisterSymbol(var.symbol(), id);
    logger_(Logger::RegisterVariableData(id, sort_id));
  }

  void RegisterName(const std::string& id, const std::string& sort_id) {
    if (IsRegisteredName(id))
      throw std::domain_error(id);
    const Symbol::Sort sort = LookupSort(sort_id);
    const Term name = CreateName(sort);
    names_.Register(id, name);
    lela::format::output::RegisterSymbol(name.symbol(), id);
    logger_(Logger::RegisterNameData(id, sort_id));
  }

  void RegisterFunction(const std::string& id, int arity, const std::string& sort_id) {
    if (IsRegisteredFunction(id))
      throw std::domain_error(id);
    const Symbol::Sort sort = LookupSort(sort_id);
    const Symbol fun = CreateFunction(sort, arity);
    funs_.Register(id, fun);
    lela::format::output::RegisterSymbol(fun, id);
    logger_(Logger::RegisterFunctionData(id, arity, sort_id));
  }

  void RegisterMetaVariable(const std::string& id, Term t) {
    if (IsRegisteredMetaVariable(id))
      throw std::domain_error(id);
    meta_vars_.Register(id, t);
    logger_(Logger::RegisterMetaVariableData(id, t));
  }

  void RegisterFormula(const std::string& id, const Formula& phi) {
    formulas_.Register(id, phi.Clone());
    logger_(Logger::RegisterFormulaData(id, phi));
  }

  void RegisterCallback(const std::string& id, Callback cb) {
    if (IsRegisteredCallback(id)) {
      throw std::domain_error(id);
    }
    callbacks_.Register(id, cb);
  }

  void UnregisterMetaVariable(const std::string& id) {
    if (!IsRegisteredMetaVariable(id))
      throw std::domain_error(id);
    meta_vars_.Unregister(id);
    logger_(Logger::UnregisterMetaVariableData(id));
  }

  bool AddToKb(const Formula& alpha) {
    const bool ok = kb_.Add(alpha);
    logger_(Logger::AddToKbData(alpha, ok));
    return ok;
  }

  bool Query(const Formula& alpha) {
    const bool yes = kb_.Entails(alpha, assume_consistent_);
    logger()(Logger::QueryData(kb_, alpha, assume_consistent_, yes));
    return yes;
  }

  bool assume_consistent() const { return assume_consistent_; }
  void assume_consistent(bool b) { assume_consistent_ = b; }

  KnowledgeBase* kb() { return &kb_; }
  const KnowledgeBase& kb() const { return kb_; }

  Symbol::Factory* sf() { return &sf_; }
  Term::Factory* tf() { return &tf_; }

  const LogPredicate& logger() const { return logger_; }

 private:
  template<typename T>
  class Registry {
   public:
    bool Registered(const std::string& id) const { auto it = r_.find(id); return it != r_.end(); }
    void Register(const std::string& id, const T& val) { r_.insert(std::make_pair(id, val)); }
    void Register(const std::string& id, T&& val) { Unregister(id); r_.emplace(id, std::forward<T>(val)); }
    void Unregister(const std::string& id) { auto it = r_.find(id); if (it != r_.end()) { r_.erase(it); } }
    const T& Find(const std::string& id) const { auto it = r_.find(id); return it->second; }
   private:
    std::map<std::string, T> r_;
  };

  LogPredicate           logger_;
  Registry<Symbol::Sort> sorts_;
  Registry<Term>         vars_;
  Registry<Term>         names_;
  Registry<Symbol>       funs_;
  Registry<Term>         meta_vars_;
  Registry<Formula::Ref> formulas_;
  Registry<Callback>     callbacks_;
  Symbol::Factory        sf_;
  Term::Factory          tf_;
  KnowledgeBase          kb_;
  bool                   assume_consistent_ = true;
};

}  // namespace pdl
}  // namespace format
}  // namespace lela

#endif  // LELA_FORMAT_PDL_CONTEXT_H_

