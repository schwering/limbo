// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016-2017 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// Recursive descent parser for the problem description language. The grammar
// for formulas is aims to reduce brackets and implement operator precedence.
// See the comment above Parser::start() and its callees for the grammar
// definition. The Context template parameter is merely passed around to be
// the argument of Parser::Action functors, as returned by Parser::Parse().

#ifndef LIMBO_FORMAT_PDL_PARSER_H_
#define LIMBO_FORMAT_PDL_PARSER_H_

#include <cassert>

#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <limbo/formula.h>
#include <limbo/grounder.h>
#include <limbo/setup.h>

#include <limbo/format/output.h>

#include <limbo/format/pdl/lexer.h>

namespace limbo {
namespace format {
namespace pdl {

#define LIMBO_S(x)          #x
#define LIMBO_S_(x)         LIMBO_S(x)
#define LIMBO_STR__LINE__   LIMBO_S_(__LINE__)
#define LIMBO_MSG(msg)      (std::string(msg) +" (in rule "+ __FUNCTION__ +":"+ LIMBO_STR__LINE__ +")")

template<typename ForwardIt, typename Context>
class Parser {
 public:
  typedef Lexer<ForwardIt> Lex;
  typedef typename Lex::iterator iterator;

  struct Void {
    friend std::ostream& operator<<(std::ostream& os, Void) { return os; }
  };

  // Encapsulates a parsing result, either a Success, an Unapplicable, or a Error.
  template<typename T = Void>
  struct Result {
    typedef T type;
    enum Type { kSuccess, kUnapplicable, kError };

    Result() = default;
    Result(Result&&) = default;
    Result& operator=(Result&&) = default;

    explicit Result(T&& val) : val(std::forward<T>(val)), type_(kSuccess) {}

    Result(Type type, const std::string& msg, ForwardIt begin = ForwardIt(), ForwardIt end = ForwardIt(), T&& val = T())
        : val(std::forward<T>(val)), type_(type), msg_(msg), begin_(begin), end_(end) {}

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
  class Action : public std::shared_ptr<std::function<Result<T>(Context*)>> {
   public:
    typedef std::function<Result<T>(Context*)> function;
    typedef std::shared_ptr<function> base;

    Action() = default;

    template<typename NullaryFunction>
    Action(NullaryFunction func) : base::shared_ptr(new function(func)) {}

    Result<T> Run(Context* ctx) const {
      function* f = base::get();
      if (f) {
        return (*f)(ctx);
      } else {
        return Result<T>(Result<T>::kError, LIMBO_MSG("Action is null"));
      }
    }

    friend Action operator+(Action a, Action b) {
      if (!a) {
        return b;
      }
      if (!b) {
        return a;
      }
      return [a, b](Context* ctx) {
        Result<> r = (*a)(ctx);
        if (!r) {
          return r;
        }
        return (*b)(ctx);
      };
    }

    Action operator+=(Action a) {
      *this = *this + a;
      return *this;
    }
  };

  Parser(ForwardIt begin, ForwardIt end) : lexer_(begin, end), begin_(lexer_.begin()), end_(lexer_.end()) {}

  Result<Action<>> Parse() { return start(); }

 private:
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
  Result<Action<>> declaration() {
    if ((Is(Tok(), Token::kRigid) && Is(Tok(1), Token::kSort)) || Is(Tok(), Token::kSort)) {
      Action<> a;
      bool rigid = Is(Tok(), Token::kRigid);
      if (rigid) {
        Advance();
      }
      do {
        Advance();
        if (Is(Tok(), Token::kIdentifier)) {
          const std::string id = Tok().val.str();
          Advance();
          a += [this, rigid, id](Context* ctx) {
            if (!ctx->IsRegisteredSort(id)) {
              ctx->RegisterSort(id, rigid);
              return Success<>();
            } else {
              return Error<>(LIMBO_MSG("Sort "+ id +" is already registered"));
            }
          };
        } else {
          return Error<Action<>>(LIMBO_MSG("Expected sort identifier"));
        }
      } while (Is(Tok(), Token::kComma));
      return Success<Action<>>(std::move(a));
    }
    if (Is(Tok(), Token::kVar) || Is(Tok(), Token::kName)) {
      const bool var = Is(Tok(), Token::kVar);
      std::vector<std::string> ids;
      do {
        Advance();
        if (Is(Tok(), Token::kIdentifier)) {
          ids.push_back(Tok().val.str());
          Advance();
        } else {
          return Error<Action<>>(LIMBO_MSG(var ? "Expected variable identifier" : "Expected name identifier"));
        }
      } while (Is(Tok(0), Token::kComma));
      if (Is(Tok(0), Token::kRArrow) &&
          Is(Tok(1), Token::kIdentifier)) {
        const std::string sort = Tok(1).val.str();
        Advance(2);
        Action<> a;
        for (const std::string& id : ids) {
          a += [this, var, sort, id](Context* ctx) {
            if (ctx->IsRegisteredSort(sort)) {
              if (!ctx->IsRegisteredTerm(id)) {
                var ? ctx->RegisterVariable(id, sort) : ctx->RegisterName(id, sort);
                return Success<>();
              } else {
                return Error<>(LIMBO_MSG("Term "+ id +" is already registered"));
              }
            } else {
              return Error<>(LIMBO_MSG("Sort "+ sort +" is not registered"));
            }
          };
        }
        return Success<Action<>>(std::move(a));
      } else {
        return Error<Action<>>(LIMBO_MSG("Expected arrow and sort identifier"));
      }
    }
    if ((Is(Tok(), Token::kSensor) && Is(Tok(1), Token::kFun)) || Is(Tok(), Token::kFun)) {
      struct Decl {
        std::string fun_id;
        Symbol::Arity arity;
        std::string sensor_id;
      };
      std::vector<Decl> ids;
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
          return Error<Action<>>(LIMBO_MSG("Expected function identifier"));
        }
      } while (Is(Tok(0), Token::kComma));
      if (Is(Tok(0), Token::kRArrow) && Is(Tok(1), Token::kIdentifier)) {
        const std::string sort_id = Tok(1).val.str();
        Advance(2);
        Action<> a;
        for (const Decl& d : ids) {
          a += [this, sensor, sort_id, d](Context* ctx) {
            if (!ctx->IsRegisteredSort(sort_id)) {
              return Error<>(LIMBO_MSG("Sort "+ sort_id +" is not registered"));
            }
            if (ctx->IsRegisteredTerm(d.fun_id)) {
              return Error<>(LIMBO_MSG("Term "+ d.fun_id +" is already registered"));
            }
            if (sensor && !ctx->IsRegisteredSort(d.sensor_id)) {
              return Error<>(LIMBO_MSG("Sensor sort "+ d.sensor_id +" is not registered"));
            }
            if (!sensor) {
              ctx->RegisterFunction(d.fun_id, d.arity, sort_id);
            } else {
              ctx->RegisterSensorFunction(d.fun_id, sort_id, d.sensor_id);
            }
            return Success<>();
          };
        }
        return Success<Action<>>(std::move(a));
      } else {
        return Error<Action<>>(LIMBO_MSG("Expected arrow and sort identifier"));
      }
    }
    return Unapplicable<Action<>>(LIMBO_MSG("Expected 'Rigid', 'Sort', 'Var', 'Name', 'Fun' or 'Sensor'"));
  }

  // atomic_term --> x
  //              |  n
  //              |  f
  Result<Action<Term>> atomic_term() {
    if (Is(Tok(), Token::kIdentifier)) {
      const std::string id = Tok().val.str();
      Advance();
      return Success<Action<Term>>([this, id](Context* ctx) {
        if (ctx->IsRegisteredVariable(id)) {
          return Success<Term>(ctx->LookupVariable(id));
        } else if (ctx->IsRegisteredName(id)) {
          return Success<Term>(ctx->LookupName(id));
        } else if (ctx->IsRegisteredFunction(id)) {
          Symbol f = ctx->LookupFunction(id);
          if (f.arity() != 0) {
            return Error<Term>(LIMBO_MSG("Wrong number of arguments for "+ id));
          }
          return Success(ctx->CreateTerm(f, {}));
        } else if (ctx->IsRegisteredMetaVariable(id)) {
          return Success(ctx->LookupMetaVariable(id));
        } else {
          return Error<Term>(LIMBO_MSG("Error in atomic_term"));
        }
      });
    }
    return Error<Action<Term>>(LIMBO_MSG("Expected a declared variable/name/function identifier"));
  }

  // term --> x
  //       |  n
  //       |  f
  //       |  f(term, ..., term)
  Result<Action<Term>> term() {
    if (Is(Tok(), Token::kIdentifier)) {
      const std::string id = Tok().val.str();
      Advance();
      std::vector<Action<Term>> args;
      if (Is(Tok(), Token::kLParen)) {
        Advance();
        for (;;) {
          Result<Action<Term>> t = term();
          if (!t) {
            return Error<Action<Term>>(LIMBO_MSG("Expected argument term"), t);
          }
          args.push_back(t.val);
          if (Is(Tok(), Token::kComma)) {
            Advance();
            continue;
          } else if (Is(Tok(), Token::kRParen)) {
            Advance();
            break;
          } else {
            return Error<Action<Term>>(LIMBO_MSG("Expected comma ',' or closing parenthesis ')'"));
          }
        }
      }
      return Success<Action<Term>>([this, id, args_a = args](Context* ctx) {
        if (ctx->IsRegisteredVariable(id)) {
          return Success<Term>(ctx->LookupVariable(id));
        } else if (ctx->IsRegisteredName(id)) {
          return Success<Term>(ctx->LookupName(id));
        } else if (ctx->IsRegisteredFunction(id)) {
          Symbol f = ctx->LookupFunction(id);
          if (static_cast<size_t>(f.arity()) != args_a.size()) {
            return Error<Term>(LIMBO_MSG("Wrong number of arguments for "+ id));
          }
          Term::Vector args;
          for (const Action<Term>& a : args_a) {
            Result<Term> t = a.Run(ctx);
            if (t) {
              args.push_back(t.val);
            } else {
              return Error<Term>(LIMBO_MSG("Expected argument term"), t);
            }
          }
          return Success(ctx->CreateTerm(f, args));
        } else if (ctx->IsRegisteredMetaVariable(id)) {
          return Success(ctx->LookupMetaVariable(id));
        } else {
          return Error<Term>(LIMBO_MSG("Error in term"));
        }
      });
    }
    return Error<Action<Term>>(LIMBO_MSG("Expected a declared variable/name/function identifier"));
  }

  // literal --> term [ '==' | '!=' ] term
  Result<Action<Literal>> literal() {
    Result<Action<Term>> lhs = term();
    if (!lhs) {
      return Error<Action<Literal>>(LIMBO_MSG("Expected a lhs term"), lhs);
    }
    bool pos;
    if (Is(Tok(), Token::kEquality) || Is(Tok(), Token::kInequality)) {
      pos = Is(Tok(), Token::kEquality);
      Advance();
    } else {
      return Error<Action<Literal>>(LIMBO_MSG("Expected equality or inequality '=='/'!='"));
    }
    Result<Action<Term>> rhs = term();
    if (!rhs) {
      return Error<Action<Literal>>(LIMBO_MSG("Expected rhs term"), rhs);
    }
    return Success<Action<Literal>>([lhs_a = lhs.val, pos, rhs_a = rhs.val](Context* ctx) {
      Result<Term> lhs = lhs_a.Run(ctx);
      if (!lhs) {
        return Error<Literal>(LIMBO_MSG("Expected a lhs term"), lhs);
      }
      Result<Term> rhs = rhs_a.Run(ctx);
      if (!rhs) {
        return Error<Literal>(LIMBO_MSG("Expected a rhs term"), rhs);
      }
      Literal a = pos ? Literal::Eq(lhs.val, rhs.val) : Literal::Neq(lhs.val, rhs.val);
      return Success(Literal(a));
    });
  }

  // primary_formula --> ! primary_formula
  //                  |  Ex atomic_term primary_formula
  //                  |  Fa atomic_term primary_formula
  //                  |  Know < k > primary_formula
  //                  |  Cons < k > primary_formula
  //                  |  Bel < k , l > primary_formula => primary_formula
  //                  |  Guarantee primary_formula
  //                  |  [ term ] primary_formula
  //                  |  REG primary_formula
  //                  |  ( formula )
  //                  |  abbreviation
  //                  |  literal
  Result<Action<Formula::Ref>> primary_formula() {
    if (Is(Tok(), Token::kNot)) {
      Advance();
      Result<Action<Formula::Ref>> alpha = primary_formula();
      if (!alpha) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected a primary formula within negation"), alpha);
      }
      return Success<Action<Formula::Ref>>([this, alpha_a = alpha.val](Context* ctx) {
        Result<Formula::Ref> alpha = alpha_a.Run(ctx);
        if (!alpha) {
          return Error<Formula::Ref>(LIMBO_MSG("Expected a primary formula within negation"), alpha);
        }
        return Success<Formula::Ref>(Formula::Factory::Not(std::move(alpha.val)));
      });
    }
    if (Is(Tok(), Token::kExists) || Is(Tok(), Token::kForall)) {
      bool ex = Is(Tok(), Token::kExists);
      Advance();
      Result<Action<Term>> x = atomic_term();
      if (!x) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected variable in quantifier"), x);
      }
      Result<Action<Formula::Ref>> alpha = primary_formula();
      if (!alpha) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected primary formula within quantifier"), alpha);
      }
      return Success<Action<Formula::Ref>>([this, ex, x_a = x.val, alpha_a = alpha.val](Context* ctx) {
        Result<Term> x = x_a.Run(ctx);
        if (!x || !x.val.variable()) {
          return Error<Formula::Ref>(LIMBO_MSG("Expected variable in quantifier"), x);
        }
        Result<Formula::Ref> alpha = alpha_a.Run(ctx);
        if (!alpha) {
          return Error<Formula::Ref>(LIMBO_MSG("Expected primary formula within quantifier"), alpha);
        }
        return Success(ex ? Formula::Factory::Exists(x.val, std::move(alpha.val))
                          : Formula::Factory::Forall(x.val, std::move(alpha.val)));
      });
    }
    if (Is(Tok(), Token::kKnow) || Is(Tok(), Token::kCons)) {
      bool know = Is(Tok(), Token::kKnow);
      Advance();
      if (!Is(Tok(), Token::kLess)) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected '<'"));
      }
      Advance();
      if (!Is(Tok(), Token::kUint)) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected split level integer"));
      }
      const Formula::belief_level k = std::stoi(Tok().val.str());
      Advance();
      if (!Is(Tok(), Token::kGreater)) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected '>'"));
      }
      Advance();
      Result<Action<Formula::Ref>> alpha = primary_formula();
      if (!alpha) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected primary formula within modality"), alpha);
      }
      return Success<Action<Formula::Ref>>([this, know, k, alpha_a = alpha.val](Context* ctx) {
        Result<Formula::Ref> alpha = alpha_a.Run(ctx);
        if (!alpha) {
          return Error<Formula::Ref>(LIMBO_MSG("Expected primary formula within modality"), alpha);
        }
        if (know) {
          return Success(Formula::Factory::Know(k, std::move(alpha.val)));
        } else {
          return Success(Formula::Factory::Cons(k, std::move(alpha.val)));
        }
      });
    }
    if (Is(Tok(), Token::kBel)) {
      Advance();
      if (!Is(Tok(), Token::kLess)) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected '<'"));
      }
      Advance();
      if (!Is(Tok(), Token::kUint)) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected first split level integer"));
      }
      const Formula::belief_level k = std::stoi(Tok().val.str());
      Advance();
      if (!Is(Tok(), Token::kComma)) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected ','"));
      }
      Advance();
      if (!Is(Tok(), Token::kUint)) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected second split level integer"));
      }
      const Formula::belief_level l = std::stoi(Tok().val.str());
      Advance();
      if (!Is(Tok(), Token::kGreater)) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected '>'"));
      }
      Advance();
      Result<Action<Formula::Ref>> alpha = primary_formula();
      if (!alpha) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected primary formula within modality"), alpha);
      }
      if (!Is(Tok(), Token::kDoubleRArrow)) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected conditional belief arrow"));
      }
      Advance();
      Result<Action<Formula::Ref>> beta = primary_formula();
      if (!beta) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected primary formula within modality"), beta);
      }
      return Success<Action<Formula::Ref>>([this, k, l, alpha_a = alpha.val, beta_a = beta.val](Context* ctx) {
        Result<Formula::Ref> alpha = alpha_a.Run(ctx);
        if (!alpha) {
          return Error<Formula::Ref>(LIMBO_MSG("Expected primary formula within modality"), alpha);
        }
        Result<Formula::Ref> beta = beta_a.Run(ctx);
        if (!beta) {
          return Error<Formula::Ref>(LIMBO_MSG("Expected primary formula within modality"), beta);
        }
        return Success(Formula::Factory::Bel(k, l, std::move(alpha.val), std::move(beta.val)));
      });
    }
    if (Is(Tok(), Token::kGuarantee)) {
      Advance();
      Result<Action<Formula::Ref>> alpha = primary_formula();
      if (!alpha) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected primary formula within modality"), alpha);
      }
      return Success<Action<Formula::Ref>>([this, alpha_a = alpha.val](Context* ctx) {
        Result<Formula::Ref> alpha = alpha_a.Run(ctx);
        if (!alpha) {
          return Error<Formula::Ref>(LIMBO_MSG("Expected primary formula within modality"), alpha);
        }
        return Success(Formula::Factory::Guarantee(std::move(alpha.val)));
      });
    }
    if (Is(Tok(), Token::kLBracket)) {
      Advance();
      Result<Action<Term>> t = term();
      if (!t) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected a term in action"), t);
      }
      if (!Is(Tok(), Token::kRBracket)) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected ']'"));
      }
      Advance();
      Result<Action<Formula::Ref>> alpha = primary_formula();
      if (!alpha) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected a primary formula within action"), alpha);
      }
      return Success<Action<Formula::Ref>>([this, t_a = t.val, alpha_a = alpha.val](Context* ctx) {
        Result<Term> t = t_a.Run(ctx);
        if (!t) {
          return Error<Formula::Ref>(LIMBO_MSG("Expected a term in action"), t);
        }
        Result<Formula::Ref> alpha = alpha_a.Run(ctx);
        if (!alpha) {
          return Error<Formula::Ref>(LIMBO_MSG("Expected a primary formula within negation"), alpha);
        }
        return Success<Formula::Ref>(Formula::Factory::Action(t.val, std::move(alpha.val)));
      });
    }
    if (Is(Tok(), Token::kRegress)) {
      Advance();
      Result<Action<Formula::Ref>> alpha = primary_formula();
      if (!alpha) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected primary formula within regression operator"), alpha);
      }
      return Success<Action<Formula::Ref>>([this, alpha_a = alpha.val](Context* ctx) {
        Result<Formula::Ref> alpha = alpha_a.Run(ctx);
        if (!alpha) {
          return Error<Formula::Ref>(LIMBO_MSG("Expected primary formula within modality"), alpha);
        }
        return Success(ctx->Regress(*alpha.val));
      });
    }
    if (Is(Tok(), Token::kLParen)) {
      Advance();
      Result<Action<Formula::Ref>> alpha = formula();
      if (!alpha) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected formula within brackets"), alpha);
      }
      if (!Is(Tok(), Token::kRParen)) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected closing right parenthesis ')'"));
      }
      Advance();
      return Success<Action<Formula::Ref>>([this, alpha_a = alpha.val](Context* ctx) {
        Result<Formula::Ref> alpha = alpha_a.Run(ctx);
        if (!alpha) {
          return Error<Formula::Ref>(LIMBO_MSG("Expected formula within brackets"), alpha);
        }
        return Success(std::move(alpha.val));
      });
    }
    if (Is(Tok(), Token::kIdentifier) &&
        !(Is(Tok(1), Token::kLParen) || Is(Tok(1), Token::kEquality) || Is(Tok(1), Token::kInequality))) {
      std::string id = Tok().val.str();
      Advance();
      return Success<Action<Formula::Ref>>([this, id](Context* ctx) {
        if (!ctx->IsRegisteredFormula(id)) {
          return Error<Formula::Ref>(LIMBO_MSG("Undefined formula abbreviation "+ id));
        }
        return Success(ctx->LookupFormula(id).Clone());
      });
    }
    Result<Action<Literal>> a = literal();
    if (!a) {
      return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected literal"), a);
    }
    return Success<Action<Formula::Ref>>([this, a_a = a.val](Context* ctx) {
      Result<Literal> a = a_a.Run(ctx);
      if (!a) {
        return Error<Formula::Ref>(LIMBO_MSG("Expected literal"), a);
      }
      return Success(Formula::Factory::Atomic(Clause{a.val}));
    });
  }

  // conjunctive_formula --> primary_formula [ && primary_formula ]*
  Result<Action<Formula::Ref>> conjunctive_formula() {
    Result<Action<Formula::Ref>> alpha = primary_formula();
    if (!alpha) {
      return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected left conjunctive formula"), alpha);
    }
    while (Is(Tok(), Token::kAnd)) {
      Advance();
      Result<Action<Formula::Ref>> beta = primary_formula();
      if (!beta) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected left conjunctive formula"), beta);
      }
      alpha = Success<Action<Formula::Ref>>([alpha_a = alpha.val, beta_a = beta.val](Context* ctx) {
        Result<Formula::Ref> alpha = alpha_a.Run(ctx);
        if (!alpha) {
          return Error<Formula::Ref>(LIMBO_MSG("Expected left conjunctive formula"), alpha);
        }
        Result<Formula::Ref> beta = beta_a.Run(ctx);
        if (!beta) {
          return Error<Formula::Ref>(LIMBO_MSG("Expected right conjunctive formula"), beta);
        }
        return Success(Formula::Factory::And(std::move(alpha.val), std::move(beta.val)));
      });
    }
    return alpha;
  }

  // disjunctive_formula --> conjunctive_formula [ || conjunctive_formula ]*
  Result<Action<Formula::Ref>> disjunctive_formula() {
    Result<Action<Formula::Ref>> alpha = conjunctive_formula();
    if (!alpha) {
      return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected left argument conjunctive formula"), alpha);
    }
    while (Is(Tok(), Token::kOr)) {
      Advance();
      Result<Action<Formula::Ref>> beta = conjunctive_formula();
      if (!beta) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected right argument conjunctive formula"), beta);
      }
      alpha = Success<Action<Formula::Ref>>([alpha_a = alpha.val, beta_a = beta.val](Context* ctx) {
        Result<Formula::Ref> alpha = alpha_a.Run(ctx);
        if (!alpha) {
          return Error<Formula::Ref>(LIMBO_MSG("Expected left argument conjunctive formula"), alpha);
        }
        Result<Formula::Ref> beta = beta_a.Run(ctx);
        if (!beta) {
          return Error<Formula::Ref>(LIMBO_MSG("Expected right argument conjunctive formula"), beta);
        }
        return Success(Formula::Factory::Or(std::move(alpha.val), std::move(beta.val)));
      });
    }
    return alpha;
  }

  // implication_formula --> disjunctive_formula -> implication_formula
  //                      |  disjunctive_formula
  Result<Action<Formula::Ref>> implication_formula() {
    Result<Action<Formula::Ref>> alpha = disjunctive_formula();
    if (!alpha) {
      return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected left argument disjunctive formula"), alpha);
    }
    if (Is(Tok(), Token::kRArrow)) {
      Advance();
      Result<Action<Formula::Ref>> beta = implication_formula();
      if (!beta) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected right argument disjunctive formula"), beta);
      }
      alpha = Success<Action<Formula::Ref>>([this, alpha_a = alpha.val, beta_a = beta.val](Context* ctx) {
        Result<Formula::Ref> alpha = alpha_a.Run(ctx);
        if (!alpha) {
          return Error<Formula::Ref>(LIMBO_MSG("Expected left argument disjunctive formula"), alpha);
        }
        Result<Formula::Ref> beta = beta_a.Run(ctx);
        if (!beta) {
          return Error<Formula::Ref>(LIMBO_MSG("Expected right argument disjunctive formula"), beta);
        }
        return Success(Formula::Factory::Impl(std::move(alpha.val), std::move(beta.val)));
      });
    }
    return alpha;
  }

  // equivalence_formula --> implication_formula <-> implication_formula
  //                      |  implication_formula
  Result<Action<Formula::Ref>> equivalence_formula() {
    Result<Action<Formula::Ref>> alpha = implication_formula();
    if (!alpha) {
      return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected left argument implication formula"), alpha);
    }
    if (Is(Tok(), Token::kLRArrow)) {
      Advance();
      Result<Action<Formula::Ref>> beta = implication_formula();
      if (!beta) {
        return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected right argument implication formula"), beta);
      }
      alpha = Success<Action<Formula::Ref>>([alpha_a = alpha.val, beta_a = beta.val](Context* ctx) {
        Result<Formula::Ref> alpha = alpha_a.Run(ctx);
        if (!alpha) {
          return Error<Formula::Ref>(LIMBO_MSG("Expected left argument implication formula"), alpha);
        }
        Result<Formula::Ref> beta = beta_a.Run(ctx);
        if (!beta) {
          return Error<Formula::Ref>(LIMBO_MSG("Expected right argument implication formula"), beta);
        }
        return Success(Formula::Factory::Equiv(std::move(alpha.val), std::move(beta.val)));
      });
    }
    return alpha;
  }

  // formula --> equivalence_formula
  Result<Action<Formula::Ref>> formula() {
    return equivalence_formula();
  }

  // real_literal --> Real : literal
  Result<Action<>> real_literal() {
    if (!Is(Tok(), Token::kReal)) {
      return Unapplicable<Action<>>(LIMBO_MSG("Expected 'Real'"));
    }
    Advance();
    if (!Is(Tok(), Token::kColon)) {
      return Error<Action<>>(LIMBO_MSG("Expected ':'"));
    }
    Advance();
    Result<Action<Literal>> a = literal();
    if (!a) {
      return Error<Action<>>(LIMBO_MSG("Expected real world literal"), a);
    }
    return Success<Action<>>([this, a_a = a.val](Context* ctx) {
      Result<Literal> a = a_a.Run(ctx);
      if (!a) {
        return Error<>(LIMBO_MSG("Expected real world literal"), a);
      }
      if (!a.val.ground() || a.val.unsatisfiable()) {
        return Error<>(LIMBO_MSG("Real world literal must be ground and satisfiable"));
      }
      ctx->AddReal(a.val);
      return Success<>();
    });
  }

  // kb_formula --> KB : formula
  //             |  KB : [] [ [atomic_term] ] term = atomic_term <-> formula
  Result<Action<>> kb_formula() {
    if (!Is(Tok(), Token::kKB)) {
      return Unapplicable<Action<>>(LIMBO_MSG("Expected 'KB'"));
    }
    Advance();
    if (!Is(Tok(), Token::kColon)) {
      return Error<Action<>>(LIMBO_MSG("Expected ':'"));
    }
    Advance();
    if (Is(Tok(0), Token::kBox)) {
      Advance();
      // Remainder is:
      // [ [atomic_term] ] term = atomic_term <-> formula
      const bool ssa = Is(Tok(), Token::kLBracket);
      Result<Action<Term>> t;
      if (ssa) {
        Advance();
        t = atomic_term();
        if (!t) {
          return Error<Action<>>(LIMBO_MSG("Expected action variable"), t);
        }
        if (!Is(Tok(), Token::kRBracket)) {
          return Error<Action<>>(LIMBO_MSG("Expected ']'"));
        }
        Advance();
      }
      Result<Action<Literal>> a = literal();
      if (!a) {
        return Error<Action<>>(LIMBO_MSG("Expected KB dynamic left-hand side literal"), a);
      }
      if (!Is(Tok(), Token::kLRArrow)) {
        return Error<Action<>>(LIMBO_MSG("Expected '<->'"));
      }
      Advance();
      Result<Action<Formula::Ref>> alpha = formula();
      if (!alpha) {
        return Error<Action<>>(LIMBO_MSG("Expected KB dynamic right-hand side formula"), alpha);
      }
      return Success<Action<>>([this, ssa, t_a = t.val, a_a = a.val, alpha_a = alpha.val](Context* ctx) {
        Result<Term> t;
        if (ssa) {
          t = t_a.Run(ctx);
          if (!t || !t.val.variable()) {
            return Error<>(LIMBO_MSG("Expected action variable"), t);
          }
        }
        Result<Literal> a = a_a.Run(ctx);
        if (!a) {
          return Error<>(LIMBO_MSG("Expected KB dynamic left-hand side literal"), a);
        }
        if (!(a.val.pos() && a.val.lhs().sort() == a.val.rhs().sort() && !a.val.lhs().sort().rigid())) {
          return Error<>(LIMBO_MSG("Left-hand side literal of dynamic axiom must be of the form f(x,...) = y "
                                   "f, y of same, non-rigid sort"));
        }
        Result<Formula::Ref> alpha = alpha_a.Run(ctx);
        if (!alpha) {
          return Error<>(LIMBO_MSG("Expected KB dynamic right-hand side formula"), alpha);
        }
        if (!(alpha.val->objective() && !alpha.val->dynamic())) {
          return Error<>(LIMBO_MSG("KB dynamic right-hand side formula must not contain modal operators"), alpha);
        }
        Formula::SortedTermSet xs;
        a.val.Traverse([&xs](const Term t) { if (t.variable()) { xs.insert(t); } return true; });
        if (ssa) {
          t.val.Traverse([&xs](const Term t) { if (t.variable()) { xs.insert(t); } return true; });
        }
        for (Term y : alpha.val->free_vars().values()) {
          if (!xs.contains(y)) {
            return Error<>(LIMBO_MSG("Free variables in the right-hand side of dynamic axiom must be bound by the "
                                     "left-hand side"));
          }
        }
        if ((ssa && ctx->Add(t.val, a.val, *alpha.val)) ||
            (!ssa && ctx->Add(a.val, *alpha.val))) {
          return Success<>();
        } else {
          return Error<>(LIMBO_MSG("Couldn't add formula to KB; is it proper+ "
                                  "(i.e., its NF must be a universally quantified clause)?"));
        }
      });
    } else {
      // Remainder is:
      // formula
      Result<Action<Formula::Ref>> alpha = formula();
      if (!alpha) {
        return Error<Action<>>(LIMBO_MSG("Expected KB formula"), alpha);
      }
      return Success<Action<>>([this, alpha_a = alpha.val](Context* ctx) {
        Result<Formula::Ref> alpha = alpha_a.Run(ctx);
        if (!alpha) {
          return Error<>(LIMBO_MSG("Expected KB formula"), alpha);
        }
        if (ctx->Add(*alpha.val)) {
          return Success<>();
        } else {
          return Error<>(LIMBO_MSG("Couldn't add formula to KB; is it proper+ "
                                  "(i.e., its NF must be a universally quantified clause)?"));
        }
      });
    }
  }

  // subjective_formula --> formula
  Result<Action<Formula::Ref>> subjective_formula() {
    Result<Action<Formula::Ref>> alpha = formula();
    if (!alpha) {
      return Error<Action<Formula::Ref>>(LIMBO_MSG("Expected subjective formula"), alpha);
    }
    return Success<Action<Formula::Ref>>([this, alpha_a = alpha.val](Context* ctx) {
      Result<Formula::Ref> alpha = alpha_a.Run(ctx);
      if (!alpha) {
        return Error<Formula::Ref>(LIMBO_MSG("Expected subjective formula"), alpha);
      }
#if 0
      if (!alpha.val->subjective()) {
        return Error<Formula::Ref>(LIMBO_MSG("Expected subjective formula "
                                              "(i.e., no functions outside of modal operators; "
                                              "probably caused by missing brackets)"));
      }
#endif
      return Success<Formula::Ref>(std::move(alpha.val));
    });
  }

  // query --> [ Query | Refute | Assert ] : subjective_formula
  Result<Action<>> query() {
    if (!(Is(Tok(), Token::kQuery) || Is(Tok(), Token::kNot) || Is(Tok(), Token::kForall) ||
          Is(Tok(), Token::kExists) || Is(Tok(), Token::kLParen) || Is(Tok(), Token::kKnow) ||
          Is(Tok(), Token::kCons) || Is(Tok(), Token::kBel) || Is(Tok(), Token::kGuarantee) ||
          Is(Tok(), Token::kRegress) || Is(Tok(), Token::kIdentifier)) &&
        !Is(Tok(), Token::kAssert) &&
        !Is(Tok(), Token::kRefute)) {
      return Unapplicable<Action<>>(LIMBO_MSG("Expected 'Query', 'Assert', or 'Refute'"));
    }
    const bool is_query = !Is(Tok(), Token::kAssert) && !Is(Tok(), Token::kRefute);
    const bool is_assert = Is(Tok(), Token::kAssert);
    if (Is(Tok(), Token::kQuery) || Is(Tok(), Token::kAssert) || Is(Tok(), Token::kRefute)) {
      Advance();
      if (!Is(Tok(), Token::kColon)) {
        return Error<Action<>>(LIMBO_MSG("Expected ':'"));
      }
      Advance();
      }
    Result<Action<Formula::Ref>> alpha = subjective_formula();
    if (!alpha) {
      return Error<Action<>>(LIMBO_MSG("Expected query/assertion/refutation subjective_formula"), alpha);
    }
    return Success<Action<>>([this, alpha_a = alpha.val, is_query, is_assert](Context* ctx) {
      Result<Formula::Ref> alpha = alpha_a.Run(ctx);
      if (!alpha) {
        return Error<>(LIMBO_MSG("Expected query/assertion/refutation subjective_formula"), alpha);
      }
      const bool r = ctx->Query(*alpha.val);
      if (is_query) {
        return Success<>();
      } else if (r == is_assert) {
        return Success<>();
      } else {
        std::stringstream ss;
        using limbo::format::operator<<;
        ss << (is_assert ? "Assertion" : "Refutation") << " of " << *alpha.val << " failed";
        return Error<>(LIMBO_MSG(ss.str()));
      }
    });
  }

  typedef std::pair<std::string, std::vector<Term>> IdTerms;

  // bind_meta_variables --> [ identifier [ in term [, term]* ] -> sort-id ]?
  Result<Action<IdTerms>> bind_meta_variables() {
    if (!Is(Tok(0), Token::kIdentifier) ||
        !(Is(Tok(1), Token::kIn) || Is(Tok(1), Token::kRArrow))) {
      return Success<Action<IdTerms>>([](Context* ctx) {
        return Success<IdTerms>();
      });
    }
    const std::string id = Tok().val.str();
    Advance();
    std::vector<Action<Term>> ts;
    if (Is(Tok(), Token::kIn)) {
      do {
        Advance();
        Result<Action<Term>> t = term();
        if (!t) {
          return Error<Action<IdTerms>>(LIMBO_MSG("Expected argument term"), t);
        }
        ts.push_back(std::move(t.val));
      } while (Is(Tok(), Token::kComma));
    }
    if (!Is(Tok(), Token::kRArrow)) {
      return Error<Action<IdTerms>>(LIMBO_MSG("Expected right arrow '->'"));
    }
    Advance();
    if (!Is(Tok(), Token::kIdentifier)) {
      return Error<Action<IdTerms>>(LIMBO_MSG("Expected sort identifier"));
    }
    const std::string sort = Tok().val.str();
    Advance();
    return Success<Action<IdTerms>>([this, id, sort_id = sort, ts_a = ts](Context* ctx) {
      if (!ctx->IsRegisteredSort(sort_id)) {
        return Error<IdTerms>(LIMBO_MSG("Sort "+ sort_id +" is not registered"));
      }
      Symbol::Sort sort = ctx->LookupSort(sort_id);
      std::vector<Term> ts;
      if (ts_a.empty()) {
        const KnowledgeBase::TermSet& ns = ctx->kb().mentioned_names(sort);
        ts.insert(ts.end(), ns.begin(), ns.end());
      } else {
        for (const Action<Term>& t_a : ts_a) {
          Result<Term> t = t_a.Run(ctx);
          if (!t) {
            return Error<IdTerms>(LIMBO_MSG("Expected term in range"), t);
          }
          if (t.val.sort() != sort) {
            return Error<IdTerms>(LIMBO_MSG("Term in range is not of sort "+ sort_id));
          }
          ts.push_back(t.val);
        }
      }
      return Success<IdTerms>(std::make_pair(id, ts));
    });
  }

  // if_else --> If formula block [ Else block ]
  Result<Action<>> if_else() {
    if (!Is(Tok(), Token::kIf)) {
      return Unapplicable<Action<>>(LIMBO_MSG("Expected 'If'"));
    }
    Advance();
    Result<Action<IdTerms>> bind = bind_meta_variables();
    if (!bind) {
      return Error<Action<>>(LIMBO_MSG("Expected bind_meta_variables"), bind);
    }
    Result<Action<Formula::Ref>> alpha = formula();
    if (!alpha) {
      return Error<Action<>>(LIMBO_MSG("Expected formula in if_else"), alpha);
    }
    Result<Action<>> if_block = block();
    if (!if_block) {
      return Error<Action<>>(LIMBO_MSG("Expected if block in if_else"), if_block);
    }
    Result<Action<>> else_block;
    if (Is(Tok(), Token::kElse)) {
      Advance();
      else_block = block();
      if (!else_block) {
        return Error<Action<>>(LIMBO_MSG("Expected else block in if_else"), else_block);
      }
    } else {
      else_block = Success<Action<>>([](Context* ctx) { return Success<>(); });
    }
    return Success<Action<>>([this, bind_a = bind.val, alpha_a = alpha.val, if_block_a = if_block.val,
                              else_block_a = else_block.val](Context* ctx) {
      Result<IdTerms> bind = bind_a.Run(ctx);
      const std::string id = bind.val.first;
      bool cond;
      if (!id.empty()) {
        cond = false;
        for (Term t : bind.val.second) {
          ctx->RegisterMetaVariable(id, t);
          Result<Formula::Ref> alpha = alpha_a.Run(ctx);
          if (!alpha) {
            return Error<>(LIMBO_MSG("Expected condition subjective_formula"), alpha);
          }
          if (ctx->Query(*alpha.val)) {
            cond = true;
            break;
          }
          ctx->UnregisterMetaVariable(id);
        }
      } else {
        Result<Formula::Ref> alpha = alpha_a.Run(ctx);
        if (!alpha) {
          return Error<>(LIMBO_MSG("Expected condition subjective_formula"), alpha);
        }
        cond = ctx->Query(*alpha.val);
      }
      Result<> r;
      if (cond) {
        r = if_block_a.Run(ctx);
        if (!id.empty()) {
          ctx->UnregisterMetaVariable(id);
        }
      } else {
        r = else_block_a.Run(ctx);
      }
      if (!r) {
        return Error<>(LIMBO_MSG("Expected block in if_else"), r);
      }
      return r;
    });
  }

  // while_loop --> While formula block [ Else block ]
  Result<Action<>> while_loop() {
    if (!Is(Tok(), Token::kWhile)) {
      return Unapplicable<Action<>>(LIMBO_MSG("Expected 'While'"));
    }
    Advance();
    Result<Action<IdTerms>> bind = bind_meta_variables();
    if (!bind) {
      return Error<Action<>>(LIMBO_MSG("Expected bind_meta_variables"), bind);
    }
    Result<Action<Formula::Ref>> alpha = formula();
    if (!alpha) {
      return Error<Action<>>(LIMBO_MSG("Expected formula in while_loop"), alpha);
    }
    Result<Action<>> while_block = block();
    if (!while_block) {
      return Error<Action<>>(LIMBO_MSG("Expected if block in while_else"), while_block);
    }
    Result<Action<>> else_block;
    if (Is(Tok(), Token::kElse)) {
      Advance();
      else_block = block();
      if (!else_block) {
        return Error<Action<>>(LIMBO_MSG("Expected else block in while_loop"), else_block);
      }
    } else {
      else_block = Success<Action<>>([](Context* ctx) { return Success<>(); });
    }
    return Success<Action<>>([this, bind_a = bind.val, alpha_a = alpha.val, while_block_a = while_block.val,
                              else_block_a = else_block.val](Context* ctx) {
      Result<IdTerms> bind = bind_a.Run(ctx);
      const std::string id = bind.val.first;
      bool once = false;
      for (;;) {
        bool cond;
        if (!id.empty()) {
          cond = false;
          for (Term t : bind.val.second) {
            ctx->RegisterMetaVariable(id, t);
            Result<Formula::Ref> alpha = alpha_a.Run(ctx);
            if (!alpha) {
              return Error<>(LIMBO_MSG("Expected condition subjective_formula"), alpha);
            }
            if (ctx->Query(*alpha.val)) {
              cond = true;
              break;
            }
            ctx->UnregisterMetaVariable(id);
          }
        } else {
          Result<Formula::Ref> alpha = alpha_a.Run(ctx);
          if (!alpha) {
            return Error<>(LIMBO_MSG("Expected condition subjective_formula"), alpha);
          }
          cond = ctx->Query(*alpha.val);
        }
        if (cond) {
          once = true;
          Result<> r = while_block_a.Run(ctx);
          if (!id.empty()) {
            ctx->UnregisterMetaVariable(id);
          }
          if (!r) {
            return Error<>(LIMBO_MSG("Expected block in while_loop"), r);
          }
        } else {
          break;
        }
      }
      if (!once) {
        Result<> r = else_block_a.Run(ctx);
        if (!r) {
          return Error<>(LIMBO_MSG("Expected block in while_loop"), r);
        }
      }
      return Success<>();
    });
  }

  // for_loop --> For formula block [ Else block ]
  Result<Action<>> for_loop() {
    if (!Is(Tok(), Token::kFor)) {
      return Unapplicable<Action<>>(LIMBO_MSG("Expected 'For'"));
    }
    Advance();
    Result<Action<IdTerms>> bind = bind_meta_variables();
    if (!bind) {
      return Error<Action<>>(LIMBO_MSG("Expected bind_meta_variables"), bind);
    }
    Result<Action<Formula::Ref>> alpha = formula();
    if (!alpha) {
      return Error<Action<>>(LIMBO_MSG("Expected formula in for_loop"), alpha);
    }
    Result<Action<>> for_block = block();
    if (!for_block) {
      return Error<Action<>>(LIMBO_MSG("Expected if block in for_else"), for_block);
    }
    Result<Action<>> else_block;
    if (Is(Tok(), Token::kElse)) {
      Advance();
      else_block = block();
      if (!else_block) {
        return Error<Action<>>(LIMBO_MSG("Expected else block in for_loop"), else_block);
      }
    } else {
      else_block = Success<Action<>>([](Context* ctx) { return Success<>(); });
    }
    return Success<Action<>>([this, bind_a = bind.val, alpha_a = alpha.val, for_block_a = for_block.val,
                             else_block_a = else_block.val](Context* ctx) {
      Result<IdTerms> bind = bind_a.Run(ctx);
      const std::string id = bind.val.first;
      if (id.empty()) {
        return Error<>(LIMBO_MSG("Expected meta variable id"));
      }
      bool once = false;
      for (Term t : bind.val.second) {
        ctx->RegisterMetaVariable(id, t);
        Result<Formula::Ref> alpha = alpha_a.Run(ctx);
        if (!alpha) {
          return Error<>(LIMBO_MSG("Expected condition subjective_formula"), alpha);
        }
        if (ctx->Query(*alpha.val)) {
          once = true;
          Result<> r = for_block_a.Run(ctx);
          if (!r) {
            ctx->UnregisterMetaVariable(id);
            return Error<>(LIMBO_MSG("Expected block in for_loop"), r);
          }
        }
        ctx->UnregisterMetaVariable(id);
      }
      if (!once) {
        Result<> r = else_block_a.Run(ctx);
        if (!r) {
          return Error<>(LIMBO_MSG("Expected block in for_loop"), r);
        }
      }
      return Success<>();
    });
  }

  // abbreviation --> let identifier := formula
  Result<Action<>> abbreviation() {
    if (!Is(Tok(), Token::kLet)) {
      return Unapplicable<Action<>>(LIMBO_MSG("Expected abbreviation operator 'let'"));
    }
    Advance();
    if (!Is(Tok(), Token::kIdentifier)) {
      return Error<Action<>>(LIMBO_MSG("Expected fresh identifier"));
    }
    const std::string id = Tok().val.str();
    Advance();
    if (!Is(Tok(), Token::kAssign)) {
      return Error<Action<>>(LIMBO_MSG("Expected assignment operator ':='"));
    }
    Advance();
    Result<Action<Formula::Ref>> alpha = formula();
    if (!alpha) {
      return Error<Action<>>(LIMBO_MSG("Expected formula"), alpha);
    }
    return Success<Action<>>([this, id, alpha_a = alpha.val](Context* ctx) {
      Result<Formula::Ref> alpha = alpha_a.Run(ctx);
      if (!alpha) {
        return Error<>(LIMBO_MSG("Expected formula"), alpha);
      }
      ctx->RegisterFormula(id, *alpha.val);
      return Success<>();
    });
  }

  // call --> Call id
  Result<Action<>> call() {
    if (!Is(Tok(), Token::kCall)) {
      return Unapplicable<Action<>>(LIMBO_MSG("Expected 'Call'"));
    }
    Advance();
    if (!Is(Tok(), Token::kColon)) {
      return Error<Action<>>(LIMBO_MSG("Expected ':'"));
    }
    Advance();
    if (!Is(Tok(), Token::kIdentifier)) {
      return Error<Action<>>(LIMBO_MSG("Expected meta variable in for_loop"));
    }
    const std::string id = Tok().val.str();
    Advance();
    if (!Is(Tok(), Token::kLParen)) {
      return Error<Action<>>(LIMBO_MSG("Expected opening parentheses '('"));
    }
    std::vector<Action<Term>> ts;
    do {
      Advance();
      if (Is(Tok(), Token::kRParen)) {
        break;
      }
      Result<Action<Term>> t = term();
      if (!t) {
        return Error<Action<>>(LIMBO_MSG("Expected argument"), t);
      }
      ts.push_back(std::move(t.val));
    } while (Is(Tok(), Token::kComma));
    if (!Is(Tok(), Token::kRParen)) {
      return Error<Action<>>(LIMBO_MSG("Expected closing parentheses '('"));
    }
    Advance();
    return Success<Action<>>([this, id, ts_as = ts](Context* ctx) {
      std::vector<Term> ts;
      for (const Action<Term>& arg_a : ts_as) {
        Result<Term> t = arg_a.Run(ctx);
        ts.push_back(t.val);
      }
      ctx->Call(id, ts);
      return Success<>();
    });
  }

  // block --> Begin branch* End
  Result<Action<>> block() {
    if (!Is(Tok(), Token::kBegin)) {
      Result<Action<>> r = branch();
      if (!r) {
        return Error<Action<>>(LIMBO_MSG("Expected branch in block"), r);
      }
      return r;
    } else {
      Advance();
      const size_t n_blocks = n_blocks_;
      ++n_blocks_;
      Action<> a;
      while (n_blocks_ > n_blocks) {
        if (Is(Tok(), Token::kEnd)) {
          Advance();
          --n_blocks_;
        } else {
          Result<Action<>> r = branch();
          if (!r) {
            return Error<Action<>>(LIMBO_MSG("Expected branch in block"), r);
          }
          a += r.val;
        }
      }
      return Success<Action<>>(std::move(a));
    }
  }

  // branch --> [ declaration | real_literal | kb_formula | abbreviation | query | if_else | while_loop
  //            | for_loop | call ]
  Result<Action<>> branch() {
    typedef Result<Action<>> (Parser::*Rule)();
    std::vector<Rule> rules = {&Parser::declaration, &Parser::real_literal, &Parser::kb_formula, &Parser::abbreviation,
                               &Parser::query, &Parser::if_else, &Parser::while_loop, &Parser::for_loop, &Parser::call};
    for (Rule rule : rules) {
      Result<Action<>> r = (this->*rule)();
      if (r) {
        return r;
      } else if (r.applied()) {
        return Error<Action<>>(LIMBO_MSG("Error in branch"), r);
      }
    }
    return Unapplicable<Action<>>(LIMBO_MSG("No rule applicable in branch"));
  }

  // start --> branch
  Result<Action<>> start() {
    iterator prev;
    Action<> a = [](Context*) { return Success<>(); };
    while (Tok()) {
      const Result<Action<>> r = branch();
      if (!r) {
        std::stringstream ss;
        using limbo::format::operator<<;
        ss << Tok(0) << " " << Tok(1) << " " << Tok(2) << "...";
        return Error<Action<>>(LIMBO_MSG("Error in start with unparsed input "+ ss.str()), r);
      }
      a += r.val;
    }
    return Success<Action<>>(std::move(a));
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

  iterator begin() const {
    while (begin_plus_ > 0) {
      assert(begin_ != end_);
      ++begin_;
      begin_plus_--;
    }
    return begin_;
  }

  iterator end() const {
    return end_;
  }

  static const char* kUnapplicableLabel;
  static const char* kErrorLabel;
  static const char* kCausesLabel;

  Lex lexer_;
  mutable iterator begin_;  // don't use begin_ directly: to avoid the stream blocking us, Advance() actually increments
  mutable size_t begin_plus_ = 0;  // begin_plus_ instead of begin_; use begin() to obtain the incremented iterator.
  iterator end_;
  size_t n_blocks_ = 0;
};


template<typename ForwardIt, typename Context>
const char* Parser<ForwardIt, Context>::kUnapplicableLabel = "Unappl.: ";

template<typename ForwardIt, typename Context>
const char* Parser<ForwardIt, Context>::kErrorLabel        = "Failure: ";

template<typename ForwardIt, typename Context>
const char* Parser<ForwardIt, Context>::kCausesLabel       = " causes: ";

}  // namespace pdl
}  // namespace format
}  // namespace limbo

#endif  // LIMBO_FORMAT_PDL_PARSER_H_

