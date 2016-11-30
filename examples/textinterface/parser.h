// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering
//
// Recursive descent parser for our text interface. The grammar for formulas is
// aims to reduce brackets and implement operator precedence.

#ifndef EXAMPLES_TEXTINTERFACE_PARSER_H_
#define EXAMPLES_TEXTINTERFACE_PARSER_H_

#include <cassert>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <lela/formula.h>
#include <lela/setup.h>
#include <lela/format/output.h>

#include "kb.h"
#include "lexer.h"

#define S(x)          #x
#define S_(x)         S(x)
#define S__LINE__     S_(__LINE__)
#define MSG(msg)      (std::string(__FUNCTION__) +":"+ S__LINE__ +": "+ msg)

struct Announcer {
  virtual ~Announcer() {}
  virtual void AnnounceEntailment(int k, const lela::Setup& s, const lela::Formula& phi, bool yes) = 0;
  virtual void AnnounceConsistency(int k, const lela::Setup& s, const lela::Formula& phi, bool yes) = 0;
};

template<typename ForwardIt>
class Parser {
 public:
  typedef Lexer<ForwardIt> Lex;
  typedef typename Lex::iterator iterator;

  // Encapsulates a parsing result, either a Success, an Unapplicable, or a Failure.
  template<typename T>
  struct Result {
    Result() = default;
    explicit Result(const T& val) : ok(true), val(val) {}
    Result(bool unapplicable, const std::string& msg, ForwardIt begin, ForwardIt end, const T& val = T())
        : ok(false), val(val), unapplicable(unapplicable), msg(msg), begin_(begin), end_(end) {}
    Result(const Result&) = default;
    Result& operator=(const Result&) = default;

    explicit operator bool() const { return ok; }

    ForwardIt begin() const { return begin_; }
    ForwardIt end()   const { return end_; }

    std::string to_string() const {
      std::stringstream ss;
      if (ok) {
        ss << "Success(" << val << ")";
      } else if (unapplicable) {
        ss << "Unapplicable(" << msg << ", \"" << std::string(begin(), end()) << "\")";
      } else {
        ss << "Failure(" << msg << ", \"" << std::string(begin(), end()) << "\")";
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

  Parser(ForwardIt begin, ForwardIt end, Announcer* announcer)
      : lexer_(begin, end), begin_(lexer_.begin()), end_(lexer_.end()), announcer_(announcer) {}

  Result<bool> Parse() { return start(); }

  KB& kb() { return kb_; }
  const KB& kb() const { return kb_; }

 private:
  template<typename T>
  Result<T> Success(const T& result) {
    return Result<T>(result);
  }

  template<typename T>
  Result<T> Failure(const std::string& msg, const T& val = T()) const {
    return Result<T>(false, msg, begin_.char_iter(), end_.char_iter(), val);
  }

  template<typename T, typename U>
  static Result<T> Failure(const std::string& msg, const Result<U>& r, const T& val = T()) {
    return Result<T>(false, msg + " [because] " + r.msg, r.begin(), r.end(), val);
  }

  template<typename T>
  Result<T> Unapplicable(const std::string& msg) const {
    return Result<T>(true, msg, begin_.char_iter(), end_.char_iter());
  }

  // declaration --> sort <sort-id> ;
  //              |  var <id> => <sort-id> ;
  //              |  name <id> => <sort-id> ;
  //              |  fun <id> / <arity> => <sort-id> ;
  Result<bool> declaration() {
    if (!Is(Symbol(0), Token::kSort) &&
        !Is(Symbol(0), Token::kVar) &&
        !Is(Symbol(0), Token::kName) &&
        !Is(Symbol(0), Token::kFun)) {
      return Unapplicable<bool>(MSG("No declaration"));
    }
    if (Is(Symbol(0), Token::kSort) &&
        Is(Symbol(1), Token::kIdentifier, [this](const std::string& s) { return !kb_.IsRegisteredSort(s); }) &&
        Is(Symbol(2), Token::kEndOfLine)) {
      kb_.RegisterSort(Symbol(1).val.str());
      Advance(2);
      return Success<bool>(true);
    }
    if (Is(Symbol(0), Token::kVar) &&
        Is(Symbol(1), Token::kIdentifier, [this](const std::string& s) { return !kb_.IsRegisteredTerm(s); }) &&
        Is(Symbol(2), Token::kRArrow) &&
        Is(Symbol(3), Token::kIdentifier, [this](const std::string& s) { return kb_.IsRegisteredSort(s); }) &&
        Is(Symbol(4), Token::kEndOfLine)) {
      kb_.RegisterVar(Symbol(1).val.str(), Symbol(3).val.str());
      Advance(4);
      return Success<bool>(true);
    }
    if (Is(Symbol(0), Token::kName) &&
        Is(Symbol(1), Token::kIdentifier, [this](const std::string& s) { return !kb_.IsRegisteredTerm(s); }) &&
        Is(Symbol(2), Token::kRArrow) &&
        Is(Symbol(3), Token::kIdentifier, [this](const std::string& s) { return kb_.IsRegisteredSort(s); }) &&
        Is(Symbol(4), Token::kEndOfLine)) {
      kb_.RegisterName(Symbol(1).val.str(), Symbol(3).val.str());
      Advance(4);
      return Success<bool>(true);
    }
    if (Is(Symbol(0), Token::kFun) &&
        Is(Symbol(1), Token::kIdentifier, [this](const std::string& s) { return !kb_.IsRegisteredTerm(s); }) &&
        Is(Symbol(2), Token::kSlash) &&
        Is(Symbol(3), Token::kUint) &&
        Is(Symbol(4), Token::kRArrow) &&
        Is(Symbol(5), Token::kIdentifier, [this](const std::string& s) { return kb_.IsRegisteredSort(s); }) &&
        Is(Symbol(6), Token::kEndOfLine)) {
      kb_.RegisterFun(Symbol(1).val.str(), std::stoi(Symbol(3).val.str()), Symbol(5).val.str());
      Advance(6);
      return Success<bool>(true);
    }
    return Failure<bool>(MSG("Invalid sort/var/name/fun declaration"));
  }

  // declarations --> declaration*
  Result<bool> declarations() {
    Result<bool> r;
    while ((r = declaration())) {
    }
    return !r.unapplicable ? r : Success<bool>(true);
  }

  // term --> x
  //       |  n
  //       |  f
  //       |  f(term, ..., term)
  Result<lela::Term> term() {
    if (Is(Symbol(0), Token::kIdentifier, [this](const std::string& s) { return kb_.IsRegisteredVar(s); })) {
      lela::Term x = kb_.LookupVar(Symbol(0).val.str());
      Advance(0);
      return Success(x);
    }
    if (Is(Symbol(0), Token::kIdentifier, [this](const std::string& s) { return kb_.IsRegisteredName(s); })) {
      lela::Term n = kb_.LookupName(Symbol(0).val.str());
      Advance(0);
      return Success(n);
    }
    if (Is(Symbol(0), Token::kIdentifier, [this](const std::string& s) { return kb_.IsRegisteredFun(s); })) {
      lela::Symbol s = kb_.LookupFun(Symbol(0).val.str());
      Advance(0);
      lela::Term::Vector args;
      if (s.arity() > 0 || Is(Symbol(0), Token::kLeftParen)) {
        if (!Is(Symbol(0), Token::kLeftParen)) {
          return Failure<lela::Term>(MSG("Expected left parenthesis '('"));
        }
        Advance(0);
        for (lela::Symbol::Arity i = 0; i < s.arity(); ++i) {
          if (i > 0) {
            if (!Is(Symbol(0), Token::kComma)) {
              return Failure<lela::Term>(MSG("Expected comma ','"));
            }
            Advance(0);
          }
          Result<lela::Term> t = term();
          if (!t) {
            return Failure<lela::Term>(MSG("Expected argument term"), t);
          }
          args.push_back(t.val);
        }
        if (!Is(Symbol(0), Token::kRightParen)) {
          return Failure<lela::Term>(MSG("Expected right parenthesis ')'"));
        }
        Advance(0);
      }
      return Success(kb_.solver().tf()->CreateTerm(s, args));
    }
    return Failure<lela::Term>(MSG("Expected a term"));
  }

  // literal --> term [ '==' | '!=' ] term
  Result<lela::Literal> literal() {
    Result<lela::Term> lhs = term();
    if (!lhs) {
      return Failure<lela::Literal>(MSG("Expected a lhs term"), lhs, DUMMY_LITERAL_);
    }
    bool pos;
    if (Is(Symbol(0), Token::kEquality) ||
        Is(Symbol(0), Token::kInequality)) {
      pos = Is(Symbol(0), Token::kEquality);
      Advance(0);
    } else {
      return Failure<lela::Literal>(MSG("Expected equality or inequality '=='/'!='"), DUMMY_LITERAL_);
    }
    Result<lela::Term> rhs = term();
    if (!rhs) {
      return Failure<lela::Literal>(MSG("Expected rhs term"), rhs, DUMMY_LITERAL_);
    }
    lela::Literal a = pos ? lela::Literal::Eq(lhs.val, rhs.val) : lela::Literal::Neq(lhs.val, rhs.val);
    return Success<lela::Literal>(a);
  }

  // kb_clause --> () ;
  //            |  ( literal [, literal]* ) ;
  Result<bool> kb_clause() {
    if (!Is(Symbol(0), Token::kKB)) {
      return Unapplicable<bool>(MSG("No kb_clause"));
    }
    Advance(0);
    std::vector<lela::Literal> ls;
    for (int i = 0; (i == 0 && Is(Symbol(0), Token::kLeftParen)) ||
                    (i >  0 && (Is(Symbol(0), Token::kComma) || Is(Symbol(0), Token::kOr))); ++i) {
      Advance(0);
      Result<lela::Literal> a = literal();
      if (!a) {
        return Failure<bool>(MSG("Expected literal"), a);
      }
      ls.push_back(a.val);
    }
    if (!Is(Symbol(0), Token::kRightParen)) {
      return Failure<bool>(MSG("Expected right parenthesis ')'"));
    }
    Advance(0);
    if (!Is(Symbol(0), Token::kEndOfLine)) {
      return Failure<bool>(MSG("Expected end of line ';'"));
    }
    Advance(0);
    const lela::Clause c(ls.begin(), ls.end());
    if (!std::all_of(c.begin(), c.end(), [](lela::Literal a) { return (!a.lhs().function() && !a.rhs().function()) || a.quasiprimitive(); })) {
      using lela::format::operator<<;
      std::stringstream ss;
      ss << c;
      return Failure<bool>(MSG("KB clause "+ ss.str() +" must only contain ewff/quasiprimitive literals"));
    }
    kb_.AddClause(c);
    return Success<bool>(true);
  }

  // kb_clauses --> kb_clause*
  Result<bool> kb_clauses() {
    Result<bool> r;
    while ((r = kb_clause())) {
    }
    return !r.unapplicable ? r : Success<bool>(true);
  }

  // primary_formula --> ! primary_formula
  //                  |  Ex x primary_formula
  //                  |  Fa x primary_formula
  //                  |  ( formula )
  //                  |  abbreviation
  //                  |  literal
  Result<lela::Formula> primary_formula(int binary_connective_recursion = 0) {
    if (Is(Symbol(0), Token::kNot)) {
      Advance(0);
      Result<lela::Formula> phi = primary_formula();
      if (!phi) {
        return Failure<lela::Formula>(MSG("Expected a primary formula within negation"), phi, DUMMY_FORMULA_);
      }
      return Success<lela::Formula>(lela::Formula::Not(phi.val));
    }
    if (Is(Symbol(0), Token::kExists) || Is(Symbol(0), Token::kForall)) {
      bool ex = Is(Symbol(0), Token::kExists);
      Advance(0);
      Result<lela::Term> x = term();
      if (!x.val.variable()) {
        return Failure<lela::Formula>(MSG("Expected variable in quantifier"), x, DUMMY_FORMULA_);
      }
      Result<lela::Formula> phi = primary_formula();
      if (!phi) {
        return Failure<lela::Formula>(MSG("Expected primary formula within quantifier"), phi, DUMMY_FORMULA_);
      }
      return Success<lela::Formula>(ex ? lela::Formula::Exists(x.val, phi.val)
                                       : lela::Formula::Not(lela::Formula::Exists(x.val, lela::Formula::Not(phi.val))));
    }
    if (Is(Symbol(0), Token::kLeftParen)) {
      Advance(0);
      Result<lela::Formula> phi = formula();
      if (!phi) {
        return Failure<lela::Formula>(MSG("Expected formula within brackets"), phi, DUMMY_FORMULA_);
      }
      if (!Is(Symbol(0), Token::kRightParen)) {
        return Failure<lela::Formula>(MSG("Expected closing right parenthesis ')'"), DUMMY_FORMULA_);
      }
      Advance(0);
      return phi;
    }
    if (Is(Symbol(0), Token::kIdentifier) && kb_.IsRegisteredFormula(Symbol(0).val.str())) {
      std::string id = Symbol(0).val.str();
      Advance(0);
      return Success<lela::Formula>(kb_.LookupFormula(id));
    }
    Result<lela::Literal> a = literal();
    if (a) {
      return Success<lela::Formula>(lela::Formula::Clause(lela::Clause{a.val}));
    }
    return Failure<lela::Formula>("Expected formula", DUMMY_FORMULA_);
  }

  // conjunctive_formula --> primary_formula && primary_formula
  Result<lela::Formula> conjunctive_formula() {
    Result<lela::Formula> phi = primary_formula();
    if (!phi) {
      return Failure<lela::Formula>(MSG("Expected left conjunctive formula"), phi, DUMMY_FORMULA_);
    }
    while (Is(Symbol(0), Token::kAnd)) {
      Advance(0);
      Result<lela::Formula> psi = primary_formula();
      if (!psi) {
        return Failure<lela::Formula>(MSG("Expected left conjunctive formula"), psi, DUMMY_FORMULA_);
      }
      phi = Success<lela::Formula>(lela::Formula::Not(lela::Formula::Or(lela::Formula::Not(phi.val),
                                                                        lela::Formula::Not(psi.val))));
    }
    return phi;
  }

  // disjunctive_formula --> conjunctive_formula || conjunctive_formula
  Result<lela::Formula> disjunctive_formula() {
    Result<lela::Formula> phi = conjunctive_formula();
    if (!phi) {
      return Failure<lela::Formula>(MSG("Expected left argument conjunctive formula"), phi, DUMMY_FORMULA_);
    }
    while (Is(Symbol(0), Token::kOr)) {
      Advance(0);
      Result<lela::Formula> psi = conjunctive_formula();
      if (!psi) {
        return Failure<lela::Formula>(MSG("Expected right argument conjunctive formula"), psi, DUMMY_FORMULA_);
      }
      phi = Success<lela::Formula>(lela::Formula::Or(phi.val, psi.val));
    }
    return phi;
  }

  // formula --> disjunctive_formula
  Result<lela::Formula> formula() {
    return disjunctive_formula();
  }

  // abbreviation --> let identifier := formula
  Result<bool> abbreviation() {
    if (!Is(Symbol(0), Token::kLet)) {
      return Unapplicable<bool>(MSG("Expected abbreviation operator 'let'"));
    }
    Advance(0);
    if (!Is(Symbol(0), Token::kIdentifier)) {
      return Failure<bool>(MSG("Expected fresh identifier"));
    }
    const std::string id = Symbol(0).val.str();
    Advance(0);
    if (!Is(Symbol(0), Token::kAssign)) {
      return Failure<bool>(MSG("Expected assignment operator ':='"));
    }
    Advance(0);
    const Result<lela::Formula> phi = formula();
    if (!phi) {
      return Failure<bool>(MSG("Expected formula"), phi);
    }
    if (!Is(Symbol(0), Token::kEndOfLine)) {
      return Failure<bool>(MSG("Expected end of line ';'"));
    }
    Advance(0);
    kb_.RegisterFormula(id, phi.val);
    return Success<bool>(true);
  }

  // abbreviations --> abbreviation*
  Result<bool> abbreviations() {
    Result<bool> r;
    while ((r = abbreviation())) {
    }
    return !r.unapplicable ? r : Success<bool>(true);
  }

  // query --> Entails(k, formula) | Consistent(k, formula)
  Result<bool> query() {
    if (!Is(Symbol(0), Token::kEntails) &&
        !Is(Symbol(0), Token::kConsistent)) {
      return Unapplicable<bool>(MSG("No query"));
    }
    const bool entailment = Is(Symbol(0), Token::kEntails);
    Advance(0);
    if (!Is(Symbol(0), Token::kLeftParen)) {
      return Failure<bool>(MSG("Expected left parenthesis '('"));
    }
    Advance(0);
    if (!Is(Symbol(0), Token::kUint)) {
      return Failure<bool>(MSG("Expected split level integer"));
    }
    const int k = std::stoi(Symbol(0).val.str());
    Advance(0);
    if (!Is(Symbol(0), Token::kComma)) {
      return Failure<bool>(MSG("Expected comma ';'"));
    }
    Advance(0);
    Result<lela::Formula> phi = formula();
    if (!phi) {
      return Failure<bool>(MSG("Expected query formula"));
    }
    if (!Is(Symbol(0), Token::kRightParen)) {
      return Failure<bool>(MSG("Expected right parenthesis ')'"));
    }
    Advance(0);
    if (!Is(Symbol(0), Token::kEndOfLine)) {
      return Failure<bool>(MSG("Expected end of line ';'"));
    }
    Advance(0);
    const lela::Formula phi_nf = phi.val.reader().NF();
    if (entailment) {
      const bool r = kb_.solver().Entails(k, phi_nf.reader());
      announcer_->AnnounceEntailment(k, kb_.solver().setup(), phi_nf, r);
      return Success<bool>(r);
    } else {
      const bool r = kb_.solver().Consistent(k, phi_nf.reader());
      announcer_->AnnounceConsistency(k, kb_.solver().setup(), phi_nf, r);
      return Success<bool>(r);
    }
  }

  // queries --> query*
  Result<bool> queries() {
    Result<bool> r;
    bool all = true;
    while ((r = query())) {
      all &= r.val;
    }
    return !r.unapplicable ? r : Success<bool>(all);
  }

  // assertion_refutation --> Assert query | Refute query
  Result<bool> assertion_refutation() {
    if (!Is(Symbol(0), Token::kAssert) &&
        !Is(Symbol(0), Token::kRefute)) {
      return Unapplicable<bool>(MSG("No assertion_refutation"));
    }
    Result<bool> failure = Failure<bool>(MSG("Assertion/refutation failed"));
    const bool pos = Is(Symbol(0), Token::kAssert);
    Advance(0);
    Result<bool> r = query();
    if (!r) {
      return Failure<bool>(MSG("Expected formula"), r);
    }
    return r.val == pos ? Success<bool>(true) : failure;
  }

  // assertions_refutations --> assertion_refutation*
  Result<bool> assertions_refutations() {
    Result<bool> r;
    bool all = true;
    while ((r = assertion_refutation())) {
      all &= r.val;
    }
    return !r.unapplicable ? r : Success<bool>(all);
  }

  // start --> declarations kb_clauses
  Result<bool> start() {
    Result<bool> r;
    iterator last;
    do {
      last = begin_;
      r = declarations();
      if (!r) {
        return Failure<bool>(MSG("Error in declarations"), r);
      }
      r = kb_clauses();
      if (!r) {
        return Failure<bool>(MSG("Error in kb_clauses"), r);
      }
      r = abbreviations();
      if (!r) {
        return Failure<bool>(MSG("Error in abbreviations"), r);
      }
      r = queries();
      if (!r) {
        return Failure<bool>(MSG("Error in queries"), r);
      }
      r = assertions_refutations();
      if (!r) {
        return Failure<bool>(MSG("Error in assertions_refutations"), r);
      }
    } while (begin_ != last);
    std::stringstream ss;
    ss << Symbol(0) << " " << Symbol(1) << " " << Symbol(2);
    return !Symbol(0) ? Success<bool>(true) : Failure<bool>(MSG("Unparsed input "+ ss.str()));
  }

  bool Is(const lela::internal::Maybe<Token>& symbol, Token::Id id) const {
    return symbol && symbol.val.id() == id;
  }

  template<typename UnaryPredicate>
  bool Is(const lela::internal::Maybe<Token>& symbol, Token::Id id, UnaryPredicate p) const {
    return symbol && symbol.val.id() == id && p(symbol.val.str());
  }

  lela::internal::Maybe<Token> Symbol(int n = 0) const {
    auto it = begin_;
    for (int i = 0; i < n && it != end_; ++i) {
      assert(begin_ != end_);
      ++it;
    }
    return it != end_ ? lela::internal::Just(*it) : lela::internal::Nothing;
  }

  void Advance(int n = 0) {
    for (int i = 0; i <= n; ++i) {
      assert(begin_ != end_);
      ++begin_;
    }
  }

  Lex lexer_;
  iterator begin_;
  iterator end_;
  KB kb_;
  Announcer* announcer_;
  const lela::Literal DUMMY_LITERAL_ = lela::Literal::Eq(kb_.solver().tf()->CreateTerm(kb_.solver().sf()->CreateName(0)),
                                                         kb_.solver().tf()->CreateTerm(kb_.solver().sf()->CreateName(0)));
  const lela::Formula DUMMY_FORMULA_ = lela::Formula::Clause(lela::Clause{});
};

#endif  // EXAMPLES_TEXTINTERFACE_PARSER_H_

