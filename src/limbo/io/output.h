// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014-2019 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.

#ifndef LIMBO_IO_OUTPUT_H_
#define LIMBO_IO_OUTPUT_H_

#include <cstdlib>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>

#include <limbo/clause.h>
#include <limbo/formula.h>
#include <limbo/lit.h>

#define LIMBO_REG(t)  limbo::io::Registry::Instance()->Register((t), #t)
#define LIMBO_MARK    (std::cout << __FILE__ << ":" << __LINE__ << std::endl)

namespace limbo {
namespace io {

class Registry : private internal::Singleton<Registry> {
 public:
  static Registry* Instance() {
    if (instance == nullptr) {
      instance = std::unique_ptr<Registry>(new Registry());
    }
    return instance.get();
  }

  static void ResetInstance() {
    instance = nullptr;
  }

  void Register(const Alphabet::Sort& sort,       const std::string& s) { R(&sorts_, sort, s); }
  void Register(const Alphabet::FunSymbol&  fun,  const std::string& s) { R(&funs_,  fun,  s); }
  void Register(const Alphabet::NameSymbol& name, const std::string& s) { R(&names_, name, s); }
  void Register(const Alphabet::VarSymbol&  var,  const std::string& s) { R(&vars_,  var,  s); }

  const std::string& Lookup(const Alphabet::Sort& sort,       const char* def = "s") const { return L(&sorts_, sort, def); }
  const std::string& Lookup(const Alphabet::FunSymbol&  fun,  const char* def = "f") const { return L(&funs_,  fun,  def); }
  const std::string& Lookup(const Alphabet::NameSymbol& name, const char* def = "n") const { return L(&names_, name, def); }
  const std::string& Lookup(const Alphabet::VarSymbol&  var,  const char* def = "x") const { return L(&vars_,  var,  def); }

 private:
  template<typename T>
  using StringMap = Alphabet::DenseMap<T, std::string>;

  Registry() = default;
  Registry(const Registry&) = delete;
  Registry(Registry&&) = delete;
  Registry& operator=(const Registry&) = delete;
  Registry& operator=(Registry&&) = delete;

  template<typename T>
  void R(StringMap<T>* map, const T& x, const std::string& s) {
    (*map)[x] = s;
  }

  template<typename T>
  const std::string& L(StringMap<T>* map, const T& x, const char* def) const {
    if ((*map)[x].length() == 0) {
      std::stringstream ss;
      ss << def << int(x);
      (*map)[x] = ss.str();
    }
    return (*map)[x];
  }

  mutable StringMap<Alphabet::Sort>       sorts_;
  mutable StringMap<Alphabet::FunSymbol>  funs_;
  mutable StringMap<Alphabet::NameSymbol> names_;
  mutable StringMap<Alphabet::VarSymbol>  vars_;
};

struct Strings {
  static constexpr const char* kEquals = "\u003D";
  static constexpr const char* kNotEquals = "\u2260";
  static constexpr const char* kNot = "\u00AC";
  static constexpr const char* kOr = "\u2228";
  static constexpr const char* kOrS = " \u2228 ";
  static constexpr const char* kAnd = "\u2227";
  static constexpr const char* kAndS = " \u2227 ";
  static constexpr const char* kExists = "\u2203";
  static constexpr const char* kForall = "\u2200";
  static constexpr const char* kKnow = "\033[1mK\033[0m";
  static constexpr const char* kMaybe = "\033[1mM\033[0m";
  static constexpr const char* kBelieve = "\033[1mB\033[0m";
  static constexpr const char* kAction = "\033[1mA\033[0m";
  static constexpr const char* kStrip = "|";
};

template<typename InputIt>
struct Sequence {
  InputIt begin;
  InputIt end;
  const char* sep;
  const char* lead;
  const char* trail;
};

template<typename T, typename InputIt = decltype(std::declval<T const>().begin())>
Sequence<InputIt> sequence(const T& range,
                           const char* sep = ",",
                           const char* lead = "",
                           const char* trail = "") {
  return Sequence<InputIt>{range.begin(), range.end(), sep, lead, trail};
}

template<typename InputIt>
std::ostream& operator<<(std::ostream& os, const Sequence<InputIt>& s);
std::ostream& operator<<(std::ostream& os, const RFormula& w);

std::ostream& operator<<(std::ostream& os, const Fun& f) {
  const Alphabet::RWord w = Alphabet::Instance()->Unstrip(Alphabet::Symbol::StrippedFun(f));
  if (w.empty()) {
    return os << 'f' << int(f);
  } else {
    return os << RFormula(w);
  }
}

std::ostream& operator<<(std::ostream& os, const Name& n) {
  const Alphabet::RWord w = Alphabet::Instance()->Unstrip(Alphabet::Symbol::StrippedName(n));
  if (w.empty()) {
    return os << 'n' << int(n);
  } else {
    return os << RFormula(w);
  }
}

std::ostream& operator<<(std::ostream& os, const Lit& a) {
  return os << a.fun() << ' ' << (a.pos() ? Strings::kEquals : Strings::kNotEquals) << ' ' << a.name();
}

std::ostream& operator<<(std::ostream& os, const Clause& c) { return os << sequence(c, Strings::kOrS); }
std::ostream& operator<<(std::ostream& os, const std::vector<Lit>& as) { return os << sequence(as); }

std::ostream& operator<<(std::ostream& os, const Alphabet::Sort& s)       { return os << Registry::Instance()->Lookup(s); }
std::ostream& operator<<(std::ostream& os, const Alphabet::FunSymbol&  f) { return os << Registry::Instance()->Lookup(f); }
std::ostream& operator<<(std::ostream& os, const Alphabet::NameSymbol& n) { return os << Registry::Instance()->Lookup(n); }
std::ostream& operator<<(std::ostream& os, const Alphabet::VarSymbol&  x) { return os << Registry::Instance()->Lookup(x); }

std::ostream& operator<<(std::ostream& os, const Alphabet::Symbol& s) {
  switch (s.tag) {
    case Alphabet::Symbol::kFun:            return os << s.u.f;
    case Alphabet::Symbol::kName:           return os << s.u.n;
    case Alphabet::Symbol::kVar:            return os << s.u.x;
    case Alphabet::Symbol::kStrippedFun:    return os << Strings::kStrip << s.u.f_s << Strings::kStrip;
    case Alphabet::Symbol::kStrippedName:   return os << Strings::kStrip << s.u.n_s << Strings::kStrip;
    case Alphabet::Symbol::kEquals:         return os << Strings::kEquals;
    case Alphabet::Symbol::kNotEquals:      return os << Strings::kNotEquals;
    case Alphabet::Symbol::kStrippedLit:    return os << Strings::kStrip << s.u.a << Strings::kStrip;
    case Alphabet::Symbol::kNot:            return os << Strings::kNot << ' ';
    case Alphabet::Symbol::kOr:             return os << Strings::kOr;
    case Alphabet::Symbol::kAnd:            return os << Strings::kAnd;
    case Alphabet::Symbol::kExists:         return os << Strings::kExists << ' ' << s.u.x;
    case Alphabet::Symbol::kForall:         return os << Strings::kForall << ' ' << s.u.x;
    case Alphabet::Symbol::kKnow:           return os << Strings::kKnow << '_' << s.u.k;
    case Alphabet::Symbol::kMaybe:          return os << Strings::kMaybe << '_' << s.u.k;
    case Alphabet::Symbol::kBelieve:        return os << Strings::kBelieve << '_' << s.u.k << ',' << s.u.l;
    case Alphabet::Symbol::kAction:         return os << Strings::kAction << ' ';
  }
  std::abort();
}

std::ostream& operator<<(std::ostream& os, const Alphabet::RWord& w) { return os << sequence(w, " "); }
std::ostream& operator<<(std::ostream& os, const Alphabet::Word&  w) { return os << sequence(w, " "); }

std::ostream& operator<<(std::ostream& os, const RFormula& r) {
  switch (r.tag()) {
    case Alphabet::Symbol::kFun:
    case Alphabet::Symbol::kVar:
    case Alphabet::Symbol::kName:         return os << r.head() << sequence(r.args(), ",",
                                                                            r.arity() > 0 ? "(" : "",
                                                                            r.arity() > 0 ? ")" : "");
    case Alphabet::Symbol::kEquals:
    case Alphabet::Symbol::kNotEquals:    return os << r.arg(0) << ' ' << r.head() << ' ' << r.arg(1);
    case Alphabet::Symbol::kStrippedFun:
    case Alphabet::Symbol::kStrippedName:
    case Alphabet::Symbol::kStrippedLit:  return os << r.head();
    case Alphabet::Symbol::kOr:           return os << sequence(r.args(), Strings::kOrS, "[", "]");
    case Alphabet::Symbol::kAnd:          return os << sequence(r.args(), Strings::kAndS, "{", "}");
    case Alphabet::Symbol::kNot:
    case Alphabet::Symbol::kExists:
    case Alphabet::Symbol::kForall:
    case Alphabet::Symbol::kKnow:
    case Alphabet::Symbol::kMaybe:        return os << r.head() << ' ' << r.arg(0);
    case Alphabet::Symbol::kBelieve:      return os << r.head() << ' ' << r.arg(0) << " \u27FE " << r.arg(1);
    case Alphabet::Symbol::kAction:       return os << r.head() << ' ' << r.arg(0) << ' ' << r.arg(1);
  }
  std::abort();
}

std::ostream& operator<<(std::ostream& os, const Formula& f) { return os << f.readable(); }

template<typename InputIt>
std::ostream& operator<<(std::ostream& os, const Sequence<InputIt>& s) {
  os << s.lead;
  for (auto it = s.begin; it != s.end; ++it) {
    if (it != s.begin) {
      os << s.sep;
    }
    os << *it;
  }
  os << s.trail;
  return os;
}

}  // namespace io
}  // namespace limbo

using limbo::io::operator<<;  // I too often forget this, so for now, it's here

#endif  // LIMBO_IO_OUTPUT_H_

