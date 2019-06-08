// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014-2019 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.

#ifndef LIMBO_IO_OUTPUT_H_
#define LIMBO_IO_OUTPUT_H_

#include <cstdlib>
#include <memory>
#include <ostream>
#include <string>

#include <limbo/clause.h>
#include <limbo/formula.h>
#include <limbo/lit.h>
#include <limbo/internal/maybe.h>
#include <limbo/io/iocontext.h>

namespace limbo {
namespace io {

namespace strings {
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
}

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
  const Alphabet::RWord w = Alphabet::instance().Unstrip(f);
  if (w.empty()) {
    return os << 'f' << int(f);
  } else {
    return os << RFormula(w);
  }
}

std::ostream& operator<<(std::ostream& os, const Name& n) {
  const Alphabet::RWord w = Alphabet::instance().Unstrip(n);
  if (w.empty()) {
    return os << 'n' << int(n);
  } else {
    return os << RFormula(w);
  }
}

std::ostream& operator<<(std::ostream& os, const Lit& a) {
  return os << a.fun() << ' ' << (a.pos() ? strings::kEquals : strings::kNotEquals) << ' ' << a.name();
}

std::ostream& operator<<(std::ostream& os, const Clause& c) {
  return os << sequence(c, strings::kOrS);
}

std::ostream& operator<<(std::ostream& os, const std::vector<Lit>& as) {
  return os << sequence(as);
}

std::ostream& operator<<(std::ostream& os, const Alphabet::Sort& s) {
  return os << IoContext::instance().sort_registry().ToString(s, "s");
}

std::ostream& operator<<(std::ostream& os, const Alphabet::FunSymbol&  f) {
  return os << IoContext::instance().fun_registry().ToString(f, "f");
}

std::ostream& operator<<(std::ostream& os, const Alphabet::NameSymbol& n) {
  return os << IoContext::instance().name_registry().ToString(n, "n");
}

std::ostream& operator<<(std::ostream& os, const Alphabet::VarSymbol&  x) {
  return os << IoContext::instance().var_registry().ToString(x, "x");
}

std::ostream& operator<<(std::ostream& os, const Alphabet::Symbol& s) {
  switch (s.tag) {
    case Alphabet::Symbol::kFun:            return os << s.u.f;
    case Alphabet::Symbol::kName:           return os << s.u.n;
    case Alphabet::Symbol::kVar:            return os << s.u.x;
    case Alphabet::Symbol::kStrippedFun:    return os << strings::kStrip << s.u.f_s << strings::kStrip;
    case Alphabet::Symbol::kStrippedName:   return os << strings::kStrip << s.u.n_s << strings::kStrip;
    case Alphabet::Symbol::kEquals:         return os << strings::kEquals;
    case Alphabet::Symbol::kNotEquals:      return os << strings::kNotEquals;
    case Alphabet::Symbol::kStrippedLit:    return os << strings::kStrip << s.u.a << strings::kStrip;
    case Alphabet::Symbol::kNot:            return os << strings::kNot;
    case Alphabet::Symbol::kOr:             return os << strings::kOr;
    case Alphabet::Symbol::kAnd:            return os << strings::kAnd;
    case Alphabet::Symbol::kExists:         return os << strings::kExists << ' ' << s.u.x;
    case Alphabet::Symbol::kForall:         return os << strings::kForall << ' ' << s.u.x;
    case Alphabet::Symbol::kKnow:           return os << strings::kKnow << '_' << s.u.k;
    case Alphabet::Symbol::kMaybe:          return os << strings::kMaybe << '_' << s.u.k;
    case Alphabet::Symbol::kBelieve:        return os << strings::kBelieve << '_' << s.u.kl.k << ',' << s.u.kl.l;
    case Alphabet::Symbol::kAction:         return os << strings::kAction << ' ';
  }
  std::abort();
}
std::ostream& operator<<(std::ostream& os, const IoContext::MetaSymbol&  m) {
  return os << IoContext::instance().meta_registry().ToString(m, "m");
}

std::ostream& operator<<(std::ostream& os, const Alphabet::RWord& w) {
  return os << sequence(w, " ");
}

std::ostream& operator<<(std::ostream& os, const Alphabet::Word& w) {
  return os << sequence(w, " ");
}

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
    case Alphabet::Symbol::kOr:           return os << sequence(r.args(), strings::kOrS, "[", "]");
    case Alphabet::Symbol::kAnd:          return os << sequence(r.args(), strings::kAndS, "{", "}");
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

template<typename T>
std::ostream& operator<<(std::ostream& os, const limbo::internal::Maybe<T>& m) {
  if (m) {
    os << "Just(" << m.val << ")";
  } else {
    os << "Nothing";
  }
  return os;
}

}  // namespace io

#ifdef LIMBO_SAT_H_
#ifndef NDEBUG
void Sat::Print() const {
  using limbo::io::operator<<;
  auto& o = std::cout;
  auto e = '\n';
  o << "empty_clause_ = " << std::boolalpha << empty_clause_ << e;
  for (int i = 1; i < int(clauses_.size()); ++i) {
    o << "clauses_[" << i << "] = " << int(clauses_[i]) << " = " << clausef_[clauses_[i]] << e;
  }
  for (const Fun f : watchers_.keys()) {
    o << "watchers_[" << f << "] =";
    for (const CRef cr : watchers_[f]) {
      o << " " << int(cr);
    }
    o << e;
  }
  o << "propagate_with_learnt_ = " << std::boolalpha << propagate_with_learnt_ << e;
  o << "clause_bump_step_ = " << clause_bump_step_ << e;
  assert(std::all_of(trail_.begin(), trail_.end(), [this](Lit a) { return satisfies(a); }));
  for (int i = 0; i < int(trail_.size()); ++i) {
    o << "trail_[" << i << "] = " << trail_[i] << " at level " << int(level_of(trail_[i])) << " due to " << int(reason_of(trail_[i]));
    if (reason_of(trail_[i]) == CRef::kNull) {
    } else if (reason_of(trail_[i]) == CRef::kDomain) {
      const Fun f = trail_[i].fun();
      o << " = domain clause for " << f << ":";
      for (const Name n : data_[f].keys()) {
        if (data_[f][n].occurs) {
          o << " " << n;
        }
      }
    } else {
      o << clausef_[reason_of(trail_[i])];
    }
    o << e;
  }
  for (int i = 0; i < int(level_size_.size()); ++i) {
    o << "level_size_[" << i << "] = " << level_size_[i] << e;
    for (int j = i == 0 ? 0 : level_size_[i-1]; j < level_size_[i]; ++j) {
      assert(int(level_of(trail_[j])) == i);
      assert(satisfies(trail_[j]));
    }
  }
  o << "trail_head_ = " << trail_head_ << e;
  o << "trail_eqs_ = " << trail_eqs_ << e;
  for (const Fun f : domain_size_.keys()) {
    o << "trail_neqs_[" << f << "] = " << trail_neqs_[f] << e;
  }
  for (const Fun f : domain_size_.keys()) {
    o << "domain_size_[" << f << "] = " << domain_size_[f] << e;
  }
  for (const Fun f : domain_.keys()) {
    o << "domain_[" << f << "] =";
    for (int i = 0; i < domain_[f].size(); ++i) {
      o << " " << domain_[f][i];
    }
    o << e;
  }
  for (const Fun f : model_.keys()) {
    o << "model_[" << f << "] = " << model_[f] << e;
  }
  for (const Fun f : data_.keys()) {
    for (const Name n : data_[f].keys()) {
      o << "data_[" << f << "][" << n << "] = {" <<
          "occurs = " << data_[f][n].occurs << ", " <<
          "popped = " << data_[f][n].popped << ", " <<
          "model_neq = " << data_[f][n].model_neq << ", " <<
          "seen_subsumed = " << data_[f][n].seen_subsumed << ", " <<
          "wanted = " << data_[f][n].wanted << ", " <<
          "level = " << data_[f][n].level << ", " <<
          "reason = " << int(data_[f][n].reason) << "} " << e;
    }
  }
  for (const Fun f : fun_activity_->keys()) {
    o << "fun_activity_[" << f << "] = " << (*fun_activity_)[f] << e;
  }
  o << "fun_queue_ =";
  for (const Fun f : fun_queue_) {
    o << " " << f;
  }
  o << e;
  o << "fun_bump_step_ = " << fun_bump_step_ << e;
}
#endif
#endif

}  // namespace limbo

using limbo::io::operator<<;  // I too often forget this, so for now, it's here

#endif  // LIMBO_IO_OUTPUT_H_

