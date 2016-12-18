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
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
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
#define LELA_MSG(msg)      (std::string(msg) +" (at "+ __FUNCTION__ +"():"+ LELA_STR__LINE__ +")")

namespace {
  static const size_t kErrorMsgWidth = std::max(std::max(std::string("Unapplicable: ").length(),
                                                         std::string("Failure: ").length()),
                                                         std::string("because ").length());
}

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

    std::string to_string() const {
      std::stringstream ss;
      if (ok) {
        ss << "Success: " << val;
      } else {
        ss << std::setw(kErrorMsgWidth) << std::left << (unapplicable ? "Unapplicable:" : "Failure:");
        ss << msg << std::endl;
        ss << "At rest of input: \"" << std::string(begin(), end()) << "\"";
      }
      return ss.str();
    }

    bool ok;
    T val;
    bool unapplicable;
    std::string msg;

   private:
    template<typename U>
    friend std::ostream& operator<<(std::ostream& os, const Result<U>& r) {
      return os << r.to_string();
    }

    ForwardIt begin_;
    ForwardIt end_;
  };

  Parser(ForwardIt begin, ForwardIt end, Context<LogPredicate>* ctx)
      : lexer_(begin, end), begin_(lexer_.begin()), end_(lexer_.end()), ctx_(ctx) {}

  Result<bool> Parse() { return start(); }

  Context<LogPredicate>& ctx() { return *ctx_; }
  const Context<LogPredicate>& ctx() const { return *ctx_; }

 private:
  template<typename T>
  Result<T> Success(T&& result) const { return Result<T>(std::forward<T>(result)); }

  template<typename T>
  Result<T> Failure(const std::string& msg) const {
    return Result<T>(false, msg, begin().char_iter(), end().char_iter());
  }

  template<typename T>
  Result<T> Failure(const std::string& msg, T&& val) const {
    return Result<T>(false, msg, begin().char_iter(), end().char_iter(), val);
  }

  template<typename T, typename U>
  static Result<T> Failure(const std::string& msg, const Result<U>& r) {
    std::stringstream ss;
    ss << msg << std::endl << std::setw(kErrorMsgWidth) << std::left << "because:" << r.msg;
    return Result<T>(false, ss.str(), r.begin(), r.end());
  }

  template<typename T>
  Result<T> Unapplicable(const std::string& msg) const {
    return Result<T>(true, "\t" + msg, begin().char_iter(), end().char_iter());
  }

  // declaration --> sort <sort-id> ;
  //              |  var <id> -> <sort-id> ;
  //              |  name <id> -> <sort-id> ;
  //              |  fun <id> / <arity> -> <sort-id> ;
  Result<bool> declaration() {
    if (!Is(Token(0), Token::kSort) &&
        !Is(Token(0), Token::kVar) &&
        !Is(Token(0), Token::kName) &&
        !Is(Token(0), Token::kFun)) {
      return Unapplicable<bool>(LELA_MSG("No declaration"));
    }
    if (Is(Token(0), Token::kSort) &&
        Is(Token(1), Token::kIdentifier, [this](const std::string& s) { return !ctx_->IsRegisteredSort(s); }) &&
        Is(Token(2), Token::kEndOfLine)) {
      ctx_->RegisterSort(Token(1).val.str());
      Advance(2);
      return Success(true);
    }
    if (Is(Token(0), Token::kVar) &&
        Is(Token(1), Token::kIdentifier, [this](const std::string& s) { return !ctx_->IsRegisteredTerm(s); }) &&
        Is(Token(2), Token::kRArrow) &&
        Is(Token(3), Token::kIdentifier, [this](const std::string& s) { return ctx_->IsRegisteredSort(s); }) &&
        Is(Token(4), Token::kEndOfLine)) {
      ctx_->RegisterVariable(Token(1).val.str(), Token(3).val.str());
      Advance(4);
      return Success(true);
    }
    if (Is(Token(0), Token::kName) &&
        Is(Token(1), Token::kIdentifier, [this](const std::string& s) { return !ctx_->IsRegisteredTerm(s); }) &&
        Is(Token(2), Token::kRArrow) &&
        Is(Token(3), Token::kIdentifier, [this](const std::string& s) { return ctx_->IsRegisteredSort(s); }) &&
        Is(Token(4), Token::kEndOfLine)) {
      ctx_->RegisterName(Token(1).val.str(), Token(3).val.str());
      Advance(4);
      return Success(true);
    }
    if (Is(Token(0), Token::kFun) &&
        Is(Token(1), Token::kIdentifier, [this](const std::string& s) { return !ctx_->IsRegisteredTerm(s); }) &&
        Is(Token(2), Token::kSlash) &&
        Is(Token(3), Token::kUint) &&
        Is(Token(4), Token::kRArrow) &&
        Is(Token(5), Token::kIdentifier, [this](const std::string& s) { return ctx_->IsRegisteredSort(s); }) &&
        Is(Token(6), Token::kEndOfLine)) {
      ctx_->RegisterFunction(Token(1).val.str(), std::stoi(Token(3).val.str()), Token(5).val.str());
      Advance(6);
      return Success(true);
    }
    return Failure<bool>(LELA_MSG("Invalid sort/var/name/fun declaration"));
  }

  // declarations --> declaration*
  Result<bool> declarations() {
    Result<bool> r;
    while ((r = declaration())) {
    }
    if (!r.unapplicable) {
      return r;
    } else {
      return Success(true);
    }
  }

  // term --> x
  //       |  n
  //       |  f
  //       |  f(term, ..., term)
  Result<Term> term() {
    if (Is(Token(0), Token::kIdentifier, [this](const std::string& s) { return ctx_->IsRegisteredVariable(s); })) {
      Term x = ctx_->LookupVariable(Token(0).val.str());
      Advance(0);
      return Success(Term(x));
    }
    if (Is(Token(0), Token::kIdentifier, [this](const std::string& s) { return ctx_->IsRegisteredName(s); })) {
      Term n = ctx_->LookupName(Token(0).val.str());
      Advance(0);
      return Success(Term(n));
    }
    if (Is(Token(0), Token::kIdentifier, [this](const std::string& s) { return ctx_->IsRegisteredFunction(s); })) {
      class Symbol s = ctx_->LookupFunction(Token(0).val.str());
      Advance(0);
      Term::Vector args;
      if (s.arity() > 0 || Is(Token(0), Token::kLeftParen)) {
        if (!Is(Token(0), Token::kLeftParen)) {
          return Failure<Term>(LELA_MSG("Expected left parenthesis '('"));
        }
        Advance(0);
        for (Symbol::Arity i = 0; i < s.arity(); ++i) {
          if (i > 0) {
            if (!Is(Token(0), Token::kComma)) {
              return Failure<Term>(LELA_MSG("Expected comma ','"));
            }
            Advance(0);
          }
          Result<Term> t = term();
          if (!t) {
            return Failure<Term>(LELA_MSG("Expected argument term"), t);
          }
          args.push_back(t.val);
        }
        if (!Is(Token(0), Token::kRightParen)) {
          return Failure<Term>(LELA_MSG("Expected right parenthesis ')'"));
        }
        Advance(0);
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
    if (Is(Token(0), Token::kEquality) ||
        Is(Token(0), Token::kInequality)) {
      pos = Is(Token(0), Token::kEquality);
      Advance(0);
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
  //                  |  ( formula )
  //                  |  abbreviation
  //                  |  literal
  Result<Formula::Ref> primary_formula(int binary_connective_recursion = 0) {
    if (Is(Token(0), Token::kNot)) {
      Advance(0);
      Result<Formula::Ref> phi = primary_formula();
      if (!phi) {
        return Failure<Formula::Ref>(LELA_MSG("Expected a primary formula within negation"), phi);
      }
      return Success(Formula::Not(std::move(phi.val)));
    }
    if (Is(Token(0), Token::kExists) || Is(Token(0), Token::kForall)) {
      bool ex = Is(Token(0), Token::kExists);
      Advance(0);
      Result<Term> x = term();
      if (!x) {
        return Failure<Formula::Ref>(LELA_MSG("Expected variable in quantifier"), x);
      }
      if (!x.val.variable()) {
        return Failure<Formula::Ref>(LELA_MSG("Expected variable in quantifier"), x);
      }
      Result<Formula::Ref> phi = primary_formula();
      if (!phi) {
        return Failure<Formula::Ref>(LELA_MSG("Expected primary formula within quantifier"), phi);
      }
      return Success(ex ? Formula::Exists(x.val, std::move(phi.val))
                        : Formula::Not(Formula::Exists(x.val, Formula::Not(std::move(phi.val)))));
    }
    if (Is(Token(0), Token::kLeftParen)) {
      Advance(0);
      Result<Formula::Ref> phi = formula();
      if (!phi) {
        return Failure<Formula::Ref>(LELA_MSG("Expected formula within brackets"), phi);
      }
      if (!Is(Token(0), Token::kRightParen)) {
        return Failure<Formula::Ref>(LELA_MSG("Expected closing right parenthesis ')'"));
      }
      Advance(0);
      return phi;
    }
    if (Is(Token(0), Token::kIdentifier) && ctx_->IsRegisteredFormula(Token(0).val.str())) {
      std::string id = Token(0).val.str();
      Advance(0);
      return Success(ctx_->LookupFormula(id).Clone());
    }
    Result<std::unique_ptr<Literal>> a = literal();
    if (!a) {
      return Failure<Formula::Ref>(LELA_MSG("Expected literal"), a);
    }
    return Success(Formula::Atomic(Clause{*a.val}));
  }

  // conjunctive_formula --> primary_formula [ && primary_formula ]*
  Result<Formula::Ref> conjunctive_formula() {
    Result<Formula::Ref> phi = primary_formula();
    if (!phi) {
      return Failure<Formula::Ref>(LELA_MSG("Expected left conjunctive formula"), phi);
    }
    while (Is(Token(0), Token::kAnd)) {
      Advance(0);
      Result<Formula::Ref> psi = primary_formula();
      if (!psi) {
        return Failure<Formula::Ref>(LELA_MSG("Expected left conjunctive formula"), psi);
      }
      phi = Success(Formula::Not(Formula::Or(Formula::Not(std::move(phi.val)),
                                             Formula::Not(std::move(psi.val)))));
    }
    return phi;
  }

  // disjunctive_formula --> conjunctive_formula [ || conjunctive_formula ]*
  Result<Formula::Ref> disjunctive_formula() {
    Result<Formula::Ref> phi = conjunctive_formula();
    if (!phi) {
      return Failure<Formula::Ref>(LELA_MSG("Expected left argument conjunctive formula"), phi);
    }
    while (Is(Token(0), Token::kOr)) {
      Advance(0);
      Result<Formula::Ref> psi = conjunctive_formula();
      if (!psi) {
        return Failure<Formula::Ref>(LELA_MSG("Expected right argument conjunctive formula"), psi);
      }
      phi = Success(Formula::Or(std::move(phi.val), std::move(psi.val)));
    }
    return phi;
  }

  // implication_formula --> disjunctive_formula -> disjunctive_formula
  //                      |  disjunctive_formula
  Result<Formula::Ref> implication_formula() {
    Result<Formula::Ref> phi = disjunctive_formula();
    if (!phi) {
      return Failure<Formula::Ref>(LELA_MSG("Expected left argument disjunctive formula"), phi);
    }
    if (Is(Token(0), Token::kRArrow)) {
      Advance(0);
      Result<Formula::Ref> psi = disjunctive_formula();
      if (!psi) {
        return Failure<Formula::Ref>(LELA_MSG("Expected right argument disjunctive formula"), psi);
      }
      phi = Success(Formula::Or(Formula::Not(std::move(phi.val)), std::move(psi.val)));
    }
    return phi;
  }

  // equivalence_formula --> implication_formula -> implication_formula
  //                      |  implication_formula
  Result<Formula::Ref> equivalence_formula() {
    Result<Formula::Ref> phi = implication_formula();
    if (!phi) {
      return Failure<Formula::Ref>(LELA_MSG("Expected left argument implication formula"), phi);
    }
    if (Is(Token(0), Token::kLRArrow)) {
      Advance(0);
      Result<Formula::Ref> psi = implication_formula();
      if (!psi) {
        return Failure<Formula::Ref>(LELA_MSG("Expected right argument implication formula"), psi);
      }
      Formula::Ref lr = Formula::Or(Formula::Not(phi.val->Clone()), psi.val->Clone());
      Formula::Ref rl = Formula::Or(Formula::Not(std::move(phi.val)), std::move(psi.val));
      phi = Success(Formula::Not(Formula::Or(Formula::Not(std::move(lr)), Formula::Not(std::move(rl)))));
    }
    return phi;
  }

  // formula --> equivalence_formula
  Result<Formula::Ref> formula() {
    return equivalence_formula();
  }

  // abbreviation --> let identifier := formula
  Result<bool> abbreviation() {
    if (!Is(Token(0), Token::kLet)) {
      return Unapplicable<bool>(LELA_MSG("Expected abbreviation operator 'let'"));
    }
    Advance(0);
    if (!Is(Token(0), Token::kIdentifier)) {
      return Failure<bool>(LELA_MSG("Expected fresh identifier"));
    }
    const std::string id = Token(0).val.str();
    Advance(0);
    if (!Is(Token(0), Token::kAssign)) {
      return Failure<bool>(LELA_MSG("Expected assignment operator ':='"));
    }
    Advance(0);
    const Result<Formula::Ref> phi = formula();
    if (!phi) {
      return Failure<bool>(LELA_MSG("Expected formula"), phi);
    }
    if (!Is(Token(0), Token::kEndOfLine)) {
      return Failure<bool>(LELA_MSG("Expected end of line ';'"));
    }
    Advance(0);
    ctx_->RegisterFormula(id, *phi.val);
    return Success(true);
  }

  // abbreviations --> abbreviation*
  Result<bool> abbreviations() {
    Result<bool> r;
    while ((r = abbreviation())) {
    }
    if (!r.unapplicable) {
      return r;
    } else {
      return Success(true);
    }
  }

  // kb_clause --> KB formula ;
  Result<bool> kb_clause() {
    if (!Is(Token(0), Token::kKB)) {
      return Unapplicable<bool>(LELA_MSG("No kb_clause"));
    }
    Advance(0);
    Result<Formula::Ref> phi = formula();
    if (!phi) {
      return Failure<bool>(LELA_MSG("Expected KB clause formula"), phi);
    }
    if (!Is(Token(0), Token::kEndOfLine)) {
      return Failure<bool>(LELA_MSG("Expected end of line ';'"));
    }
    Advance(0);
    phi.val = phi.val->NF(ctx_->sf(), ctx_->tf());
    const Formula* phi_ptr = phi.val.get();
    size_t nots = 0;
    for (;;) {
      switch (phi_ptr->type()) {
        case Formula::kAtomic: {
          if (nots % 2 != 0) {
            goto error;
          }
          const Clause c = phi_ptr->as_atomic().arg();
          if (!std::all_of(c.begin(), c.end(), [](Literal a) {
                           return a.quasiprimitive() || (!a.lhs().function() && !a.rhs().function()); })) {
            goto error;
          }
          ctx_->AddClause(c);
          return Success(true);
        }
        case Formula::kNot: {
          ++nots;
          phi_ptr = &phi_ptr->as_not().arg();
          break;
        }
        case Formula::kOr: {
          goto error;
        }
        case Formula::kExists: {
          if (nots % 2 == 0) {
            goto error;
          }
          phi_ptr = &phi_ptr->as_exists().arg();
          break;
        }
      }
    }
error:
    return Failure<bool>(LELA_MSG("Expected KB clause formula: NF of the formula must be clause with universal quantifiers"), phi);
  }

  // kb_clauses --> kb_clause*
  Result<bool> kb_clauses() {
    Result<bool> r;
    while ((r = kb_clause())) {
    }
    if (!r.unapplicable) {
      return r;
    } else {
      return Success(true);
    }
  }

  // query --> Entails(k, formula) | Consistent(k, formula)
  Result<bool> query() {
    if (!Is(Token(0), Token::kEntails) &&
        !Is(Token(0), Token::kConsistent)) {
      return Unapplicable<bool>(LELA_MSG("No query"));
    }
    const bool entailment = Is(Token(0), Token::kEntails);
    Advance(0);
    if (!Is(Token(0), Token::kLeftParen)) {
      return Failure<bool>(LELA_MSG("Expected left parenthesis '('"));
    }
    Advance(0);
    if (!Is(Token(0), Token::kUint)) {
      return Failure<bool>(LELA_MSG("Expected split level integer"));
    }
    const int k = std::stoi(Token(0).val.str());
    Advance(0);
    if (!Is(Token(0), Token::kComma)) {
      return Failure<bool>(LELA_MSG("Expected comma ';'"));
    }
    Advance(0);
    Result<Formula::Ref> phi = formula();
    if (!phi) {
      return Failure<bool>(LELA_MSG("Expected query formula"));
    }
    if (!Is(Token(0), Token::kRightParen)) {
      return Failure<bool>(LELA_MSG("Expected right parenthesis ')'"));
    }
    Advance(0);
    if (!Is(Token(0), Token::kEndOfLine)) {
      return Failure<bool>(LELA_MSG("Expected end of line ';'"));
    }
    Advance(0);
    const Formula::Ref phi_nf = phi.val->NF(ctx_->sf(), ctx_->tf());
    if (entailment) {
      const bool r = ctx_->solver()->Entails(k, *phi_nf);
      ctx_->logger()(Logger::EntailmentData(k, &ctx_->solver()->setup(), *phi_nf, r));
      return Success(bool(r));
    } else {
      const bool r = ctx_->solver()->Consistent(k, *phi_nf);
      ctx_->logger()(Logger::ConsistencyData(k, &ctx_->solver()->setup(), *phi_nf, r));
      return Success(bool(r));
    }
  }

  // queries --> query*
  Result<bool> queries() {
    Result<bool> r;
    bool all = true;
    while ((r = query())) {
      all &= r.val;
    }
    if (!r.unapplicable) {
      return r;
    } else {
      return Success(bool(all));
    }
  }

  // assertion_refutation --> Assert query | Refute query
  Result<bool> assertion_refutation() {
    if (!Is(Token(0), Token::kAssert) &&
        !Is(Token(0), Token::kRefute)) {
      return Unapplicable<bool>(LELA_MSG("No assertion_refutation"));
    }
    Result<bool> failure = Failure<bool>(LELA_MSG("Assertion/refutation failed"));
    const bool pos = Is(Token(0), Token::kAssert);
    Advance(0);
    Result<bool> r = query();
    if (!r) {
      return Failure<bool>(LELA_MSG("Expected formula"), r);
    }
    if (r.val == pos) {
      return Success(true);
    } else {
      return failure;
    }
  }

  // assertions_refutations --> assertion_refutation*
  Result<bool> assertions_refutations() {
    Result<bool> r;
    bool all = true;
    while ((r = assertion_refutation())) {
      all &= r.val;
    }
    if (!r.unapplicable) {
      return r;
    } else {
      return Success(bool(all));
    }
  }

  // start --> declarations kb_clauses
  Result<bool> start() {
    Result<bool> r;
    iterator prev;
    do {
      prev = begin();
      r = declarations();
      if (!r) {
        return Failure<bool>(LELA_MSG("Error in declarations"), r);
      }
      r = kb_clauses();
      if (!r) {
        return Failure<bool>(LELA_MSG("Error in kb_clauses"), r);
      }
      r = abbreviations();
      if (!r) {
        return Failure<bool>(LELA_MSG("Error in abbreviations"), r);
      }
      r = queries();
      if (!r) {
        return Failure<bool>(LELA_MSG("Error in queries"), r);
      }
      r = assertions_refutations();
      if (!r) {
        return Failure<bool>(LELA_MSG("Error in assertions_refutations"), r);
      }
    } while (begin() != prev);
    std::stringstream ss;
    using lela::format::output::operator<<;
    ss << Token(0) << " " << Token(1) << " " << Token(2);
    return !Token(0) ? Success(true) : Failure<bool>(LELA_MSG("Unparsed input "+ ss.str()));
  }

  bool Is(const lela::internal::Maybe<Token>& symbol, Token::Id id) const {
    return symbol && symbol.val.id() == id;
  }

  template<typename UnaryPredicate>
  bool Is(const lela::internal::Maybe<Token>& symbol, Token::Id id, UnaryPredicate p) const {
    return symbol && symbol.val.id() == id && p(symbol.val.str());
  }

  lela::internal::Maybe<Token> Token(size_t n = 0) const {
    auto it = begin();
    for (size_t i = 0; i < n && it != end_; ++i) {
      assert(begin() != end());
      ++it;
    }
    return it != end_ ? lela::internal::Just(*it) : lela::internal::Nothing;
  }

  void Advance(size_t n = 0) {
    begin_plus_ += n + 1;
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

  Lex lexer_;
  mutable iterator begin_;  // don't use begin_ directly: to avoid the stream blocking us, Advance() actually increments
  mutable size_t begin_plus_ = 0;  // begin_plus_ instead of begin_; use begin() to obtain the incremented iterator.
  iterator end_;
  Context<LogPredicate>* ctx_;
  LogPredicate log_;
};

}  // namespace pdl
}  // namespace format
}  // namespace lela

#endif  // LELA_FORMAT_PDL_PARSER_H_

