// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering
//
// Recursive descent parser for the problem description language. The grammar
// for formulas is aims to reduce brackets and implement operator precedence.
// See the comment above Parser::start() and its callees for the grammar
// definition.

#ifndef LELA_FORMAT_PDL_PARSER_H_
#define LELA_FORMAT_PDL_PARSER_H_

#include <cassert>

#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <lela/formula.h>
#include <lela/setup.h>
#include <lela/format/output.h>
#include <lela/format/pdl/context.h>
#include <lela/format/pdl/lexer.h>

namespace lela {
namespace format {
namespace pdl {

#define LELA_S(x)          #x
#define LELA_S_(x)         LELA_S(x)
#define LELA_STR__LINE__   LELA_S_(__LINE__)
#define LELA_MSG(msg)      (std::string(msg) +" (in rule "+ __FUNCTION__ +":"+ LELA_STR__LINE__ +")")

template<typename ForwardIt, typename LogPredicate>
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
    enum Kind { kSuccess, kUnapplicable, kError };

    Result() = default;
    Result(Result&&) = default;
    Result& operator=(Result&&) = default;

    explicit Result(T&& val) : val(std::forward<T>(val)), kind_(kSuccess) {}

    Result(Kind kind, const std::string& msg, ForwardIt begin = ForwardIt(), ForwardIt end = ForwardIt(), T&& val = T())
        : val(std::forward<T>(val)), kind_(kind), msg_(msg), begin_(begin), end_(end) {}

    explicit operator bool() const { return kind_ == kSuccess; }
    bool successful() const { return kind_ == kSuccess; }
    bool applied() const { return kind_ != kUnapplicable; }

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
    bool kind_;
    std::string msg_;

    ForwardIt begin_;
    ForwardIt end_;
  };

  template<typename T = Void>
  class Action : public std::shared_ptr<std::function<Result<T>()>> {
   public:
    typedef std::function<Result<T>()> function;
    typedef std::shared_ptr<function> base;

    Action() = default;

    template<typename NullaryFunction>
    Action(NullaryFunction func) : base::shared_ptr(new function(func)) {}

    //template<typename NullaryFunction>
    //Action(NullaryFunction&& func) : base::shared_ptr(new function(std::forward<NullaryFunction>(func))) {}

    Result<T> Run() const {
      function* f = base::get();
      if (f) {
        return (*f)();
      } else {
        return Result<T>(Result<T>::kError, LELA_MSG("Action is null"));
      }
    }

    friend Action operator+(Action a, Action b) {
      if (!a) {
        return b;
      }
      if (!b) {
        return a;
      }
      return [a, b]() {
        Result<> r = (*a)();
        if (!r) {
          return r;
        }
        return (*b)();
      };
    }

    Action operator+=(Action a) {
      *this = *this + a;
      return *this;
    }
  };

  Parser(ForwardIt begin, ForwardIt end, Context<LogPredicate>* ctx)
      : lexer_(begin, end), begin_(lexer_.begin()), end_(lexer_.end()), ctx_(ctx) {}

  Result<Action<>> Parse() { return start(); }

  Context<LogPredicate>& ctx() { return *ctx_; }
  const Context<LogPredicate>& ctx() const { return *ctx_; }

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
    return Result<T>(Result<T>::kUnapplicable, "\t" + msg, begin().char_iter(), end().char_iter());
  }

#if 1
  // declaration --> sort <sort-id> [ , <sort-id>]*
  //              |  var <id> [ , <id> ]* -> <sort-id>
  //              |  name <id> [ , <id> ]* -> <sort-id>
  //              |  fun <id> [ , <id> ]* / <arity> -> <sort-id>
  Result<Action<>> declaration() {
    if (Is(Tok(), Token::kSort)) {
      Action<> a;
      do {
        Advance();
        if (Is(Tok(), Token::kIdentifier)) {
          const std::string id = Tok().val.str();
          Advance();
          a += [this, id]() {
            if (!ctx_->IsRegisteredSort(id)) {
              ctx_->RegisterSort(id);
              return Success<>();
            } else {
              return Error<>(LELA_MSG("Sort "+ id +" is already registered"));
            }
          };
        } else {
          return Error<Action<>>(LELA_MSG("Expected sort identifier"));
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
          return Error<Action<>>(LELA_MSG(var ? "Expected variable identifier" : "Expected name identifier"));
        }
      } while (Is(Tok(0), Token::kComma));
      if (Is(Tok(0), Token::kRArrow) &&
          Is(Tok(1), Token::kIdentifier)) {
        const std::string sort = Tok(1).val.str();
        Advance(2);
        Action<> a;
        for (const std::string& id : ids) {
          a += [this, var, sort, id]() {
            if (ctx_->IsRegisteredSort(sort)) {
              if (!ctx_->IsRegisteredTerm(id)) {
                var ? ctx_->RegisterVariable(id, sort) : ctx_->RegisterName(id, sort);
                return Success<>();
              } else {
                return Error<>(LELA_MSG("Term "+ id +" is already registered"));
              }
            } else {
              return Error<>(LELA_MSG("Sort "+ sort +" is already registered"));
            }
          };
        }
        return Success<Action<>>(std::move(a));
      } else {
        return Error<Action<>>(LELA_MSG("Expected arrow and sort identifier"));
      }
    }
    if (Is(Tok(), Token::kFun)) {
      std::vector<std::pair<std::string, Symbol::Arity>> ids;
      do {
        Advance();
        if (Is(Tok(0), Token::kIdentifier) &&
            Is(Tok(1), Token::kSlash) &&
            Is(Tok(2), Token::kUint)) {
          ids.push_back(std::make_pair(Tok(0).val.str(), std::stoi(Tok(2).val.str())));
          Advance(3);
        } else {
          return Error<Action<>>(LELA_MSG("Expected function identifier"));
        }
      } while (Is(Tok(0), Token::kComma));
      if (Is(Tok(0), Token::kRArrow) &&
          Is(Tok(1), Token::kIdentifier)) {
        const std::string sort = Tok(1).val.str();
        Advance(2);
        Action<> a;
        for (const auto& id_arity : ids) {
          const std::string id = id_arity.first;
          const Symbol::Arity arity = id_arity.second;
          a += [this, sort, id, arity]() {
            if (ctx_->IsRegisteredSort(sort)) {
              if (!ctx_->IsRegisteredTerm(id)) {
                ctx_->RegisterFunction(id, arity, sort);
                return Success<>();
              } else {
                return Error<>(LELA_MSG("Term "+ id +" is already registered"));
              }
            } else {
              return Error<>(LELA_MSG("Sort "+ sort +" is already registered"));
            }
          };
        }
        return Success<Action<>>(std::move(a));
      } else {
        return Error<Action<>>(LELA_MSG("Expected arrow and sort identifier"));
      }
    }
    return Unapplicable<Action<>>(LELA_MSG("Expected 'Sort', 'Var', 'Name' or 'Fun'"));
  }

  // atomic_term --> x
  //              |  n
  //              |  f
  Result<Action<Term>> atomic_term() {
    if (Is(Tok(), Token::kIdentifier)) {
      const std::string id = Tok().val.str();
      Advance();
      return Success<Action<Term>>([this, id]() {
        if (ctx_->IsRegisteredVariable(id)) {
          return Success<Term>(ctx_->LookupVariable(id));
        } else if (ctx_->IsRegisteredName(id)) {
          return Success<Term>(ctx_->LookupName(id));
        } else if (ctx_->IsRegisteredFunction(id)) {
          Symbol f = ctx_->LookupFunction(id);
          if (f.arity() != 0) {
            return Error<Term>(LELA_MSG("Wrong number of arguments for "+ id));
          }
          return Success(ctx_->tf()->CreateTerm(f));
        } else {
          return Error<Term>(LELA_MSG("Error in atomic_term"));
        }
      });
    }
    return Error<Action<Term>>(LELA_MSG("Expected a declared variable/name/function identifier"));
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
      if (Is(Tok(), Token::kLeftParen)) {
        Advance();
        for (;;) {
          Result<Action<Term>> t = term();
          if (!t) {
            return Error<Action<Term>>(LELA_MSG("Expected argument term"), t);
          }
          args.push_back(t.val);
          if (Is(Tok(), Token::kComma)) {
            Advance();
            continue;
          } else if (Is(Tok(), Token::kRightParen)) {
            Advance();
            break;
          } else {
            return Error<Action<Term>>(LELA_MSG("Expected comma ',' or closing parenthesis ')'"));
          }
        }
      }
      return Success<Action<Term>>([this, id, args_a = args]() {
        if (ctx_->IsRegisteredVariable(id)) {
          return Success<Term>(ctx_->LookupVariable(id));
        } else if (ctx_->IsRegisteredName(id)) {
          return Success<Term>(ctx_->LookupName(id));
        } else if (ctx_->IsRegisteredFunction(id)) {
          Symbol f = ctx_->LookupFunction(id);
          if (f.arity() != args_a.size()) {
            return Error<Term>(LELA_MSG("Wrong number of arguments for "+ id));
          }
          Term::Vector args;
          for (const Action<Term>& a : args_a) {
            Result<Term> t = a.Run();
            if (t) {
              args.push_back(t.val);
            } else {
              return Error<Term>(LELA_MSG("Expected argument term"), t);
            }
          }
          return Success(ctx_->tf()->CreateTerm(f, args));
        } else {
          return Error<Term>(LELA_MSG("Error in term"));
        }
      });
    }
    return Error<Action<Term>>(LELA_MSG("Expected a declared variable/name/function identifier"));
  }

  // literal --> term [ '==' | '!=' ] term
  Result<Action<std::unique_ptr<Literal>>> literal() {
    Result<Action<Term>> lhs = term();
    if (!lhs) {
      return Error<Action<std::unique_ptr<Literal>>>(LELA_MSG("Expected a lhs term"), lhs);
    }
    bool pos;
    if (Is(Tok(), Token::kEquality) || Is(Tok(), Token::kInequality)) {
      pos = Is(Tok(), Token::kEquality);
      Advance();
    } else {
      return Error<Action<std::unique_ptr<Literal>>>(LELA_MSG("Expected equality or inequality '=='/'!='"));
    }
    Result<Action<Term>> rhs = term();
    if (!rhs) {
      return Error<Action<std::unique_ptr<Literal>>>(LELA_MSG("Expected rhs term"), rhs);
    }
    return Success<Action<std::unique_ptr<Literal>>>([lhs_a = lhs.val, pos, rhs_a = rhs.val]() {
      Result<Term> lhs = lhs_a.Run();
      if (!lhs) {
        return Error<std::unique_ptr<Literal>>(LELA_MSG("Expected a lhs term"), lhs);
      }
      Result<Term> rhs = rhs_a.Run();
      if (!rhs) {
        return Error<std::unique_ptr<Literal>>(LELA_MSG("Expected a rhs term"), rhs);
      }
      Literal a = pos ? Literal::Eq(lhs.val, rhs.val) : Literal::Neq(lhs.val, rhs.val);
      return Success(std::unique_ptr<Literal>(new Literal(a)));
    });
  }

  // primary_formula --> ! primary_formula
  //                  |  Ex atomic_term primary_formula
  //                  |  Fa atomic_term primary_formula
  //                  |  Know k : primary_formula
  //                  |  Cons k : primary_formula
  //                  |  Bel k l : primary_formula => primary_formula
  //                  |  ( formula )
  //                  |  abbreviation
  //                  |  literal
  Result<Action<Formula::Ref>> primary_formula() {
    if (Is(Tok(), Token::kNot)) {
      Advance();
      Result<Action<Formula::Ref>> alpha = primary_formula();
      if (!alpha) {
        return Error<Action<Formula::Ref>>(LELA_MSG("Expected a primary formula within negation"), alpha);
      }
      return Success<Action<Formula::Ref>>([this, alpha_a = alpha.val]() {
        Result<Formula::Ref> alpha = alpha_a.Run();
        if (!alpha) {
          return Error<Formula::Ref>(LELA_MSG("Expected a primary formula within negation"), alpha);
        }
        return Success<Formula::Ref>(Formula::Factory::Not(std::move(alpha.val)));
      });
    }
    if (Is(Tok(), Token::kExists) || Is(Tok(), Token::kForall)) {
      bool ex = Is(Tok(), Token::kExists);
      Advance();
      Result<Action<Term>> x = atomic_term();
      if (!x) {
        return Error<Action<Formula::Ref>>(LELA_MSG("Expected variable in quantifier"), x);
      }
      Result<Action<Formula::Ref>> alpha = primary_formula();
      if (!alpha) {
        return Error<Action<Formula::Ref>>(LELA_MSG("Expected primary formula within quantifier"), alpha);
      }
      return Success<Action<Formula::Ref>>([this, ex, x_a = x.val, alpha_a = alpha.val]() {
        Result<Term> x = x_a.Run();
        if (!x.val.variable()) {
          return Error<Formula::Ref>(LELA_MSG("Expected variable in quantifier"), x);
        }
        Result<Formula::Ref> alpha = alpha_a.Run();
        if (!alpha) {
          return Error<Formula::Ref>(LELA_MSG("Expected primary formula within quantifier"), alpha);
        }
        return Success(ex ? Formula::Factory::Exists(x.val, std::move(alpha.val))
                          : Formula::Factory::Not(Formula::Factory::Exists(x.val,
                                                                           Formula::Factory::Not(std::move(alpha.val)))));
      });
    }
    if (Is(Tok(), Token::kKnow) || Is(Tok(), Token::kCons)) {
      bool know = Is(Tok(), Token::kKnow);
      Advance();
      if (!Is(Tok(), Token::kLess)) {
        return Error<Action<Formula::Ref>>(LELA_MSG("Expected '<'"));
      }
      Advance();
      if (!Is(Tok(), Token::kUint)) {
        return Error<Action<Formula::Ref>>(LELA_MSG("Expected split level integer"));
      }
      const Formula::split_level k = std::stoi(Tok().val.str());
      Advance();
      if (!Is(Tok(), Token::kGreater)) {
        return Error<Action<Formula::Ref>>(LELA_MSG("Expected '>'"));
      }
      Advance();
      Result<Action<Formula::Ref>> alpha = primary_formula();
      if (!alpha) {
        return Error<Action<Formula::Ref>>(LELA_MSG("Expected primary formula within modality"), alpha);
      }
      return Success<Action<Formula::Ref>>([this, know, k, alpha_a = alpha.val]() {
        Result<Formula::Ref> alpha = alpha_a.Run();
        if (!alpha) {
          return Error<Formula::Ref>(LELA_MSG("Expected primary formula within modality"), alpha);
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
        return Error<Action<Formula::Ref>>(LELA_MSG("Expected '<'"));
      }
      Advance();
      if (!Is(Tok(), Token::kUint)) {
        return Error<Action<Formula::Ref>>(LELA_MSG("Expected first split level integer"));
      }
      const Formula::split_level k = std::stoi(Tok().val.str());
      Advance();
      if (!Is(Tok(), Token::kComma)) {
        return Error<Action<Formula::Ref>>(LELA_MSG("Expected ','"));
      }
      Advance();
      if (!Is(Tok(), Token::kUint)) {
        return Error<Action<Formula::Ref>>(LELA_MSG("Expected second split level integer"));
      }
      const Formula::split_level l = std::stoi(Tok().val.str());
      Advance();
      if (!Is(Tok(), Token::kGreater)) {
        return Error<Action<Formula::Ref>>(LELA_MSG("Expected '>'"));
      }
      Advance();
      Result<Action<Formula::Ref>> alpha = primary_formula();
      if (!alpha) {
        return Error<Action<Formula::Ref>>(LELA_MSG("Expected primary formula within modality"), alpha);
      }
      if (!Is(Tok(), Token::kDoubleRArrow)) {
        return Error<Action<Formula::Ref>>(LELA_MSG("Expected conditional belief arrow"));
      }
      Advance();
      Result<Action<Formula::Ref>> beta = primary_formula();
      if (!beta) {
        return Error<Action<Formula::Ref>>(LELA_MSG("Expected primary formula within modality"), beta);
      }
      return Success<Action<Formula::Ref>>([this, k, l, alpha_a = alpha.val, beta_a = beta.val]() {
        Result<Formula::Ref> alpha = alpha_a.Run();
        if (!alpha) {
          return Error<Formula::Ref>(LELA_MSG("Expected primary formula within modality"), alpha);
        }
        Result<Formula::Ref> beta = beta_a.Run();
        if (!beta) {
          return Error<Formula::Ref>(LELA_MSG("Expected primary formula within modality"), beta);
        }
        return Success(Formula::Factory::Bel(k, l, std::move(alpha.val), std::move(beta.val)));
      });
    }
    if (Is(Tok(), Token::kLeftParen)) {
      Advance();
      Result<Action<Formula::Ref>> alpha = formula();
      if (!alpha) {
        return Error<Action<Formula::Ref>>(LELA_MSG("Expected formula within brackets"), alpha);
      }
      if (!Is(Tok(), Token::kRightParen)) {
        return Error<Action<Formula::Ref>>(LELA_MSG("Expected closing right parenthesis ')'"));
      }
      Advance();
      return Success<Action<Formula::Ref>>([this, alpha_a = alpha.val]() {
        Result<Formula::Ref> alpha = alpha_a.Run();
        if (!alpha) {
          return Error<Formula::Ref>(LELA_MSG("Expected formula within brackets"), alpha);
        }
        return Success(std::move(alpha.val));
      });
    }
    if (Is(Tok(), Token::kIdentifier) &&
        !(Is(Tok(1), Token::kLeftParen) || Is(Tok(1), Token::kEquality) || Is(Tok(1), Token::kInequality))) {
      std::string id = Tok().val.str();
      Advance();
      return Success<Action<Formula::Ref>>([this, id]() {
        if (!ctx_->IsRegisteredFormula(id)) {
          return Error<Formula::Ref>(LELA_MSG("Undefined formula abbreviation "+ id));
        }
        return Success(ctx_->LookupFormula(id).Clone());
      });
    }
    Result<Action<std::unique_ptr<Literal>>> a = literal();
    if (!a) {
      return Error<Action<Formula::Ref>>(LELA_MSG("Expected literal"), a);
    }
    return Success<Action<Formula::Ref>>([this, a_a = a.val]() {
      Result<std::unique_ptr<Literal>> a = a_a.Run();
      if (!a) {
        return Error<Formula::Ref>(LELA_MSG("Expected literal"), a);
      }
      return Success(Formula::Factory::Atomic(Clause{*a.val}));
    });
  }

  // conjunctive_formula --> primary_formula [ && primary_formula ]*
  Result<Action<Formula::Ref>> conjunctive_formula() {
    Result<Action<Formula::Ref>> alpha = primary_formula();
    if (!alpha) {
      return Error<Action<Formula::Ref>>(LELA_MSG("Expected left conjunctive formula"), alpha);
    }
    while (Is(Tok(), Token::kAnd)) {
      Advance();
      Result<Action<Formula::Ref>> beta = primary_formula();
      if (!beta) {
        return Error<Action<Formula::Ref>>(LELA_MSG("Expected left conjunctive formula"), beta);
      }
      alpha = Success<Action<Formula::Ref>>([alpha_a = alpha.val, beta_a = beta.val]() {
        Result<Formula::Ref> alpha = alpha_a.Run();
        if (!alpha) {
          return Error<Formula::Ref>(LELA_MSG("Expected left conjunctive formula"), alpha);
        }
        Result<Formula::Ref> beta = beta_a.Run();
        if (!beta) {
          return Error<Formula::Ref>(LELA_MSG("Expected right conjunctive formula"), beta);
        }
        return Success(Formula::Factory::Not(Formula::Factory::Or(Formula::Factory::Not(std::move(alpha.val)),
                                                                  Formula::Factory::Not(std::move(beta.val)))));
      });
    }
    return alpha;
  }

  // disjunctive_formula --> conjunctive_formula [ || conjunctive_formula ]*
  Result<Action<Formula::Ref>> disjunctive_formula() {
    Result<Action<Formula::Ref>> alpha = conjunctive_formula();
    if (!alpha) {
      return Error<Action<Formula::Ref>>(LELA_MSG("Expected left argument conjunctive formula"), alpha);
    }
    while (Is(Tok(), Token::kOr)) {
      Advance();
      Result<Action<Formula::Ref>> beta = conjunctive_formula();
      if (!beta) {
        return Error<Action<Formula::Ref>>(LELA_MSG("Expected right argument conjunctive formula"), beta);
      }
      alpha = Success<Action<Formula::Ref>>([alpha_a = alpha.val, beta_a = beta.val]() {
        Result<Formula::Ref> alpha = alpha_a.Run();
        if (!alpha) {
          return Error<Formula::Ref>(LELA_MSG("Expected left argument conjunctive formula"), alpha);
        }
        Result<Formula::Ref> beta = beta_a.Run();
        if (!beta) {
          return Error<Formula::Ref>(LELA_MSG("Expected right argument conjunctive formula"), beta);
        }
        return Success(Formula::Factory::Or(std::move(alpha.val), std::move(beta.val)));
      });
    }
    return alpha;
  }

  // implication_formula --> disjunctive_formula -> disjunctive_formula
  //                      |  disjunctive_formula
  Result<Action<Formula::Ref>> implication_formula() {
    Result<Action<Formula::Ref>> alpha = disjunctive_formula();
    if (!alpha) {
      return Error<Action<Formula::Ref>>(LELA_MSG("Expected left argument disjunctive formula"), alpha);
    }
    if (Is(Tok(), Token::kRArrow)) {
      Advance();
      Result<Action<Formula::Ref>> beta = disjunctive_formula();
      if (!beta) {
        return Error<Action<Formula::Ref>>(LELA_MSG("Expected right argument disjunctive formula"), beta);
      }
      alpha = Success<Action<Formula::Ref>>([this, alpha_a = alpha.val, beta_a = beta.val]() {
        Result<Formula::Ref> alpha = alpha_a.Run();
        if (!alpha) {
          return Error<Formula::Ref>(LELA_MSG("Expected left argument disjunctive formula"), alpha);
        }
        Result<Formula::Ref> beta = beta_a.Run();
        if (!beta) {
          return Error<Formula::Ref>(LELA_MSG("Expected right argument disjunctive formula"), beta);
        }
        return Success(Formula::Factory::Or(Formula::Factory::Not(std::move(alpha.val)), std::move(beta.val)));
      });
    }
    return alpha;
  }

  // equivalence_formula --> implication_formula -> implication_formula
  //                      |  implication_formula
  Result<Action<Formula::Ref>> equivalence_formula() {
    Result<Action<Formula::Ref>> alpha = implication_formula();
    if (!alpha) {
      return Error<Action<Formula::Ref>>(LELA_MSG("Expected left argument implication formula"), alpha);
    }
    if (Is(Tok(), Token::kLRArrow)) {
      Advance();
      Result<Action<Formula::Ref>> beta = implication_formula();
      if (!beta) {
        return Error<Action<Formula::Ref>>(LELA_MSG("Expected right argument implication formula"), beta);
      }
      alpha = Success<Action<Formula::Ref>>([alpha_a = alpha.val, beta_a = beta.val]() {
        Result<Formula::Ref> alpha = alpha_a.Run();
        if (!alpha) {
          return Error<Formula::Ref>(LELA_MSG("Expected left argument implication formula"), alpha);
        }
        Result<Formula::Ref> beta = beta_a.Run();
        if (!beta) {
          return Error<Formula::Ref>(LELA_MSG("Expected right argument implication formula"), beta);
        }
        Formula::Ref lr = Formula::Factory::Or(Formula::Factory::Not(alpha.val->Clone()), beta.val->Clone());
        Formula::Ref rl = Formula::Factory::Or(Formula::Factory::Not(std::move(alpha.val)), std::move(beta.val));
        return Success(Formula::Factory::Not(Formula::Factory::Or(Formula::Factory::Not(std::move(lr)),
                                                                  Formula::Factory::Not(std::move(rl)))));
      });
    }
    return alpha;
  }

  // formula --> equivalence_formula
  Result<Action<Formula::Ref>> formula() {
    return equivalence_formula();
  }

  // abbreviation --> let identifier := formula
  Result<Action<>> abbreviation() {
    if (!Is(Tok(), Token::kLet)) {
      return Unapplicable<Action<>>(LELA_MSG("Expected abbreviation operator 'let'"));
    }
    Advance();
    if (!Is(Tok(), Token::kIdentifier)) {
      return Error<Action<>>(LELA_MSG("Expected fresh identifier"));
    }
    const std::string id = Tok().val.str();
    Advance();
    if (!Is(Tok(), Token::kAssign)) {
      return Error<Action<>>(LELA_MSG("Expected assignment operator ':='"));
    }
    Advance();
    Result<Action<Formula::Ref>> alpha = formula();
    if (!alpha) {
      return Error<Action<>>(LELA_MSG("Expected formula"), alpha);
    }
    return Success<Action<>>([this, id, alpha_a = alpha.val]() {
      Result<Formula::Ref> alpha = alpha_a.Run();
      if (!alpha) {
        return Error<>(LELA_MSG("Expected formula"), alpha);
      }
      ctx_->RegisterFormula(id, *alpha.val);
      return Success<>();
    });
  }

  // kb_formula --> KB : formula
  Result<Action<>> kb_formula() {
    if (!Is(Tok(), Token::kKB)) {
      return Unapplicable<Action<>>(LELA_MSG("Expected 'KB'"));
    }
    Advance();
    if (!Is(Tok(), Token::kColon)) {
      return Unapplicable<Action<>>(LELA_MSG("Expected ':'"));
    }
    Advance();
    Result<Action<Formula::Ref>> alpha = formula();
    if (!alpha) {
      return Error<Action<>>(LELA_MSG("Expected KB formula"), alpha);
    }
    return Success<Action<>>([this, alpha_a = alpha.val]() {
      Result<Formula::Ref> alpha = alpha_a.Run();
      if (!alpha) {
        return Error<>(LELA_MSG("Expected KB formula"), alpha);
      }
      if (ctx_->AddToKb(*alpha.val)) {
        return Success<>();
      } else {
        return Error<>(LELA_MSG("Couldn't add formula to KB; is it proper+ "
                                "(i.e., its NF must be a universally quantified clause)?"));
      }
    });
  }

  // subjective_formula --> formula
  Result<Action<Formula::Ref>> subjective_formula() {
    Result<Action<Formula::Ref>> alpha = formula();
    if (!alpha) {
      return Error<Action<Formula::Ref>>(LELA_MSG("Expected subjective formula"), alpha);
    }
    return Success<Action<Formula::Ref>>([this, alpha_a = alpha.val]() {
      Result<Formula::Ref> alpha = alpha_a.Run();
      if (!alpha) {
        return Error<Formula::Ref>(LELA_MSG("Expected subjective formula"), alpha);
      }
      if (!alpha.val->subjective()) {
        return Error<Formula::Ref>(LELA_MSG("Expected subjective formula "
                                              "(i.e., no functions outside of modal operators; "
                                              "probably caused by missing brackets)"));
      }
      return Success<Formula::Ref>(std::move(alpha.val));
    });
  }

  // query --> [ Query | Refute | Assert ] : subjective_formula
  Result<Action<>> query() {
    if (!Is(Tok(), Token::kQuery) &&
        !Is(Tok(), Token::kAssert) &&
        !Is(Tok(), Token::kRefute)) {
      return Unapplicable<Action<>>(LELA_MSG("Expected 'Query', 'Assert', or 'Refute'"));
    }
    const bool is_query = Is(Tok(), Token::kQuery);
    const bool is_assert = Is(Tok(), Token::kAssert);
    Advance();
    if (!Is(Tok(), Token::kColon)) {
      return Error<Action<>>(LELA_MSG("Expected ':'"));
    }
    Advance();
    Result<Action<Formula::Ref>> alpha = subjective_formula();
    if (!alpha) {
      return Error<Action<>>(LELA_MSG("Expected query/assertion/refutation subjective_formula"), alpha);
    }
    return Success<Action<>>([this, alpha_a = alpha.val, is_query, is_assert]() {
      Result<Formula::Ref> alpha = alpha_a.Run();
      if (!alpha) {
        return Error<>(LELA_MSG("Expected query/assertion/refutation subjective_formula"), alpha);
      }
      const bool r = ctx_->Query(*alpha.val);
      if (is_query) {
        return Success<>();
      } else if (r == is_assert) {
        return Success<>();
      } else {
        return Error<>(LELA_MSG("Assertion/refutation failed"));
      }
    });
  }

#if 0
  // if_conditional --> If formula block
  Result<bool> if_conditional() {
    if (!Is(Tok(), Token::kIf)) {
      return Unapplicable<bool>(LELA_MSG("Expected 'Query', 'Assert', or 'Refute'"));
    }
    Advance();
    Result<Formula::Ref> alpha = formula();
    if (!alpha) {
      return Error<bool>(LELA_MSG("Expected formula in if_conditional"), alpha);
    }
    Result<bool> r = block();
    if (!r) {
      return Error<bool>(LELA_MSG("Expected block in if_conditional"), r);
    }
    return Success(true);
  }

  // block --> Begin branch* End
  Result<Action<>> block() {
    if (!Is(Tok(), Token::kBegin)) {
      Result<bool> r = branch();
      if (!r) {
        return Error<bool>(LELA_MSG("Expected branch in block"), r);
      }
      return r;
    } else {
      Advance();
      const size_t n_blocks = n_blocks_;
      ++n_blocks_;
      while (n_blocks_ > n_blocks) {
        if (Is(Tok(), Token::kEnd)) {
          Advance();
          --n_blocks_;
        } else {
          Result<bool> r = branch();
          if (!r) {
            return Error<bool>(LELA_MSG("Expected branch in block"), r);
          }
        }
      }
      return Success(true);
    }
  }
#endif

  // branch --> [ declarations | kb_formula | abbreviation | query ]
  Result<Action<>> branch() {
    typedef Result<Action<>> (Parser::*Rule)();
    std::list<Rule> rules = {&Parser::declaration, &Parser::kb_formula, &Parser::abbreviation, &Parser::query};
                             //&Parser::if_conditional/*, &Parser::while_loop, &Parser::for_loop, &Parser::call*/};
    for (Rule rule : rules) {
      Result<Action<>> r = (this->*rule)();
      if (r) {
        return r;
      } else if (r.applied()) {
        return Error<Action<>>(LELA_MSG("Error in branch"), r);
      }
    }
    return Unapplicable<Action<>>(LELA_MSG("No rule applicable in branch"));
  }

  // start --> branch
  Result<Action<>> start() {
    iterator prev;
    Action<> a;
    do {
      const Result<Action<>> r = branch();
      if (!r) {
        std::stringstream ss;
        using lela::format::output::operator<<;
        ss << Tok(0) << " " << Tok(1) << " " << Tok(2) << "...";
        return Error<Action<>>(LELA_MSG("Error in start with unparsed input "+ ss.str()), r);
      }
      a += r.val;
    } while (Tok());
    return Success<Action<>>(std::move(a));
  }

  bool Is(const lela::internal::Maybe<Token>& symbol, Token::Id id) const {
    return symbol && symbol.val.id() == id;
  }

  template<typename UnaryPredicate>
  bool Is(const lela::internal::Maybe<Token>& symbol, Token::Id id, UnaryPredicate p) const {
    return symbol && symbol.val.id() == id && p(symbol.val.str());
  }

  lela::internal::Maybe<Token> Tok(size_t n = 0) const {
    auto it = begin();
    for (size_t i = 0; i < n && it != end_; ++i) {
      assert(begin() != end());
      ++it;
    }
    return it != end_ ? lela::internal::Just(*it) : lela::internal::Nothing;
  }

  void Advance(size_t n = 1) {
    begin_plus_ += n;
  }
#endif

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

  static const std::string kUnapplicableLabel;
  static const std::string kErrorLabel;
  static const std::string kCausesLabel;

  Lex lexer_;
  mutable iterator begin_;  // don't use begin_ directly: to avoid the stream blocking us, Advance() actually increments
  mutable size_t begin_plus_ = 0;  // begin_plus_ instead of begin_; use begin() to obtain the incremented iterator.
  iterator end_;
  size_t n_blocks_ = 0;
  Context<LogPredicate>* ctx_;
  LogPredicate log_;
};


template<typename ForwardIt, typename LogPredicate>
const std::string Parser<ForwardIt, LogPredicate>::kUnapplicableLabel = std::string("Unappl.: ");

template<typename ForwardIt, typename LogPredicate>
const std::string Parser<ForwardIt, LogPredicate>::kErrorLabel        = std::string("Failure: ");

template<typename ForwardIt, typename LogPredicate>
const std::string Parser<ForwardIt, LogPredicate>::kCausesLabel       = std::string(" causes: ");

}  // namespace pdl
}  // namespace format
}  // namespace lela

#endif  // LELA_FORMAT_PDL_PARSER_H_

