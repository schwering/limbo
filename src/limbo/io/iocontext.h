// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016-2019 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// The IO context is a bidirectional mapping between uninterpreted symbols and
// their string representation. Additionally, simple meta variables are
// implemented.

#ifndef LIMBO_IO_IOCONTEXT_H_
#define LIMBO_IO_IOCONTEXT_H_

#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <limbo/formula.h>

#define LIMBO_REG(sym)            LIMBO_REG_STR(sym, #sym)
#define LIMBO_REG_STR(sym, str)   \
  [](auto& sym0, auto& str0) {\
    struct Dispatcher {\
      void operator()(limbo::Alphabet::Sort sort, const char* s) {\
        limbo::io::IoContext::instance().sort_registry().Register(sort, s);\
      }\
      void operator()(limbo::Alphabet::FunSymbol f, const char* s) {\
        limbo::io::IoContext::instance().fun_registry().Register(f, s);\
      }\
      void operator()(limbo::Alphabet::NameSymbol n, const char* s) {\
        limbo::io::IoContext::instance().name_registry().Register(n, s);\
      }\
      void operator()(limbo::Alphabet::VarSymbol x, const char* s) {\
        limbo::io::IoContext::instance().var_registry().Register(x, s);\
      }\
      void operator()(limbo::io::IoContext::MetaSymbol m, const char *s) {\
        limbo::io::IoContext::instance().meta_registry().Register(m, s);\
      }\
    };\
    Dispatcher()(sym0, str0);\
  }(sym, str);

namespace limbo {
namespace io {

template<typename T>
struct CreateSymbolFunction {};

template<>
struct CreateSymbolFunction<Alphabet::Sort> {
  Alphabet::Sort operator()(bool rigid) const { return Alphabet::instance().CreateSort(rigid); }
};

template<>
struct CreateSymbolFunction<Alphabet::FunSymbol> {
  Alphabet::FunSymbol operator()(int arity) const { return Alphabet::instance().CreateFun(Alphabet::Sort(1), arity); }
};

template<>
struct CreateSymbolFunction<Alphabet::NameSymbol> {
  Alphabet::NameSymbol operator()(int arity) const { return Alphabet::instance().CreateName(Alphabet::Sort(1), arity); }
};

template<>
struct CreateSymbolFunction<Alphabet::VarSymbol> {
  Alphabet::VarSymbol operator()() const { return Alphabet::instance().CreateVar(Alphabet::Sort(1)); }
};

class IoContext : public limbo::internal::Singleton<IoContext> {
 public:
  using Abc = Alphabet;

  struct MetaSymbol : public Abc::IntRepresented {
    using IntRepresented::IntRepresented;
  };

  struct CreateMetaSymbolFunction {
    MetaSymbol operator()() const { return IoContext::instance().CreateMeta(); }
  };

  template<typename Sym, typename CreateSymbolFunction = CreateSymbolFunction<Sym>>
  class SymbolRegistry {
   public:
    explicit SymbolRegistry(CreateSymbolFunction csf = CreateSymbolFunction()) : csf_(csf) {}

    void Register(const Sym sym, const std::string& str) {
      assert(!sym.null());
      assert(str.length() > 0);
      str2sym_[str] = sym;
      sym2str_[sym] = str;
    }

    bool Registered(const Sym sym) const { return sym2str_[sym].length() > 0; }
    bool Registered(const std::string& str) const { return str2sym_.find(str) != str2sym_.end(); }

    const std::string& ToString(const Sym sym, const char* default_string) {
      if (!Registered(sym)) {
        std::stringstream ss;
        ss << default_string << sym.id();
        Register(sym, ss.str());
      }
      return sym2str_[sym];
    }

    template<typename... DefaultFunctionArgs>
    Sym ToSymbol(const std::string& str, DefaultFunctionArgs... args) {
      auto it = str2sym_.find(str);
      if (it == str2sym_.end()) {
        const Sym sym = csf_(args...);
        Register(sym, str);
        it = str2sym_.find(str);
      }
      return it->second;
    }

   private:
    SymbolRegistry(const SymbolRegistry&)            = delete;
    SymbolRegistry(SymbolRegistry&&)                 = delete;
    SymbolRegistry& operator=(const SymbolRegistry&) = delete;
    SymbolRegistry& operator=(SymbolRegistry&&)      = delete;

    CreateSymbolFunction                 csf_;
    Abc::DenseMap<Sym, std::string>      sym2str_;
    std::unordered_map<std::string, Sym> str2sym_;
  };

  using SortRegistry = SymbolRegistry<Abc::Sort>;
  using FunRegistry  = SymbolRegistry<Abc::FunSymbol>;
  using NameRegistry = SymbolRegistry<Abc::NameSymbol>;
  using VarRegistry  = SymbolRegistry<Abc::VarSymbol>;
  using MetaRegistry = SymbolRegistry<MetaSymbol, CreateMetaSymbolFunction>;

  static IoContext& instance() {
    if (instance_ == nullptr) {
      instance_ = std::unique_ptr<IoContext>(new IoContext());
    }
    return *instance_.get();
  }

  static void reset_instance() {
    instance_ = nullptr;
  }

  SortRegistry& sort_registry() { return sort_reg_; }
  FunRegistry&  fun_registry()  { return fun_reg_; }
  NameRegistry& name_registry() { return name_reg_; }
  VarRegistry&  var_registry()  { return var_reg_; }
  MetaRegistry& meta_registry() { return meta_reg_; }

  MetaSymbol CreateMeta() { return MetaSymbol(++last_meta_); }

  bool has_meta_value(MetaSymbol m)           const { return !meta_value_[m].empty(); }
  const Formula& get_meta_value(MetaSymbol m) const { return meta_value_[m]; }

  void set_meta_value(MetaSymbol m, const Formula& f) { meta_value_[m] = f.Clone(); }
  void set_meta_value(MetaSymbol m, Formula&& f)      { meta_value_[m] = std::move(f); }
  void unset_meta_value(MetaSymbol m)                 { meta_value_[m] = Formula(); }

 private:
  explicit IoContext() = default;

  IoContext(const IoContext&)            = delete;
  IoContext& operator=(const IoContext&) = delete;
  IoContext(IoContext&&)                 = delete;
  IoContext& operator=(IoContext&&)      = delete;

  SortRegistry                            sort_reg_;
  FunRegistry                             fun_reg_;
  NameRegistry                            name_reg_;
  VarRegistry                             var_reg_;
  MetaRegistry                            meta_reg_;
  Alphabet::DenseMap<MetaSymbol, Formula> meta_value_;
  int                                     last_meta_ = 0;
};

}  // namespace io
}  // namespace limbo

#endif  // LIMBO_IO_IOCONTEXTH_

