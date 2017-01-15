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

#include <iostream>
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

  // Encapsulates a parsing result, either a Success, an Unapplicable, or a Failure.
  template<typename T>
  struct Result {
    Result() = default;
    Result(Result&&) = default;
    Result& operator=(Result&&) = default;

    explicit Result(T&& val) : ok(true), val(std::forward<T>(val)) {}

    Result(bool unapplicable, const std::string& msg, ForwardIt begin, ForwardIt end)
        : ok(false), unapplicable(unapplicable), msg(msg), begin_(begin), end_(end) {}

    Result(bool unapplicable, const std::string& msg, ForwardIt begin, ForwardIt end, T&& val)
        : ok(false), val(val), unapplicable(unapplicable), msg(msg), begin_(begin), end_(end) {}

    explicit operator bool() const { return ok; }

    ForwardIt begin() const { return begin_; }
    ForwardIt end()   const { return end_; }

    std::string str() const {
      std::stringstream ss;
      if (ok) {
        ss << "Success: " << val;
      } else {
        ss << msg << std::endl;
        ss << "with remaining input: \"" << std::string(begin(), end()) << "\"";
      }
      return ss.str();
    }

    std::string remaining_input() const { return std::string(begin(), end()); }

    bool ok;
    T val;
    bool unapplicable;
    std::string msg;

   private:
    ForwardIt begin_;
    ForwardIt end_;
  };

  Parser(ForwardIt begin, ForwardIt end, Context<LogPredicate>* ctx)
      : lexer_(begin, end), begin_(lexer_.begin()), end_(lexer_.end()), ctx_(ctx) {}

  Result<bool> Parse() { return start(); }

  Context<LogPredicate>& ctx() { return *ctx_; }
  const Context<LogPredicate>& ctx() const { return *ctx_; }

 private:
  static_assert(std::is_convertible<typename ForwardIt::iterator_category, std::forward_iterator_tag>::value,
                "ForwardIt has wrong iterator category");

  template<typename T>
  Result<T> Success(T&& result) const { return Result<T>(std::forward<T>(result)); }

  template<typename T>
  Result<T> Failure(const std::string& msg) const {
    std::stringstream ss;
    ss << kFailureLabel << msg;
    return Result<T>(false, ss.str(), begin().char_iter(), end().char_iter());
  }

  template<typename T>
  Result<T> Failure(const std::string& msg, T&& val) const {
    std::stringstream ss;
    ss << kFailureLabel << msg;
    return Result<T>(false, ss.str(), begin().char_iter(), end().char_iter(), val);
  }

  template<typename T, typename U>
  static Result<T> Failure(const std::string& msg, const Result<U>& r) {
    std::stringstream ss;
    ss << r.msg << std::endl << kCausesLabel << msg;
    return Result<T>(false, ss.str(), r.begin(), r.end());
  }

  template<typename T>
  Result<T> Unapplicable(const std::string& msg) const {
    std::stringstream ss;
    ss << kUnapplicableLabel << msg;
    return Result<T>(true, "\t" + msg, begin().char_iter(), end().char_iter());
  }

  // declaration --> sort <sort-id> [ , <sort-id>]*
  //              |  var <id> [ , <id> ]* -> <sort-id>
  //              |  name <id> [ , <id> ]* -> <sort-id>
  //              |  fun <id> [ , <id> ]* / <arity> -> <sort-id>
  Result<bool> declaration() {
    if (Is(Tok(), Token::kSort)) {
      do {
        Advance();
        if (Is(Tok(), Token::kIdentifier, [this](const std::string& s) { return !ctx_->IsRegisteredSort(s); })) {
          ctx_->RegisterSort(Tok().val.str());
          Advance();
        } else {
          return Failure<bool>(LELA_MSG("Expected sort identifier"));
        }
      } while (Is(Tok(0), Token::kComma));
      return Success<bool>(true);
    }
    if (Is(Tok(), Token::kVar) || Is(Tok(), Token::kName)) {
      const bool var = Is(Tok(), Token::kVar);
      std::vector<std::string> ids;
      do {
        Advance();
        if (Is(Tok(), Token::kIdentifier, [this](const std::string& s) { return !ctx_->IsRegisteredTerm(s); })) {
          ids.push_back(Tok().val.str());
          Advance();
        } else {
          return Failure<bool>(LELA_MSG(var ? "Expected variable identifier" : "Expected name identifier"));
        }
      } while (Is(Tok(0), Token::kComma));
      if (Is(Tok(0), Token::kRArrow) &&
          Is(Tok(1), Token::kIdentifier, [this](const std::string& s) { return ctx_->IsRegisteredSort(s); })) {
        const std::string sort = Tok(1).val.str();
        for (const std::string& id : ids) {
          var ? ctx_->RegisterVariable(id, sort) : ctx_->RegisterName(id, sort);
        }
        Advance(2);
      } else {
        return Failure<bool>(LELA_MSG("Expected arrow and sort identifier"));
      }
      return Success<bool>(true);
    }
    if (Is(Tok(), Token::kFun)) {
      std::vector<std::pair<std::string, Symbol::Arity>> ids;
      do {
        Advance();
        if (Is(Tok(0), Token::kIdentifier, [this](const std::string& s) { return !ctx_->IsRegisteredTerm(s); }) &&
            Is(Tok(1), Token::kSlash) &&
            Is(Tok(2), Token::kUint)) {
          ids.push_back(std::make_pair(Tok(0).val.str(), std::stoi(Tok(2).val.str())));
          Advance(3);
        } else {
          return Failure<bool>(LELA_MSG("Expected function identifier"));
        }
      } while (Is(Tok(0), Token::kComma));
      if (Is(Tok(0), Token::kRArrow) &&
          Is(Tok(1), Token::kIdentifier, [this](const std::string& s) { return ctx_->IsRegisteredSort(s); })) {
        const std::string sort = Tok(1).val.str();
        for (const auto& id_arity : ids) {
          ctx_->RegisterFunction(id_arity.first, id_arity.second, sort);
        }
        Advance(2);
      } else {
        return Failure<bool>(LELA_MSG("Expected arrow and sort identifier"));
      }
      return Success<bool>(true);
    }
    return Unapplicable<bool>(LELA_MSG("Expected 'Sort', 'Var', 'Name' or 'Fun'"));
  }

  // term --> x
  //       |  n
  //       |  f
  //       |  f(term, ..., term)
  Result<Term> term() {
    if (Is(Tok(0), Token::kIdentifier, [this](const std::string& s) { return ctx_->IsRegisteredVariable(s); })) {
      Term x = ctx_->LookupVariable(Tok(0).val.str());
      Advance();
      return Success(Term(x));
    }
    if (Is(Tok(0), Token::kIdentifier, [this](const std::string& s) { return ctx_->IsRegisteredName(s); })) {
      Term n = ctx_->LookupName(Tok(0).val.str());
      Advance();
      return Success(Term(n));
    }
    if (Is(Tok(0), Token::kIdentifier, [this](const std::string& s) { return ctx_->IsRegisteredFunction(s); })) {
      class Symbol s = ctx_->LookupFunction(Tok(0).val.str());
      Advance();
      Term::Vector args;
      if (s.arity() > 0 || Is(Tok(0), Token::kLeftParen)) {
        if (!Is(Tok(0), Token::kLeftParen)) {
          return Failure<Term>(LELA_MSG("Expected left parenthesis '('"));
        }
        Advance();
        for (Symbol::Arity i = 0; i < s.arity(); ++i) {
          if (i > 0) {
            if (!Is(Tok(0), Token::kComma)) {
              return Failure<Term>(LELA_MSG("Expected comma ','"));
            }
            Advance();
          }
          Result<Term> t = term();
          if (!t) {
            return Failure<Term>(LELA_MSG("Expected argument term"), t);
          }
          args.push_back(t.val);
        }
        if (!Is(Tok(0), Token::kRightParen)) {
          return Failure<Term>(LELA_MSG("Expected right parenthesis ')'"));
        }
        Advance();
      }
      return Success(ctx_->tf()->CreateTerm(s, args));
    }
    return Failure<Term>(LELA_MSG("Expected a declared variable/name/function identifier"));
  }

  // literal --> term [ '==' | '!=' ] term
  Result<std::unique_ptr<Literal>> literal() {
    Result<Term> lhs = term();
    if (!lhs) {
      return Failure<std::unique_ptr<Literal>>(LELA_MSG("Expected a lhs term"), lhs);
    }
    bool pos;
    if (Is(Tok(0), Token::kEquality) ||
        Is(Tok(0), Token::kInequality)) {
      pos = Is(Tok(0), Token::kEquality);
      Advance();
    } else {
      return Failure<std::unique_ptr<Literal>>(LELA_MSG("Expected equality or inequality '=='/'!='"));
    }
    Result<Term> rhs = term();
    if (!rhs) {
      return Failure<std::unique_ptr<Literal>>(LELA_MSG("Expected rhs term"), rhs);
    }
    Literal a = pos ? Literal::Eq(lhs.val, rhs.val) : Literal::Neq(lhs.val, rhs.val);
    return Success(std::unique_ptr<Literal>(new Literal(a)));
  }

  // primary_formula --> ! primary_formula
  //                  |  Ex x primary_formula
  //                  |  Fa x primary_formula
  //                  |  Know k : primary_formula
  //                  |  Cons k : primary_formula
  //                  |  Bel k l : primary_formula => primary_formula
  //                  |  ( formula )
  //                  |  abbreviation
  //                  |  literal
  Result<Formula::Ref> primary_formula(int binary_connective_recursion = 0) {
    if (Is(Tok(0), Token::kNot)) {
      Advance();
      Result<Formula::Ref> alpha = primary_formula();
      if (!alpha) {
        return Failure<Formula::Ref>(LELA_MSG("Expected a primary formula within negation"), alpha);
      }
      return Success(Formula::Factory::Not(std::move(alpha.val)));
    }
    if (Is(Tok(0), Token::kExists) || Is(Tok(0), Token::kForall)) {
      bool ex = Is(Tok(0), Token::kExists);
      Advance();
      Result<Term> x = term();
      if (!x) {
        return Failure<Formula::Ref>(LELA_MSG("Expected variable in quantifier"), x);
      }
      if (!x.val.variable()) {
        return Failure<Formula::Ref>(LELA_MSG("Expected variable in quantifier"), x);
      }
      Result<Formula::Ref> alpha = primary_formula();
      if (!alpha) {
        return Failure<Formula::Ref>(LELA_MSG("Expected primary formula within quantifier"), alpha);
      }
      return Success(ex ? Formula::Factory::Exists(x.val, std::move(alpha.val))
                        : Formula::Factory::Not(Formula::Factory::Exists(x.val,
                                                                         Formula::Factory::Not(std::move(alpha.val)))));
    }
    if (Is(Tok(0), Token::kKnow) || Is(Tok(0), Token::kCons)) {
      bool know = Is(Tok(0), Token::kKnow);
      Advance();
      if (!Is(Tok(0), Token::kLess)) {
        return Failure<Formula::Ref>(LELA_MSG("Expected '<'"));
      }
      Advance();
      if (!Is(Tok(0), Token::kUint)) {
        return Failure<Formula::Ref>(LELA_MSG("Expected split level integer"));
      }
      const Formula::split_level k = std::stoi(Tok(0).val.str());
      Advance();
      if (!Is(Tok(0), Token::kGreater)) {
        return Failure<Formula::Ref>(LELA_MSG("Expected '>'"));
      }
      Advance();
      Result<Formula::Ref> alpha = primary_formula();
      if (!alpha) {
        return Failure<Formula::Ref>(LELA_MSG("Expected primary formula within modality"), alpha);
      }
      if (know) {
        return Success(Formula::Factory::Know(k, std::move(alpha.val)));
      } else {
        return Success(Formula::Factory::Cons(k, std::move(alpha.val)));
      }
    }
    if (Is(Tok(0), Token::kBel)) {
      Advance();
      if (!Is(Tok(0), Token::kLess)) {
        return Failure<Formula::Ref>(LELA_MSG("Expected '<'"));
      }
      Advance();
      if (!Is(Tok(0), Token::kUint)) {
        return Failure<Formula::Ref>(LELA_MSG("Expected first split level integer"));
      }
      const Formula::split_level k = std::stoi(Tok(0).val.str());
      Advance();
      if (!Is(Tok(0), Token::kComma)) {
        return Failure<Formula::Ref>(LELA_MSG("Expected ','"));
      }
      Advance();
      if (!Is(Tok(0), Token::kUint)) {
        return Failure<Formula::Ref>(LELA_MSG("Expected second split level integer"));
      }
      const Formula::split_level l = std::stoi(Tok(0).val.str());
      Advance();
      if (!Is(Tok(0), Token::kGreater)) {
        return Failure<Formula::Ref>(LELA_MSG("Expected '>'"));
      }
      Advance();
      Result<Formula::Ref> alpha = primary_formula();
      if (!alpha) {
        return Failure<Formula::Ref>(LELA_MSG("Expected primary formula within modality"), alpha);
      }
      if (!Is(Tok(0), Token::kDoubleRArrow)) {
        return Failure<Formula::Ref>(LELA_MSG("Expected conditional belief arrow"));
      }
      Advance();
      Result<Formula::Ref> beta = primary_formula();
      if (!beta) {
        return Failure<Formula::Ref>(LELA_MSG("Expected primary formula within modality"), beta);
      }
      return Success(Formula::Factory::Bel(k, l, std::move(alpha.val), std::move(beta.val)));
    }
    if (Is(Tok(0), Token::kLeftParen)) {
      Advance();
      Result<Formula::Ref> alpha = formula();
      if (!alpha) {
        return Failure<Formula::Ref>(LELA_MSG("Expected formula within brackets"), alpha);
      }
      if (!Is(Tok(0), Token::kRightParen)) {
        return Failure<Formula::Ref>(LELA_MSG("Expected closing right parenthesis ')'"));
      }
      Advance();
      return alpha;
    }
    if (Is(Tok(0), Token::kIdentifier) && ctx_->IsRegisteredFormula(Tok(0).val.str())) {
      std::string id = Tok(0).val.str();
      Advance();
      return Success(ctx_->LookupFormula(id).Clone());
    }
    Result<std::unique_ptr<Literal>> a = literal();
    if (!a) {
      return Failure<Formula::Ref>(LELA_MSG("Expected literal"), a);
    }
    return Success(Formula::Factory::Atomic(Clause{*a.val}));
  }

  // conjunctive_formula --> primary_formula [ && primary_formula ]*
  Result<Formula::Ref> conjunctive_formula() {
    Result<Formula::Ref> alpha = primary_formula();
    if (!alpha) {
      return Failure<Formula::Ref>(LELA_MSG("Expected left conjunctive formula"), alpha);
    }
    while (Is(Tok(0), Token::kAnd)) {
      Advance();
      Result<Formula::Ref> psi = primary_formula();
      if (!psi) {
        return Failure<Formula::Ref>(LELA_MSG("Expected left conjunctive formula"), psi);
      }
      alpha = Success(Formula::Factory::Not(Formula::Factory::Or(Formula::Factory::Not(std::move(alpha.val)),
                                                                 Formula::Factory::Not(std::move(psi.val)))));
    }
    return alpha;
  }

  // disjunctive_formula --> conjunctive_formula [ || conjunctive_formula ]*
  Result<Formula::Ref> disjunctive_formula() {
    Result<Formula::Ref> alpha = conjunctive_formula();
    if (!alpha) {
      return Failure<Formula::Ref>(LELA_MSG("Expected left argument conjunctive formula"), alpha);
    }
    while (Is(Tok(0), Token::kOr)) {
      Advance();
      Result<Formula::Ref> psi = conjunctive_formula();
      if (!psi) {
        return Failure<Formula::Ref>(LELA_MSG("Expected right argument conjunctive formula"), psi);
      }
      alpha = Success(Formula::Factory::Or(std::move(alpha.val), std::move(psi.val)));
    }
    return alpha;
  }

  // implication_formula --> disjunctive_formula -> disjunctive_formula
  //                      |  disjunctive_formula
  Result<Formula::Ref> implication_formula() {
    Result<Formula::Ref> alpha = disjunctive_formula();
    if (!alpha) {
      return Failure<Formula::Ref>(LELA_MSG("Expected left argument disjunctive formula"), alpha);
    }
    if (Is(Tok(0), Token::kRArrow)) {
      Advance();
      Result<Formula::Ref> psi = disjunctive_formula();
      if (!psi) {
        return Failure<Formula::Ref>(LELA_MSG("Expected right argument disjunctive formula"), psi);
      }
      alpha = Success(Formula::Factory::Or(Formula::Factory::Not(std::move(alpha.val)), std::move(psi.val)));
    }
    return alpha;
  }

  // equivalence_formula --> implication_formula -> implication_formula
  //                      |  implication_formula
  Result<Formula::Ref> equivalence_formula() {
    Result<Formula::Ref> alpha = implication_formula();
    if (!alpha) {
      return Failure<Formula::Ref>(LELA_MSG("Expected left argument implication formula"), alpha);
    }
    if (Is(Tok(0), Token::kLRArrow)) {
      Advance();
      Result<Formula::Ref> psi = implication_formula();
      if (!psi) {
        return Failure<Formula::Ref>(LELA_MSG("Expected right argument implication formula"), psi);
      }
      Formula::Ref lr = Formula::Factory::Or(Formula::Factory::Not(alpha.val->Clone()), psi.val->Clone());
      Formula::Ref rl = Formula::Factory::Or(Formula::Factory::Not(std::move(alpha.val)), std::move(psi.val));
      alpha = Success(Formula::Factory::Not(Formula::Factory::Or(Formula::Factory::Not(std::move(lr)),
                                                                 Formula::Factory::Not(std::move(rl)))));
    }
    return alpha;
  }

  // formula --> equivalence_formula
  Result<Formula::Ref> formula() {
    return equivalence_formula();
  }

  // abbreviation --> let identifier := formula
  Result<bool> abbreviation() {
    if (!Is(Tok(0), Token::kLet)) {
      return Unapplicable<bool>(LELA_MSG("Expected abbreviation operator 'let'"));
    }
    Advance();
    if (!Is(Tok(0), Token::kIdentifier)) {
      return Failure<bool>(LELA_MSG("Expected fresh identifier"));
    }
    const std::string id = Tok(0).val.str();
    Advance();
    if (!Is(Tok(0), Token::kAssign)) {
      return Failure<bool>(LELA_MSG("Expected assignment operator ':='"));
    }
    Advance();
    const Result<Formula::Ref> alpha = formula();
    if (!alpha) {
      return Failure<bool>(LELA_MSG("Expected formula"), alpha);
    }
    ctx_->RegisterFormula(id, *alpha.val);
    return Success(true);
  }

  // kb_formula --> KB : formula
  Result<bool> kb_formula() {
    if (!Is(Tok(0), Token::kKB)) {
      return Unapplicable<bool>(LELA_MSG("Expected 'KB'"));
    }
    Advance();
    if (!Is(Tok(0), Token::kColon)) {
      return Unapplicable<bool>(LELA_MSG("Expected ':'"));
    }
    Advance();
    Result<Formula::Ref> alpha = formula();
    if (!alpha) {
      return Failure<bool>(LELA_MSG("Expected KB formula"), alpha);
    }
    if (ctx_->AddToKb(*alpha.val)) {
      return Success(true);
    } else {
      return Failure<bool>(LELA_MSG("Couldn't add formula to KB; is it proper+ "
                                    "(i.e., its NF must be a universally quantified clause)?"));
    }
  }

  // subjective_formula --> formula
  Result<Formula::Ref> subjective_formula() {
    Result<Formula::Ref> alpha = formula();
    if (!alpha) {
      return Failure<Formula::Ref>(LELA_MSG("Expected subjective formula"), alpha);
    }
    if (!alpha.val->subjective()) {
      return Failure<Formula::Ref>(LELA_MSG("Expected subjective formula "
                                            "(i.e., no functions outside of modal operators; "
                                            "probably caused by missing brackets)"));
    }
    return Success(std::move(alpha.val));
  }

  // query --> [ Query | Refute | Assert ] : subjective_formula
  Result<bool> query() {
    if (!Is(Tok(0), Token::kQuery) &&
        !Is(Tok(0), Token::kAssert) &&
        !Is(Tok(0), Token::kRefute)) {
      return Unapplicable<bool>(LELA_MSG("Expected 'Query', 'Assert', or 'Refute'"));
    }
    const bool is_query = Is(Tok(0), Token::kQuery);
    const bool is_assert = Is(Tok(0), Token::kAssert);
    Advance();
    if (!Is(Tok(0), Token::kColon)) {
      return Failure<bool>(LELA_MSG("Expected ':'"));
    }
    Advance();
    Result<Formula::Ref> alpha = subjective_formula();
    if (!alpha) {
      return Failure<bool>(LELA_MSG("Expected query/assertion/refutation subjective_formula"), alpha);
    }
    const bool r = ctx_->Query(*alpha.val);
    if (is_query) {
      return Success(bool(r));
    } else if (r == is_assert) {
      return Success(true);
    } else {
      return Failure<bool>(LELA_MSG("Assertion/refutation failed"));
    }
  }

  // if_conditional --> If formula block
  Result<bool> if_conditional() {
    if (!Is(Tok(0), Token::kIf)) {
      return Unapplicable<bool>(LELA_MSG("Expected 'Query', 'Assert', or 'Refute'"));
    }
    Advance();
    Result<Formula::Ref> alpha = formula();
    if (!alpha) {
      return Failure<bool>(LELA_MSG("Expected formula in if_conditional"), alpha);
    }
    Result<bool> r = block();
    if (!r) {
      return Failure<bool>(LELA_MSG("Expected block in if_conditional"), r);
    }
    return Success(true);
  }

  // block --> Begin branch* End
  Result<bool> block() {
    if (!Is(Tok(), Token::kBegin)) {
      Result<bool> r = branch();
      if (!r) {
        return Failure<bool>(LELA_MSG("Expected branch in block"), r);
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
            return Failure<bool>(LELA_MSG("Expected branch in block"), r);
          }
        }
      }
      return Success(true);
    }
  }

  // branch --> [ declarations | kb_formula | abbreviation | query ]
  Result<bool> branch() {
    typedef Result<bool> (Parser::*Rule)();
    std::vector<Rule> rules = {&Parser::declaration, &Parser::kb_formula, &Parser::abbreviation, &Parser::query,
                               &Parser::if_conditional/*, &Parser::while_loop, &Parser::for_loop, &Parser::call*/};
    for (Rule rule : rules) {
      const Result<bool> r = (this->*rule)();
      if (r) {
        break;
      } else if (!r.unapplicable) {
        return Failure<bool>(LELA_MSG("Error in declaration/kb_formula/abbreviation/query"), r);
      }
    }
    return Success(true);
  }

  // start --> branch
  Result<bool> start() {
    iterator prev;
    do {
      prev = begin();
      const Result<bool> r = branch();
      if (!r) {
        return Failure<bool>(LELA_MSG("Error in start"), r);
      }
    } while (begin() != prev);
    std::stringstream ss;
    using lela::format::output::operator<<;
    ss << Tok(0) << " " << Tok(1) << " " << Tok(2);
    return !Tok(0) ? Success(true) : Failure<bool>(LELA_MSG("Unparsed input "+ ss.str()));
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
  static const std::string kFailureLabel;
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
const std::string Parser<ForwardIt, LogPredicate>::kFailureLabel = std::string("Failure: ");

template<typename ForwardIt, typename LogPredicate>
const std::string Parser<ForwardIt, LogPredicate>::kCausesLabel = std::string(" causes: ");

}  // namespace pdl
}  // namespace format
}  // namespace lela

#endif  // LELA_FORMAT_PDL_PARSER_H_

