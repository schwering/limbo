// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016-2019 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// Recursive descent parser for the problem description language. The grammar
// for formulas is aims to reduce brackets and implement operator precedence.
// See the comment above Parser::start() and its callees for the grammar
// definition. The IoContext template parameter is merely passed around to be
// the argument of Parser::Computation functors, as returned by Parser::Parse().

#ifndef LIMBO_IO_PARSER_H_
#define LIMBO_IO_PARSER_H_

#include <cassert>

#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <list>

#include <limbo/formula.h>
#include <limbo/internal/maybe.h>
#include <limbo/io/iocontext.h>
#include <limbo/io/lexer.h>
#include <limbo/io/output.h>

namespace limbo {
namespace io {

#define LIMBO_S(x)          #x
#define LIMBO_S_(x)         LIMBO_S(x)
#define LIMBO_STR__LINE__   LIMBO_S_(__LINE__)
#define LIMBO_MSG(msg)      (std::string(msg) +" (in rule "+ __FUNCTION__ +":"+ LIMBO_STR__LINE__ +")")

struct DefaultEventHandler {
  void SortRegistered(Alphabet::Sort sort) {
    std::cout << "Registered sort " << sort << std::endl;
  }

  void FunSymbolRegistered(Alphabet::FunSymbol f) {
    std::cout << "Registered function symbol " << f << std::endl;
  }

  void SensorFunSymbolRegistered(Alphabet::FunSymbol f) {
    std::cout << "Registered sensor function symbol " << f << std::endl;
  }

  void NameSymbolRegistered(Alphabet::NameSymbol n) {
    std::cout << "Registered name symbol " << n << std::endl;
  }

  void VarSymbolRegistered(Alphabet::VarSymbol x) {
    std::cout << "Registered variable symbol " << x << std::endl;
  }

  void MetaSymbolRegistered(IoContext::MetaSymbol m) {
    std::cout << "Registered meta symbol " << IoContext::instance().meta_registry().ToString(m, "m") << std::endl;
  }

  bool Add(const Formula& f) {
    std::cout << "Added " << f << std::endl;
    return true;
  }

  bool Query(const Formula& f) {
    std::cout << "Queried " << f << std::endl;
    return true;
  }
};

template<typename ForwardIt, typename EventHandler = DefaultEventHandler>
class Parser {
 public:
  using Lexer         = Lexer<ForwardIt>;
  using TokenIterator = typename Lexer::TokenIterator;

  struct Void {
    friend std::ostream& operator<<(std::ostream& os, Void) { return os; }
  };

  // Encapsulates a parsing result, either a Success, an Unapplicable, or a Error.
  template<typename T = Void>
  class Result {
   public:
    typedef T type;
    enum Type { kSuccess, kUnapplicable, kError };

    explicit Result() = default;

    explicit Result(T&& val) : val(std::forward<T>(val)), type_(kSuccess) {}  // XXX necessary?

    explicit Result(Type type,
                    const std::string& msg,
                    ForwardIt begin = ForwardIt(),
                    ForwardIt end = ForwardIt(),
                    T&& val = T())
        : val(std::forward<T>(val)), type_(type), msg_(msg), begin_(begin), end_(end) {}  // XXX necessary?

    Result(const Result&)            = delete;
    Result& operator=(const Result&) = delete;
    Result(Result&&)                 = default;
    Result& operator=(Result&&)      = default;

    explicit operator bool() const { return type_ == kSuccess; }
    bool successful() const { return type_ == kSuccess; }
    bool applied() const { return type_ != kUnapplicable; }

    const std::string& msg() const { return msg_; }

    ForwardIt begin() const { return begin_; }
    ForwardIt end()   const { return end_; }

    std::string str() const {
      std::stringstream ss;
      if (*this) {
        ss << "Success: " << val;
      } else {
        ss << msg() << std::endl;
        ss << "with remaining input: \"" << std::string(begin(), end()) << "\"";
      }
      return ss.str();
    }

    std::string remaining_input() const { return std::string(begin(), end()); }

    T val;

   private:
    Type type_;
    std::string msg_;

    ForwardIt begin_;
    ForwardIt end_;
  };

  template<typename T = Void>
  class Computation : public std::shared_ptr<std::function<Result<T>()>> {
   public:
    using Function = std::function<Result<T>()>;
    using Base = std::shared_ptr<Function>;

    Computation() = default;

    template<typename NullaryFunction>
    Computation(NullaryFunction func) : Base::shared_ptr(new Function(func)) {}

    Computation(const Computation&)            = default;
    Computation& operator=(const Computation&) = default;
    Computation(Computation&&)                 = default;
    Computation& operator=(Computation&&)      = default;

    Result<T> Compute() const {
      Function* f = Base::get();
      if (f) {
        return (*f)();
      } else {
        return Result<T>(Result<T>::kError, LIMBO_MSG("Computation is null"));
      }
    }

    Computation& operator+=(Computation&& b) {
      if (!*this) {
        *this = b;
        return *this;
      } else if (!b) {
        return *this;
      } else {
        *this = [this, b = std::move(b)]() {
          Result<> r = (*(*this))();
          if (!r) {
            return std::move(r);
          }
          return (*b)();
        };
        return *this;
      }
    }
  };

  Parser(ForwardIt begin, ForwardIt end, EventHandler eh = EventHandler())
      : eh_(eh), lexer_(begin, end), begin_(lexer_.begin()), end_(lexer_.end()) {}

  void set_default_if_undeclared(bool b) { default_if_undeclared_ = b; }
  bool default_if_undeclared() const { return default_if_undeclared_; }

  Result<Computation<>>        Parse()        { return start(); }
  Result<Computation<Formula>> ParseFormula() { return formula(); }

 private:
  using Abc = Alphabet;

  template<typename T>
  using Maybe = limbo::internal::Maybe<T>;

  static_assert(std::is_convertible<typename ForwardIt::iterator_category, std::forward_iterator_tag>::value,
                "ForwardIt has wrong iterator category");

  template<typename T = Void>
  static Result<T> Success(T&& result = T()) { return Result<T>(std::forward<T>(result)); }

  template<typename T = Void>
  Result<T> Error(const std::string& msg) const {
    std::stringstream ss;
    ss << kErrorLabel << msg;
    return Result<T>(Result<T>::kError, ss.str(), begin().char_iter(), end().char_iter());
  }

  template<typename T = Void, typename U>
  static Result<T> Error(const std::string& msg, const Result<U>& r) {
    std::stringstream ss;
    ss << r.msg() << std::endl << kCausesLabel << msg;
    return Result<T>(Result<T>::kError, ss.str(), r.begin(), r.end());
  }

  template<typename T = Void>
  Result<T> Unapplicable(const std::string& msg) const {
    std::stringstream ss;
    ss << kUnapplicableLabel << msg;
    return Result<T>(Result<T>::kUnapplicable, ss.str(), begin().char_iter(), end().char_iter());
  }

  // declaration --> [ Rigid ] Sort <sort-id> [ , <sort-id>]*
  //              |  Var <id> [ , <id> ]* -> <sort-id>
  //              |  Name <id> [ , <id> ]* -> <sort-id>
  //              |  [ Sensor ] Fun <id> [ , <id> ]* / <arity> -> <sort-id>
  Result<Computation<>> declaration() {
    if ((Is(Tok(), Token::kRigid) && Is(Tok(1), Token::kSort)) || Is(Tok(), Token::kSort)) {
      Computation<> a;
      bool rigid = Is(Tok(), Token::kRigid);
      if (rigid) {
        Advance();
      }
      do {
        Advance();
        if (Is(Tok(), Token::kIdentifier)) {
          const std::string id = Tok().val.str();
          Advance();
          a += [this, rigid, id]() {
            if (!sort_registry().Registered(id)) {
              const Abc::Sort sort = abc().CreateSort(rigid);
              sort_registry().Register(sort, id);
              eh_.SortRegistered(sort);
              return Success<>();
            } else {
              return Error<>(LIMBO_MSG("Sort "+ id +" is already registered"));
            }
          };
        } else {
          return Error<Computation<>>(LIMBO_MSG("Expected sort identifier"));
        }
      } while (Is(Tok(), Token::kComma));
      return Success<Computation<>>(std::move(a));
    }
    if (Is(Tok(), Token::kVar) || Is(Tok(), Token::kName)) {
      const bool var = Is(Tok(), Token::kVar);
      std::list<std::string> ids;
      do {
        Advance();
        if (Is(Tok(), Token::kIdentifier)) {
          ids.push_back(Tok().val.str());
          Advance();
        } else {
          return Error<Computation<>>(LIMBO_MSG(var ? "Expected variable identifier" : "Expected name identifier"));
        }
      } while (Is(Tok(0), Token::kComma));
      if (Is(Tok(0), Token::kRArrow) &&
          Is(Tok(1), Token::kIdentifier)) {
        const std::string sort_id = Tok(1).val.str();
        Advance(2);
        Computation<> a;
        for (const std::string& id : ids) {
          a += [this, var, sort_id, id]() {
            if (default_sort_string(sort_id) || sort_registry().Registered(sort_id)) {
              if (!fun_registry().Registered(id) &&
                  !name_registry().Registered(id) &&
                  !var_registry().Registered(id) &&
                  !meta_registry().Registered(id)) {
                const Abc::Sort sort = sort_registry().ToSymbol(sort_id, false);
                if (var) {
                  const Abc::VarSymbol x = abc().CreateVar(sort);
                  var_registry().Register(x, id);
                  eh_.VarSymbolRegistered(x);
                } else {
                  const int arity = 0;  // TODO name arities?
                  const Abc::NameSymbol n = abc().CreateName(sort, arity);
                  name_registry().Register(n, id);
                  eh_.NameSymbolRegistered(n);
                }
                return Success<>();
              } else {
                return Error<>(LIMBO_MSG("Term "+ id +" is already registered"));
              }
            } else {
              return Error<>(LIMBO_MSG("Sort "+ sort_id +" is not registered"));
            }
          };
        }
        return Success<Computation<>>(std::move(a));
      } else {
        return Error<Computation<>>(LIMBO_MSG("Expected arrow and sort identifier"));
      }
    }
    if ((Is(Tok(), Token::kSensor) && Is(Tok(1), Token::kFun)) || Is(Tok(), Token::kFun)) {
      struct Decl {
        std::string fun_id;
        int arity;
        std::string sensor_id;
      };
      std::list<Decl> ids;
      const bool sensor = Is(Tok(), Token::kSensor);
      if (sensor) {
        Advance();
      }
      do {
        Advance();
        if (Is(Tok(0), Token::kIdentifier) &&
            Is(Tok(1), Token::kSlash) &&
            ((!sensor && Is(Tok(2), Token::kUint)) || (sensor && Is(Tok(2), Token::kIdentifier)))) {
          Decl d;
          d.fun_id = Tok(0).val.str();
          d.arity = !sensor ? std::stoi(Tok(2).val.str()) : 0;
          d.sensor_id = sensor ? Tok(2).val.str() : "";
          ids.push_back(d);
          Advance(3);
        } else {
          return Error<Computation<>>(LIMBO_MSG("Expected function identifier"));
        }
      } while (Is(Tok(0), Token::kComma));
      if (Is(Tok(0), Token::kRArrow) && Is(Tok(1), Token::kIdentifier)) {
        const std::string sort_id = Tok(1).val.str();
        Advance(2);
        Computation<> a;
        for (const Decl& d : ids) {
          a += [this, sensor, sort_id, d]() {
            if (!sort_registry().Registered(sort_id)) {
              return Error<>(LIMBO_MSG("Sort "+ sort_id +" is not registered"));
            }
            if (fun_registry().Registered(d.fun_id) ||
                name_registry().Registered(d.fun_id) ||
                var_registry().Registered(d.fun_id) ||
                meta_registry().Registered(d.fun_id)) {
              return Error<>(LIMBO_MSG("Term "+ d.fun_id +" is already registered"));
            }
            if (sensor && !sort_registry().Registered(d.sensor_id)) {
              return Error<>(LIMBO_MSG("Sensor sort "+ d.sensor_id +" is not registered"));
            }
            const Abc::Sort sort = sort_registry().ToSymbol(sort_id, false);
            const Abc::FunSymbol f = abc().CreateFun(sort, d.arity);
            fun_registry().Register(f, d.fun_id);
            if (!sensor) {
              //io().RegisterSensorFunction(d.fun_id, sort_id, d.sensor_id);
              //eh_.SensorFunRegistered(f);
            } else {
              eh_.FunSymbolRegistered(f);
            }
            return Success<>();
          };
        }
        return Success<Computation<>>(std::move(a));
      } else {
        return Error<Computation<>>(LIMBO_MSG("Expected arrow and sort identifier"));
      }
    }
    return Unapplicable<Computation<>>(LIMBO_MSG("Expected 'Rigid', 'Sort', 'Var', 'Name', 'Fun' or 'Sensor'"));
  }

  // atomic_term --> x
  //              |  n
  //              |  f
  Result<Computation<Formula>> atomic_term() {
    if (Is(Tok(), Token::kIdentifier)) {
      const std::string id = Tok().val.str();
      Advance();
      return Success<Computation<Formula>>([this, id]() {
        if (default_var_string(id) || var_registry().Registered(id)) {
          return Success<Formula>(Formula::Var(var_registry().ToSymbol(id)));
        } else if (default_name_string(id) || name_registry().Registered(id)) {
          return Success<Formula>(Formula::Name(name_registry().ToSymbol(id, 0)));
        } else if (default_fun_string(id) || fun_registry().Registered(id)) {
          Abc::FunSymbol f = fun_registry().ToSymbol(id, 0);
          if (f.arity() != 0) {
            return Error<Formula>(LIMBO_MSG("Wrong number of arguments for "+ id));
          }
          return Success(Formula::Fun(f));
        } else {
          return Error<Formula>(LIMBO_MSG("Error in atomic_term"));
        }
      });
    }
    return Error<Computation<Formula>>(LIMBO_MSG("Expected a declared variable/name/function identifier"));
  }

  // term --> x
  //       |  n
  //       |  f
  //       |  f(term, ..., term)
  Result<Computation<Formula>> term() {
    if (Is(Tok(), Token::kIdentifier)) {
      const std::string id = Tok().val.str();
      Advance();
      std::list<Computation<Formula>> args;
      if (Is(Tok(), Token::kLParen)) {
        Advance();
        for (;;) {
          Result<Computation<Formula>> t = term();
          if (!t) {
            return Error<Computation<Formula>>(LIMBO_MSG("Expected argument term"), t);
          }
          args.push_back(t.val);
          if (Is(Tok(), Token::kComma)) {
            Advance();
            continue;
          } else if (Is(Tok(), Token::kRParen)) {
            Advance();
            break;
          } else {
            return Error<Computation<Formula>>(LIMBO_MSG("Expected comma ',' or closing parenthesis ')'"));
          }
        }
      }
      return Success<Computation<Formula>>([this, id, args_a = args]() {
        if (default_var_string(id) || var_registry().Registered(id)) {
          const Abc::VarSymbol x = var_registry().ToSymbol(id);
          if (0 != int(args_a.size())) {
            return Error<Formula>(LIMBO_MSG("Wrong number of arguments for "+ id));
          }
          return Success<Formula>(Formula::Var(x));
        } else if (default_name_string(id) || name_registry().Registered(id)) {
          const Abc::NameSymbol n = name_registry().ToSymbol(id, 0);
          if (n.arity() != int(args_a.size())) {
            return Error<Formula>(LIMBO_MSG("Wrong number of arguments for "+ id));
          }
          std::list<Formula> args;
          for (const Computation<Formula>& a : args_a) {
            Result<Formula> t = a.Compute();
            if (t) {
              args.push_back(std::move(t.val));
            } else {
              return Error<Formula>(LIMBO_MSG("Expected argument term"), t);
            }
          }
          return Success<Formula>(Formula::Name(n, std::move(args)));
        } else if (default_fun_string(id) || fun_registry().Registered(id)) {
          const Abc::FunSymbol f = fun_registry().ToSymbol(id, 0);
          if (f.arity() != int(args_a.size())) {
            return Error<Formula>(LIMBO_MSG("Wrong number of arguments for "+ id));
          }
          std::list<Formula> args;
          for (const Computation<Formula>& a : args_a) {
            Result<Formula> t = a.Compute();
            if (t) {
              args.push_back(std::move(t.val));
            } else {
              return Error<Formula>(LIMBO_MSG("Expected argument term"), t);
            }
          }
          return Success(Formula::Fun(f, std::move(args)));
        } else if (default_meta_string(id) || meta_registry().Registered(id)) {
          const typename IoContext::MetaSymbol m = meta_registry().ToSymbol(id);  // XXX why typename??
          const Formula& alpha = io().get_meta_value(m);
          return Success(alpha.Clone());
        } else {
          return Error<Formula>(LIMBO_MSG("Error in term"));
        }
      });
    }
    return Error<Computation<Formula>>(LIMBO_MSG("Expected a declared variable/name/function identifier"));
  }

  // literal --> term [ '==' | '!=' ] term
  Result<Computation<Formula>> literal() {
    Result<Computation<Formula>> lhs = term();
    if (!lhs) {
      return Error<Computation<Formula>>(LIMBO_MSG("Expected a lhs term"), lhs);
    }
    bool pos;
    if (Is(Tok(), Token::kEquality) || Is(Tok(), Token::kInequality)) {
      pos = Is(Tok(), Token::kEquality);
      Advance();
    } else {
      return Error<Computation<Formula>>(LIMBO_MSG("Expected equality or inequality '=='/'!='"));
    }
    Result<Computation<Formula>> rhs = term();
    if (!rhs) {
      return Error<Computation<Formula>>(LIMBO_MSG("Expected rhs term"), rhs);
    }
    return Success<Computation<Formula>>([lhs_a = lhs.val, pos, rhs_a = rhs.val]() {
      Result<Formula> lhs = lhs_a.Compute();
      if (!lhs) {
        return Error<Formula>(LIMBO_MSG("Expected a lhs term"), lhs);
      }
      Result<Formula> rhs = rhs_a.Compute();
      if (!rhs) {
        return Error<Formula>(LIMBO_MSG("Expected a rhs term"), rhs);
      }
      Formula a = pos
          ? Formula::Equals(std::move(lhs.val), std::move(rhs.val))
          : Formula::NotEquals(std::move(lhs.val), std::move(rhs.val));
      return Success(std::move(a));
    });
  }

  // primary_formula --> ! primary_formula
  //                  |  Ex atomic_term primary_formula
  //                  |  Fa atomic_term primary_formula
  //                  |  Know < k > primary_formula
  //                  |  Maybe < k > primary_formula
  //                  |  Bel < k , l > primary_formula => primary_formula
  //                  |  Guarantee primary_formula
  //                  |  [ term ] primary_formula
  //                  |  REG primary_formula
  //                  |  ( formula )
  //                  |  abbreviation
  //                  |  literal
  Result<Computation<Formula>> primary_formula() {
    if (Is(Tok(), Token::kNot)) {
      Advance();
      Result<Computation<Formula>> alpha = primary_formula();
      if (!alpha) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected a primary formula within negation"), std::move(alpha));
      }
      return Success<Computation<Formula>>([alpha_a = std::move(alpha.val)]() {
        Result<Formula> alpha = alpha_a.Compute();
        if (!alpha) {
          return Error<Formula>(LIMBO_MSG("Expected a primary formula within negation"), alpha);
        }
        return Success<Formula>(Formula::Not(std::move(alpha.val)));
      });
    }
    if (Is(Tok(), Token::kExists) || Is(Tok(), Token::kForall)) {
      bool ex = Is(Tok(), Token::kExists);
      Advance();
      Result<Computation<Formula>> x = atomic_term();
      if (!x) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected variable in quantifier"), x);
      }
      Result<Computation<Formula>> alpha = primary_formula();
      if (!alpha) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected primary formula within quantifier"), alpha);
      }
      return Success<Computation<Formula>>([ex = std::move(ex),
                                            x_a = std::move(x.val),
                                            alpha_a = std::move(alpha.val)]() {
        Result<Formula> x = x_a.Compute();
        if (!x || !x.val.head().var()) {
          return Error<Formula>(LIMBO_MSG("Expected variable in quantifier"), x);
        }
        Result<Formula> alpha = alpha_a.Compute();
        if (!alpha) {
          return Error<Formula>(LIMBO_MSG("Expected primary formula within quantifier"), alpha);
        }
        return Success(ex ? Formula::Exists(x.val.head().u.x, std::move(alpha.val))
                          : Formula::Forall(x.val.head().u.x, std::move(alpha.val)));
      });
    }
    if (Is(Tok(), Token::kKnow) || Is(Tok(), Token::kMaybe)) {
      const bool know = Is(Tok(), Token::kKnow);
      Advance();
      if (!Is(Tok(), Token::kLess)) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected '<'"));
      }
      Advance();
      if (!Is(Tok(), Token::kUint)) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected split level integer"));
      }
      const int belief_k = std::stoi(Tok().val.str());
      Advance();
      if (!Is(Tok(), Token::kGreater)) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected '>'"));
      }
      Advance();
      Result<Computation<Formula>> alpha = primary_formula();
      if (!alpha) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected primary formula within modality"), alpha);
      }
      return Success<Computation<Formula>>([know, belief_k, alpha_a = std::move(alpha.val)]() {
        Result<Formula> alpha = alpha_a.Compute();
        if (!alpha) {
          return Error<Formula>(LIMBO_MSG("Expected primary formula within modality"), alpha);
        }
        if (know) {
          return Success(Formula::Know(belief_k, std::move(alpha.val)));
        } else {
          return Success(Formula::Maybe(belief_k, std::move(alpha.val)));
        }
      });
    }
    if (Is(Tok(), Token::kBelieve)) {
      Advance();
      if (!Is(Tok(), Token::kLess)) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected '<'"));
      }
      Advance();
      if (!Is(Tok(), Token::kUint)) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected first split level integer"));
      }
      const int belief_k = std::stoi(Tok().val.str());
      Advance();
      if (!Is(Tok(), Token::kComma)) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected ','"));
      }
      Advance();
      if (!Is(Tok(), Token::kUint)) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected second split level integer"));
      }
      const int belief_l = std::stoi(Tok().val.str());
      Advance();
      if (!Is(Tok(), Token::kGreater)) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected '>'"));
      }
      Advance();
      Result<Computation<Formula>> alpha = primary_formula();
      if (!alpha) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected primary formula within modality"), alpha);
      }
      if (!Is(Tok(), Token::kDoubleRArrow)) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected conditional belief arrow"));
      }
      Advance();
      Result<Computation<Formula>> beta = primary_formula();
      if (!beta) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected primary formula within modality"), beta);
      }
      return Success<Computation<Formula>>([belief_k,
                                           belief_l,
                                            alpha_a = std::move(alpha.val),
                                            beta_a = std::move(beta.val)]() {
        Result<Formula> alpha = alpha_a.Compute();
        if (!alpha) {
          return Error<Formula>(LIMBO_MSG("Expected primary formula within modality"), alpha);
        }
        Result<Formula> beta = beta_a.Compute();
        if (!beta) {
          return Error<Formula>(LIMBO_MSG("Expected primary formula within modality"), beta);
        }
        return Success(Formula::Believe(belief_k, belief_l, std::move(alpha.val), std::move(beta.val)));
      });
    }
    if (Is(Tok(), Token::kGuarantee)) {
      return Error<Computation<Formula>>(LIMBO_MSG("Guarantee currently not implemented"));
      Advance();
      Result<Computation<Formula>> alpha = primary_formula();
      if (!alpha) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected primary formula within modality"), alpha);
      }
      return Success<Computation<Formula>>([alpha_a = std::move(alpha.val)]() {
        Result<Formula> alpha = alpha_a.Compute();
        if (!alpha) {
          return Error<Formula>(LIMBO_MSG("Expected primary formula within modality"), alpha);
        }
        //return Success(Formula::Factory::Guarantee(std::move(alpha.val)));
        return Error<Formula>(LIMBO_MSG("Guarantee currently not implemented"), alpha);
      });
    }
    if (Is(Tok(), Token::kLBracket)) {
      Advance();
      Result<Computation<Formula>> t = term();
      if (!t) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected a term in action"), t);
      }
      if (!Is(Tok(), Token::kRBracket)) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected ']'"));
      }
      Advance();
      Result<Computation<Formula>> alpha = primary_formula();
      if (!alpha) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected a primary formula within action"), alpha);
      }
      return Success<Computation<Formula>>([t_a = std::move(t.val), alpha_a = std::move(alpha.val)]() {
        Result<Formula> t = t_a.Compute();
        if (!t) {
          return Error<Formula>(LIMBO_MSG("Expected a term in action"), t);
        }
        Result<Formula> alpha = alpha_a.Compute();
        if (!alpha) {
          return Error<Formula>(LIMBO_MSG("Expected a primary formula within negation"), alpha);
        }
        return Success<Formula>(Formula::Action(std::move(t.val), std::move(alpha.val)));
      });
    }
    if (Is(Tok(), Token::kRegress)) {
      return Error<Computation<Formula>>(LIMBO_MSG("Regression currently not implemented"));
      Advance();
      Result<Computation<Formula>> alpha = primary_formula();
      if (!alpha) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected primary formula within regression operator"), alpha);
      }
      return Success<Computation<Formula>>([alpha_a = std::move(alpha.val)]() {
        Result<Formula> alpha = alpha_a.Compute();
        if (!alpha) {
          return Error<Formula>(LIMBO_MSG("Expected primary formula within modality"), alpha);
        }
        return Error<Formula>(LIMBO_MSG("Regression currently not implemented"), alpha);
        //return Success(io().Regress(*alpha.val));
      });
    }
    if (Is(Tok(), Token::kLParen)) {
      Advance();
      Result<Computation<Formula>> alpha = formula();
      if (!alpha) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected formula within brackets"), alpha);
      }
      if (!Is(Tok(), Token::kRParen)) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected closing right parenthesis ')'"));
      }
      Advance();
      return Success<Computation<Formula>>([alpha_a = std::move(alpha.val)]() {
        Result<Formula> alpha = alpha_a.Compute();
        if (!alpha) {
          return Error<Formula>(LIMBO_MSG("Expected formula within brackets"), alpha);
        }
        return Success(std::move(alpha.val));
      });
    }
    if (Is(Tok(), Token::kIdentifier) &&
        !(Is(Tok(1), Token::kLParen) || Is(Tok(1), Token::kEquality) || Is(Tok(1), Token::kInequality))) {
      std::string id = Tok().val.str();
      Advance();
      return Success<Computation<Formula>>([this, id]() {
        return Error<Formula>(LIMBO_MSG("Formula abbreviations currently not implemented"));
      });
    }
    Result<Computation<Formula>> a = literal();
    if (!a) {
      return Error<Computation<Formula>>(LIMBO_MSG("Expected literal"), a);
    }
    return Success<Computation<Formula>>([a_a = std::move(a.val)]() {
      Result<Formula> a = a_a.Compute();
      if (!a) {
        return Error<Formula>(LIMBO_MSG("Expected literal"), a);
      }
      return Success(std::move(a.val));
    });
  }

  // conjunctive_formula --> primary_formula [ && primary_formula ]*
  Result<Computation<Formula>> conjunctive_formula() {
    Result<Computation<Formula>> alpha = primary_formula();
    if (!alpha) {
      return Error<Computation<Formula>>(LIMBO_MSG("Expected left conjunctive formula"), alpha);
    }
    while (Is(Tok(), Token::kAnd)) {
      Advance();
      Result<Computation<Formula>> beta = primary_formula();
      if (!beta) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected left conjunctive formula"), beta);
      }
      alpha = Success<Computation<Formula>>([alpha_a = std::move(alpha.val), beta_a = std::move(beta.val)]() {
        Result<Formula> alpha = alpha_a.Compute();
        if (!alpha) {
          return Error<Formula>(LIMBO_MSG("Expected left conjunctive formula"), alpha);
        }
        Result<Formula> beta = beta_a.Compute();
        if (!beta) {
          return Error<Formula>(LIMBO_MSG("Expected right conjunctive formula"), beta);
        }
        return Success(Formula::And(std::move(alpha.val), std::move(beta.val)));
      });
    }
    return alpha;
  }

  // disjunctive_formula --> conjunctive_formula [ || conjunctive_formula ]*
  Result<Computation<Formula>> disjunctive_formula() {
    Result<Computation<Formula>> alpha = conjunctive_formula();
    if (!alpha) {
      return Error<Computation<Formula>>(LIMBO_MSG("Expected left argument conjunctive formula"), alpha);
    }
    while (Is(Tok(), Token::kOr)) {
      Advance();
      Result<Computation<Formula>> beta = conjunctive_formula();
      if (!beta) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected right argument conjunctive formula"), beta);
      }
      alpha = Success<Computation<Formula>>([alpha_a = std::move(alpha.val), beta_a = std::move(beta.val)]() {
        Result<Formula> alpha = alpha_a.Compute();
        if (!alpha) {
          return Error<Formula>(LIMBO_MSG("Expected left argument conjunctive formula"), alpha);
        }
        Result<Formula> beta = beta_a.Compute();
        if (!beta) {
          return Error<Formula>(LIMBO_MSG("Expected right argument conjunctive formula"), beta);
        }
        return Success(Formula::Or(std::move(alpha.val), std::move(beta.val)));
      });
    }
    return alpha;
  }

  // implication_formula --> disjunctive_formula -> implication_formula
  //                      |  disjunctive_formula
  Result<Computation<Formula>> implication_formula() {
    Result<Computation<Formula>> alpha = disjunctive_formula();
    if (!alpha) {
      return Error<Computation<Formula>>(LIMBO_MSG("Expected left argument disjunctive formula"), alpha);
    }
    if (Is(Tok(), Token::kRArrow)) {
      Advance();
      Result<Computation<Formula>> beta = implication_formula();
      if (!beta) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected right argument disjunctive formula"), beta);
      }
      alpha = Success<Computation<Formula>>([alpha_a = std::move(alpha.val), beta_a = std::move(beta.val)]() {
        Result<Formula> alpha = alpha_a.Compute();
        if (!alpha) {
          return Error<Formula>(LIMBO_MSG("Expected left argument disjunctive formula"), alpha);
        }
        Result<Formula> beta = beta_a.Compute();
        if (!beta) {
          return Error<Formula>(LIMBO_MSG("Expected right argument disjunctive formula"), beta);
        }
        return Success(Formula::Or(Formula::Not(std::move(alpha.val)), std::move(beta.val)));
      });
    }
    return alpha;
  }

  // equivalence_formula --> implication_formula <-> implication_formula
  //                      |  implication_formula
  Result<Computation<Formula>> equivalence_formula() {
    Result<Computation<Formula>> alpha = implication_formula();
    if (!alpha) {
      return Error<Computation<Formula>>(LIMBO_MSG("Expected left argument implication formula"), alpha);
    }
    if (Is(Tok(), Token::kLRArrow)) {
      Advance();
      Result<Computation<Formula>> beta = implication_formula();
      if (!beta) {
        return Error<Computation<Formula>>(LIMBO_MSG("Expected right argument implication formula"), beta);
      }
      alpha = Success<Computation<Formula>>([alpha_a = std::move(alpha.val), beta_a = std::move(beta.val)]() {
        Result<Formula> alpha = alpha_a.Compute();
        if (!alpha) {
          return Error<Formula>(LIMBO_MSG("Expected left argument implication formula"), alpha);
        }
        Result<Formula> beta = beta_a.Compute();
        if (!beta) {
          return Error<Formula>(LIMBO_MSG("Expected right argument implication formula"), beta);
        }
        Formula alpha2 = alpha.val.Clone();
        Formula beta2  = beta.val.Clone();
        return Success(Formula::And(Formula::Or(Formula::Not(std::move(alpha.val)), std::move(beta.val)),
                                    Formula::Or(std::move(alpha2), Formula::Not(std::move(beta2)))));
      });
    }
    return alpha;
  }

  // formula --> equivalence_formula
  Result<Computation<Formula>> formula() {
    return equivalence_formula();
  }

  // real_literal --> Real : literal
  Result<Computation<>> real_literal() {
    if (!Is(Tok(), Token::kReal)) {
      return Unapplicable<Computation<>>(LIMBO_MSG("Expected 'Real'"));
    }
    return Error<Computation<>>(LIMBO_MSG("Real world currently not implemented"));
    Advance();
    if (!Is(Tok(), Token::kColon)) {
      return Error<Computation<>>(LIMBO_MSG("Expected ':'"));
    }
    Advance();
    Result<Computation<Formula>> a = literal();
    if (!a) {
      return Error<Computation<>>(LIMBO_MSG("Expected real world literal"), a);
    }
    return Success<Computation<>>([a_a = a.val]() {
      Result<Formula> a = a_a.Compute();
      if (!a) {
        return Error<>(LIMBO_MSG("Expected real world literal"), a);
      }
      //if (!a.val.readable().ground() || a.val.unsatisfiable()) {
      //  return Error<>(LIMBO_MSG("Real world literal must be ground and satisfiable"));
      //}
      //io().AddReal(a.val);
      //return Success<>();
      return Error<>(LIMBO_MSG("Real world currently not implemented"), a);
    });
  }

  // kb_formula --> KB : formula
  //             |  KB : [] [ [atomic_term] ] term = atomic_term <-> formula
  Result<Computation<>> kb_formula() {
    if (!Is(Tok(), Token::kKB)) {
      return Unapplicable<Computation<>>(LIMBO_MSG("Expected 'KB'"));
    }
    Advance();
    if (!Is(Tok(), Token::kColon)) {
      return Error<Computation<>>(LIMBO_MSG("Expected ':'"));
    }
    Advance();
    //if (Is(Tok(0), Token::kBox)) {
    //  return Error<Computation<Formula>>(LIMBO_MSG("SSAs currently not implemented"), alpha);
    //  Advance();
    //  // Remainder is:
    //  // [ [atomic_term] ] term = atomic_term <-> formula
    //  const bool ssa = Is(Tok(), Token::kLBracket);
    //  Result<Computation<Formula>> t;
    //  if (ssa) {
    //    Advance();
    //    t = atomic_term();
    //    if (!t) {
    //      return Error<Computation<>>(LIMBO_MSG("Expected action variable"), t);
    //    }
    //    if (!Is(Tok(), Token::kRBracket)) {
    //      return Error<Computation<>>(LIMBO_MSG("Expected ']'"));
    //    }
    //    Advance();
    //  }
    //  Result<Computation<Formula>> a = literal();
    //  if (!a) {
    //    return Error<Computation<>>(LIMBO_MSG("Expected KB dynamic left-hand side literal"), a);
    //  }
    //  if (!Is(Tok(), Token::kLRArrow)) {
    //    return Error<Computation<>>(LIMBO_MSG("Expected '<->'"));
    //  }
    //  Advance();
    //  Result<Computation<Formula>> alpha = formula();
    //  if (!alpha) {
    //    return Error<Computation<>>(LIMBO_MSG("Expected KB dynamic right-hand side formula"), alpha);
    //  }
    //  return Success<Computation<>>([this, ssa, t_a = t.val, a_a = a.val, alpha_a = alpha.val]() {
    //    Result<Formula> t;
    //    if (ssa) {
    //      t = t_a.Compute();
    //      if (!t || !t.val.head().var()) {
    //        return Error<>(LIMBO_MSG("Expected action variable"), t);
    //      }
    //    }
    //    Result<Formula> a = a_a.Compute();
    //    if (!a) {
    //      return Error<>(LIMBO_MSG("Expected KB dynamic left-hand side literal"), a);
    //    }
    //    if (!(a.val.pos() && a.val.lhs().sort() == a.val.rhs().sort() && !a.val.lhs().sort().rigid())) {
    //      return Error<>(LIMBO_MSG("Left-hand side literal of dynamic axiom must be of the form f(x,...) = y "
    //                               "f, y of same, non-rigid sort"));
    //    }
    //    Result<Formula> alpha = alpha_a.Compute();
    //    if (!alpha) {
    //      return Error<>(LIMBO_MSG("Expected KB dynamic right-hand side formula"), alpha);
    //    }
    //    if (!(alpha.val->objective() && !alpha.val->dynamic())) {
    //      return Error<>(LIMBO_MSG("KB dynamic right-hand side formula must not contain modal operators"), alpha);
    //    }
    //    //Formula::SortedTermSet xs;
    //    //a.val.Traverse([&xs](const Term t) { if (t.variable()) { xs.insert(t); } return true; });
    //    //if (ssa) {
    //    //  t.val.Traverse([&xs](const Term t) { if (t.variable()) { xs.insert(t); } return true; });
    //    //}
    //    //for (Term y : alpha.val->free_vars().values()) {
    //    //  if (!xs.contains(y)) {
    //    //    return Error<>(LIMBO_MSG("Free variables in the right-hand side of dynamic axiom must be bound by the "
    //    //                             "left-hand side"));
    //    //  }
    //    //}
    //    //if ((ssa && io().Add(t.val, a.val, alpha.val)) ||
    //    //    (!ssa && io().Add(a.val, alpha.val))) {
    //    //  return Success<>();
    //    //} else {
    //    //  return Error<>(LIMBO_MSG("Couldn't add formula to KB; is it proper+ "
    //    //                          "(i.e., its NF must be a universally quantified clause)?"));
    //    //}
    //  });
    //} else {
      // Remainder is:
      // formula
      Result<Computation<Formula>> alpha = formula();
      if (!alpha) {
        return Error<Computation<>>(LIMBO_MSG("Expected KB formula"), alpha);
      }
      return Success<Computation<>>([this, alpha_a = alpha.val]() {
        Result<Formula> alpha = alpha_a.Compute();
        if (!alpha) {
          return Error<>(LIMBO_MSG("Expected KB formula"), alpha);
        }
        if (eh_.Add(alpha.val)) {
          return Success<>();
        } else {
          return Error<>(LIMBO_MSG("Couldn't add formula to KB; is it proper+ "
                                  "(i.e., its NF must be a universally quantified clause)?"));
        }
      });
    //}
  }

  // subjective_formula --> formula
  Result<Computation<Formula>> subjective_formula() {
    Result<Computation<Formula>> alpha = formula();
    if (!alpha) {
      return Error<Computation<Formula>>(LIMBO_MSG("Expected subjective formula"), alpha);
    }
    return Success<Computation<Formula>>([this, alpha_a = alpha.val]() {
      Result<Formula> alpha = alpha_a.Compute();
      if (!alpha) {
        return Error<Formula>(LIMBO_MSG("Expected subjective formula"), alpha);
      }
      if (!alpha.val.readable().subjective()) {
        return Error<Formula>(LIMBO_MSG("Expected subjective formula "
                                        "(i.e., no functions outside of modal operators; "
                                        "probably caused by missing brackets)"));
      }
      return Success<Formula>(std::move(alpha.val));
    });
  }

  // query --> [ Query | Refute | Assert ] : subjective_formula
  Result<Computation<>> query() {
    if (!(Is(Tok(), Token::kQuery) || Is(Tok(), Token::kNot) || Is(Tok(), Token::kForall) ||
          Is(Tok(), Token::kExists) || Is(Tok(), Token::kLParen) || Is(Tok(), Token::kKnow) ||
          Is(Tok(), Token::kMaybe) || Is(Tok(), Token::kBelieve) || Is(Tok(), Token::kGuarantee) ||
          Is(Tok(), Token::kRegress) || Is(Tok(), Token::kIdentifier)) &&
        !Is(Tok(), Token::kAssert) &&
        !Is(Tok(), Token::kRefute)) {
      return Unapplicable<Computation<>>(LIMBO_MSG("Expected 'Query', 'Assert', or 'Refute'"));
    }
    const bool is_query = !Is(Tok(), Token::kAssert) && !Is(Tok(), Token::kRefute);
    const bool is_assert = Is(Tok(), Token::kAssert);
    if (Is(Tok(), Token::kQuery) || Is(Tok(), Token::kAssert) || Is(Tok(), Token::kRefute)) {
      Advance();
      if (!Is(Tok(), Token::kColon)) {
        return Error<Computation<>>(LIMBO_MSG("Expected ':'"));
      }
      Advance();
    }
    Result<Computation<Formula>> alpha = subjective_formula();
    if (!alpha) {
      return Error<Computation<>>(LIMBO_MSG("Expected query/assertion/refutation subjective_formula"), alpha);
    }
    return Success<Computation<>>([this, alpha_a = alpha.val, is_query, is_assert]() {
      Result<Formula> alpha = alpha_a.Compute();
      if (!alpha) {
        return Error<>(LIMBO_MSG("Expected query/assertion/refutation subjective_formula"), alpha);
      }
      const bool r = eh_.Query(alpha.val);
      if (is_query) {
        return Success<>();
      } else if (r == is_assert) {
        return Success<>();
      } else {
        std::stringstream ss;
        using limbo::io::operator<<;
        ss << (is_assert ? "Assertion" : "Refutation") << " of " << alpha.val << " failed";
        return Error<>(LIMBO_MSG(ss.str()));
      }
    });
  }

  typedef std::pair<std::string, std::list<Formula>> IdTerms;

  // bind_meta_variables --> [ identifier [ in term [, term]* ] -> sort-id ]?
  Result<Computation<IdTerms>> bind_meta_variables() {
    if (!Is(Tok(0), Token::kIdentifier) ||
        !(Is(Tok(1), Token::kIn) || Is(Tok(1), Token::kRArrow))) {
      return Success<Computation<IdTerms>>([]() {
        return Success<IdTerms>();
      });
    }
    const std::string id = Tok().val.str();
    Advance();
    std::list<Computation<Formula>> ts;
    if (Is(Tok(), Token::kIn)) {
      do {
        Advance();
        Result<Computation<Formula>> t = term();
        if (!t) {
          return Error<Computation<IdTerms>>(LIMBO_MSG("Expected argument term"), t);
        }
        ts.push_back(std::move(t.val));
      } while (Is(Tok(), Token::kComma));
    }
    if (!Is(Tok(), Token::kRArrow)) {
      return Error<Computation<IdTerms>>(LIMBO_MSG("Expected right arrow '->'"));
    }
    Advance();
    if (!Is(Tok(), Token::kIdentifier)) {
      return Error<Computation<IdTerms>>(LIMBO_MSG("Expected sort identifier"));
    }
    const std::string sort = Tok().val.str();
    Advance();
    return Success<Computation<IdTerms>>([this, id, sort_id = sort, ts_a = ts]() {
      if (!default_sort_string(sort_id) || !sort_registry().Registered(sort_id)) {
        return Error<IdTerms>(LIMBO_MSG("Sort "+ sort_id +" is not registered"));
      }
      const Abc::Sort sort = sort_registry().ToSymbol(sort_id, false);
      std::list<Formula> ts;
      //if (ts_a.empty()) {
      //  const KnowledgeBase::TermSet& ns = io().kb().mentioned_names(sort);
      //  ts.insert(ts.end(), ns.begin(), ns.end());
      //} else {
        for (const Computation<Formula>& t_a : ts_a) {
          Result<Formula> t = t_a.Compute();
          if (!t) {
            return Error<IdTerms>(LIMBO_MSG("Expected term in range"), t);
          }
          if (t.val.head().sort() != sort) {
            return Error<IdTerms>(LIMBO_MSG("Term in range is not of sort "+ sort_id));
          }
          ts.push_back(std::move(t.val));
        }
      //}
      return Success<IdTerms>(std::make_pair(id, std::move(ts)));
    });
  }

  // if_else --> If formula block [ Else block ]
  Result<Computation<>> if_else() {
    if (!Is(Tok(), Token::kIf)) {
      return Unapplicable<Computation<>>(LIMBO_MSG("Expected 'If'"));
    }
    Advance();
    Result<Computation<IdTerms>> bind = bind_meta_variables();
    if (!bind) {
      return Error<Computation<>>(LIMBO_MSG("Expected bind_meta_variables"), bind);
    }
    Result<Computation<Formula>> alpha = formula();
    if (!alpha) {
      return Error<Computation<>>(LIMBO_MSG("Expected formula in if_else"), alpha);
    }
    Result<Computation<>> if_block = block();
    if (!if_block) {
      return Error<Computation<>>(LIMBO_MSG("Expected if block in if_else"), if_block);
    }
    Result<Computation<>> else_block;
    if (Is(Tok(), Token::kElse)) {
      Advance();
      else_block = block();
      if (!else_block) {
        return Error<Computation<>>(LIMBO_MSG("Expected else block in if_else"), else_block);
      }
    } else {
      else_block = Success<Computation<>>([]() { return Success<>(); });
    }
    return Success<Computation<>>([this, bind_a = bind.val, alpha_a = alpha.val, if_block_a = if_block.val,
                              else_block_a = else_block.val]() {
      Result<IdTerms> bind = bind_a.Compute();
      const std::string id = bind.val.first;
      bool cond;
      if (!id.empty()) {
        return Error<>(LIMBO_MSG("Meta variables currently not implemented"));
        cond = false;
        for (Formula& t : bind.val.second) {
          //io().RegisterMetaVariable(id, t);
          Result<Formula> alpha = alpha_a.Compute();
          if (!alpha) {
            return Error<>(LIMBO_MSG("Expected condition if_else"), alpha);
          }
          if (eh_.Query(alpha.val)) {
            cond = true;
            break;
          }
          //io().UnregisterMetaVariable(id);
        }
      } else {
        Result<Formula> alpha = alpha_a.Compute();
        if (!alpha) {
          return Error<>(LIMBO_MSG("Expected condition if_else"), alpha);
        }
        cond = eh_.Query(alpha.val);
      }
      Result<> r;
      if (cond) {
        r = if_block_a.Compute();
        if (!id.empty()) {
          return Error<>(LIMBO_MSG("Meta variables currently not implemented"));
          //io().UnregisterMetaVariable(id);
        }
      } else {
        r = else_block_a.Compute();
      }
      if (!r) {
        return Error<>(LIMBO_MSG("Expected block in if_else"), r);
      }
      return r;
    });
  }

  // while_loop --> While formula block [ Else block ]
  Result<Computation<>> while_loop() {
    if (!Is(Tok(), Token::kWhile)) {
      return Unapplicable<Computation<>>(LIMBO_MSG("Expected 'While'"));
    }
    Advance();
    Result<Computation<IdTerms>> bind = bind_meta_variables();
    if (!bind) {
      return Error<Computation<>>(LIMBO_MSG("Expected bind_meta_variables"), bind);
    }
    Result<Computation<Formula>> alpha = formula();
    if (!alpha) {
      return Error<Computation<>>(LIMBO_MSG("Expected formula in while_loop"), alpha);
    }
    Result<Computation<>> while_block = block();
    if (!while_block) {
      return Error<Computation<>>(LIMBO_MSG("Expected if block in while_else"), while_block);
    }
    Result<Computation<>> else_block;
    if (Is(Tok(), Token::kElse)) {
      Advance();
      else_block = block();
      if (!else_block) {
        return Error<Computation<>>(LIMBO_MSG("Expected else block in while_loop"), else_block);
      }
    } else {
      else_block = Success<Computation<>>([]() { return Success<>(); });
    }
    return Success<Computation<>>([this, bind_a = bind.val, alpha_a = alpha.val, while_block_a = while_block.val,
                                   else_block_a = else_block.val]() {
      Result<IdTerms> bind = bind_a.Compute();
      const std::string id = bind.val.first;
      bool once = false;
      for (;;) {
        bool cond;
        if (!id.empty()) {
          cond = false;
          for (Formula& t : bind.val.second) {
            return Error<>(LIMBO_MSG("Meta variables currently not implemented"));
            //io().RegisterMetaVariable(id, t);
            Result<Formula> alpha = alpha_a.Compute();
            if (!alpha) {
              return Error<>(LIMBO_MSG("Expected condition while_loop"), alpha);
            }
            if (eh_.Query(alpha.val)) {
              cond = true;
              break;
            }
            //io().UnregisterMetaVariable(id);
          }
        } else {
          Result<Formula> alpha = alpha_a.Compute();
          if (!alpha) {
            return Error<>(LIMBO_MSG("Expected condition while_loop"), alpha);
          }
          cond = eh_.Query(alpha.val);
        }
        if (cond) {
          once = true;
          Result<> r = while_block_a.Compute();
          if (!id.empty()) {
            return Error<>(LIMBO_MSG("Meta variables currently not implemented"));
            //io().UnregisterMetaVariable(id);
          }
          if (!r) {
            return Error<>(LIMBO_MSG("Expected block in while_loop"), r);
          }
        } else {
          break;
        }
      }
      if (!once) {
        Result<> r = else_block_a.Compute();
        if (!r) {
          return Error<>(LIMBO_MSG("Expected block in while_loop"), r);
        }
      }
      return Success<>();
    });
  }

  // for_loop --> For formula block [ Else block ]
  Result<Computation<>> for_loop() {
    if (!Is(Tok(), Token::kFor)) {
      return Unapplicable<Computation<>>(LIMBO_MSG("Expected 'For'"));
    }
    Advance();
    Result<Computation<IdTerms>> bind = bind_meta_variables();
    if (!bind) {
      return Error<Computation<>>(LIMBO_MSG("Expected bind_meta_variables"), bind);
    }
    Result<Computation<Formula>> alpha = formula();
    if (!alpha) {
      return Error<Computation<>>(LIMBO_MSG("Expected formula in for_loop"), alpha);
    }
    Result<Computation<>> for_block = block();
    if (!for_block) {
      return Error<Computation<>>(LIMBO_MSG("Expected if block in for_else"), for_block);
    }
    Result<Computation<>> else_block;
    if (Is(Tok(), Token::kElse)) {
      Advance();
      else_block = block();
      if (!else_block) {
        return Error<Computation<>>(LIMBO_MSG("Expected else block in for_loop"), else_block);
      }
    } else {
      else_block = Success<Computation<>>([]() { return Success<>(); });
    }
    return Success<Computation<>>([this, bind_a = bind.val, alpha_a = alpha.val, for_block_a = for_block.val,
                             else_block_a = else_block.val]() {
      Result<IdTerms> bind = bind_a.Compute();
      const std::string id = bind.val.first;
      if (id.empty()) {
        return Error<>(LIMBO_MSG("Expected meta variable id"));
      }
      bool once = false;
      for (Formula& t : bind.val.second) {
        return Error<>(LIMBO_MSG("Meta variables currently not implemented"));
        //io().RegisterMetaVariable(id, t);
        Result<Formula> alpha = alpha_a.Compute();
        if (!alpha) {
          return Error<>(LIMBO_MSG("Expected condition for_loop"), alpha);
        }
        if (eh_.Query(alpha.val)) {
          once = true;
          Result<> r = for_block_a.Compute();
          if (!r) {
            //io().UnregisterMetaVariable(id);
            return Error<>(LIMBO_MSG("Expected block in for_loop"), r);
          }
        }
        //io().UnregisterMetaVariable(id);
      }
      if (!once) {
        Result<> r = else_block_a.Compute();
        if (!r) {
          return Error<>(LIMBO_MSG("Expected block in for_loop"), r);
        }
      }
      return Success<>();
    });
  }

  // abbreviation --> let identifier := formula
  Result<Computation<>> abbreviation() {
    if (!Is(Tok(), Token::kLet)) {
      return Unapplicable<Computation<>>(LIMBO_MSG("Expected abbreviation operator 'let'"));
    }
    return Error<Computation<>>(LIMBO_MSG("Formula abbreviations currently not implemented"));
    Advance();
    if (!Is(Tok(), Token::kIdentifier)) {
      return Error<Computation<>>(LIMBO_MSG("Expected fresh identifier"));
    }
    const std::string id = Tok().val.str();
    Advance();
    if (!Is(Tok(), Token::kAssign)) {
      return Error<Computation<>>(LIMBO_MSG("Expected assignment operator ':='"));
    }
    Advance();
    Result<Computation<Formula>> alpha = formula();
    if (!alpha) {
      return Error<Computation<>>(LIMBO_MSG("Expected formula"), alpha);
    }
    return Success<Computation<>>([id, alpha_a = alpha.val]() {
      Result<Formula> alpha = alpha_a.Compute();
      if (!alpha) {
        return Error<>(LIMBO_MSG("Expected formula"), alpha);
      }
      //io().RegisterFormula(id, alpha.val);
      return Success<>();
    });
  }

  // call --> Call id
  Result<Computation<>> call() {
    if (!Is(Tok(), Token::kCall)) {
      return Unapplicable<Computation<>>(LIMBO_MSG("Expected 'Call'"));
    }
    Advance();
    if (!Is(Tok(), Token::kColon)) {
      return Error<Computation<>>(LIMBO_MSG("Expected ':'"));
    }
    Advance();
    if (!Is(Tok(), Token::kIdentifier)) {
      return Error<Computation<>>(LIMBO_MSG("Expected meta variable in for_loop"));
    }
    const std::string id = Tok().val.str();
    Advance();
    if (!Is(Tok(), Token::kLParen)) {
      return Error<Computation<>>(LIMBO_MSG("Expected opening parentheses '('"));
    }
    std::list<Computation<Formula>> ts;
    do {
      Advance();
      if (Is(Tok(), Token::kRParen)) {
        break;
      }
      Result<Computation<Formula>> t = term();
      if (!t) {
        return Error<Computation<>>(LIMBO_MSG("Expected argument"), t);
      }
      ts.push_back(std::move(t.val));
    } while (Is(Tok(), Token::kComma));
    if (!Is(Tok(), Token::kRParen)) {
      return Error<Computation<>>(LIMBO_MSG("Expected closing parentheses '('"));
    }
    Advance();
    return Success<Computation<>>([this, id, ts_as = ts]() {
      return Error<>(LIMBO_MSG("Procedure calls currently not implemented"));
      std::list<Formula> ts;
      for (const Computation<Formula>& arg_a : ts_as) {
        Result<Formula> t = arg_a.Compute();
        ts.push_back(std::move(t.val));
      }
      //io().Call(id, ts);
      return Success<>();
    });
  }

  // block --> Begin branch* End
  Result<Computation<>> block() {
    if (!Is(Tok(), Token::kBegin)) {
      Result<Computation<>> r = branch();
      if (!r) {
        return Error<Computation<>>(LIMBO_MSG("Expected branch in block"), r);
      }
      return r;
    } else {
      Advance();
      const size_t n_blocks = n_blocks_;
      ++n_blocks_;
      Computation<> a;
      while (n_blocks_ > n_blocks) {
        if (Is(Tok(), Token::kEnd)) {
          Advance();
          --n_blocks_;
        } else {
          Result<Computation<>> r = branch();
          if (!r) {
            return Error<Computation<>>(LIMBO_MSG("Expected branch in block"), r);
          }
          a += std::move(r.val);
        }
      }
      return Success<Computation<>>(std::move(a));
    }
  }

  // branch --> [ declaration | real_literal | kb_formula | abbreviation | query | if_else | while_loop
  //            | for_loop | call ]
  Result<Computation<>> branch() {
    typedef Result<Computation<>> (Parser::*Rule)();
    auto rules = {&Parser::declaration, &Parser::real_literal, &Parser::kb_formula, &Parser::abbreviation,
                  &Parser::query, &Parser::if_else, &Parser::while_loop, &Parser::for_loop, &Parser::call};
    for (Rule rule : rules) {
      Result<Computation<>> r = (this->*rule)();
      if (r) {
        return r;
      } else if (r.applied()) {
        return Error<Computation<>>(LIMBO_MSG("Error in branch"), r);
      }
    }
    return Unapplicable<Computation<>>(LIMBO_MSG("No rule applicable in branch"));
  }

  // start --> branch
  Result<Computation<>> start() {
    Computation<> a = []() { return Success<>(); };
    while (Tok()) {
      Result<Computation<>> r = branch();
      if (!r) {
        std::stringstream ss;
        using limbo::io::operator<<;
        ss << Tok(0) << " " << Tok(1) << " " << Tok(2) << "...";
        return Error<Computation<>>(LIMBO_MSG("Error in start with unparsed input "+ ss.str()), r);
      }
      a += std::move(r.val);
    }
    return Success<Computation<>>(std::move(a));
  }

  bool Is(const limbo::internal::Maybe<Token>& symbol, Token::Id id) const {
    return symbol && symbol.val.id() == id;
  }

  template<typename UnaryPredicate>
  bool Is(const limbo::internal::Maybe<Token>& symbol, Token::Id id, UnaryPredicate p) const {
    return symbol && symbol.val.id() == id && p(symbol.val.str());
  }

  limbo::internal::Maybe<Token> Tok(size_t n = 0) const {
    auto it = begin();
    for (size_t i = 0; i < n && it != end_; ++i) {
      assert(begin() != end());
      ++it;
    }
    return it != end_ ? limbo::internal::Just(*it) : limbo::internal::Nothing;
  }

  void Advance(size_t n = 1) {
    begin_plus_ += n;
  }

  TokenIterator begin() const {
    while (begin_plus_ > 0) {
      assert(begin_ != end_);
      ++begin_;
      begin_plus_--;
    }
    return begin_;
  }

  TokenIterator end() const {
    return end_;
  }

  static auto& abc()           { return Abc::instance(); }
  static auto& io()            { return IoContext::instance(); }
  static auto& sort_registry() { return io().sort_registry(); }
  static auto& fun_registry()  { return io().fun_registry(); }
  static auto& name_registry() { return io().name_registry(); }
  static auto& var_registry()  { return io().var_registry(); }
  static auto& meta_registry() { return io().meta_registry(); }

  static bool Registered(const std::string& s) {
    return sort_registry().Registered(s) ||
           fun_registry().Registered(s) ||
           name_registry().Registered(s) ||
           var_registry().Registered(s) ||
           meta_registry().Registered(s);
  }

  bool default_sort_string(const std::string& s) const {
    return default_if_undeclared_ && !Registered(s) && s.length() > 0 &&
        s[0] == 's';
  }

  bool default_fun_string(const std::string& s) const {
    return default_if_undeclared_ && !Registered(s) && s.length() > 0 &&
        (s[0] == 'f' || s[0] == 'g' || s[0] == 'h');
  }

  bool default_name_string(const std::string& s) const {
    return default_if_undeclared_ && !Registered(s) && s.length() > 0 &&
        (s[0] == 'n' || s[0] == 'o');
  }

  bool default_var_string(const std::string& s) const {
    return default_if_undeclared_ && !Registered(s) && s.length() > 0 &&
        (s[0] == 'a' || s[0] == 'x' || s[0] == 'y' || s[0] == 'z');
  }

  bool default_meta_string(const std::string& s) const {
    return default_if_undeclared_ && !Registered(s) && s.length() > 0 &&
        s[0] == 'm';
  }

  static const char* kUnapplicableLabel;
  static const char* kErrorLabel;
  static const char* kCausesLabel;

  EventHandler   eh_;
  bool           default_if_undeclared_ = false;
  Lexer          lexer_;
  mutable        TokenIterator begin_;  // don't use begin_ directly: to avoid the stream blocking us, Advance() actually increments
  mutable size_t begin_plus_ = 0;  // begin_plus_ instead of begin_; use begin() to obtain the incremented iterator.
  TokenIterator  end_;
  size_t         n_blocks_ = 0;
};


template<typename ForwardIt, typename EventHandler>
const char* Parser<ForwardIt, EventHandler>::kUnapplicableLabel = "Unappl.: ";

template<typename ForwardIt, typename EventHandler>
const char* Parser<ForwardIt, EventHandler>::kErrorLabel        = "Failure: ";

template<typename ForwardIt, typename EventHandler>
const char* Parser<ForwardIt, EventHandler>::kCausesLabel       = " causes: ";

}  // namespace io
}  // namespace limbo

#endif  // LIMBO_IO_PARSER_H_

