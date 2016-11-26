// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Implements a simple language to specify entailment problems.

#include <cassert>

#include <algorithm>
#include <iostream>
#include <list>
#include <functional>
#include <string>
#include <sstream>
#include <utility>

#include <lela/solver.h>
#include <lela/format/output.h>
#include <lela/format/syntax.h>

#define S(x)          #x
#define S_(x)         S(x)
#define S__LINE__     S_(__LINE__)
#define MSG(msg)      (std::string(__FUNCTION__) +":"+ S__LINE__ +": "+ msg)

using namespace lela;
using namespace lela::format;

struct syntax_error : public std::exception {
  syntax_error(const std::string& id) : id_(id) {}
  virtual const char* what() const noexcept override { return id_.c_str(); }
 private:
  std::string id_;
};
struct redeclared_error : public syntax_error { using syntax_error::syntax_error; };
struct undeclared_error : public syntax_error { using syntax_error::syntax_error; };

struct KB {
  KB() : context_(solver_.sf(), solver_.tf()) {}
  ~KB() {
  }

  bool IsRegisteredSort(const std::string& id) const {
    return sorts_.find(id) != sorts_.end();
  }

  bool IsRegisteredVar(const std::string& id) const {
    return vars_.find(id) != vars_.end();
  }

  bool IsRegisteredName(const std::string& id) const {
    return names_.find(id) != names_.end();
  }

  bool IsRegisteredFun(const std::string& id) const {
    return funs_.find(id) != funs_.end();
  }

  bool IsRegisteredFormula(const std::string& id) const {
    return formulas_.find(id) != formulas_.end();
  }

  bool IsRegistered(const std::string& id) const {
    return IsRegisteredSort(id) || IsRegisteredVar(id) || IsRegisteredName(id) || IsRegisteredFun(id) || IsRegisteredFormula(id);
  }

  Symbol::Sort LookupSort(const std::string& id) const {
    const auto it = sorts_.find(id);
    if (it == sorts_.end())
      throw undeclared_error(id);
    return it->second;
  }

  Term LookupVar(const std::string& id) const {
    const auto it = vars_.find(id);
    if (it == vars_.end())
      throw undeclared_error(id);
    return it->second;
  }

  Term LookupName(const std::string& id) const {
    const auto it = names_.find(id);
    if (it == names_.end())
      throw undeclared_error(id);
    return it->second;
  }

  const Symbol& LookupFun(const std::string& id) const {
    const auto it = funs_.find(id);
    if (it == funs_.end())
      throw undeclared_error(id);
    return it->second;
  }

  const Formula& LookupFormula(const std::string& id) const {
    const auto it = formulas_.find(id);
    if (it == formulas_.end())
      throw undeclared_error(id);
    return it->second;
  }

  void RegisterSort(const std::string& id) {
    const Symbol::Sort sort = context_.NewSort();
    //lela::format::RegisterSort(sort, id);
    lela::format::RegisterSort(sort, "");
    sorts_[id] = sort;
    std::cout << "RegisterSort " << id << std::endl;
  }

  void RegisterVar(const std::string& id, const std::string& sort_id) {
    if (IsRegistered(id))
      throw redeclared_error(id);
    const Symbol::Sort sort = LookupSort(sort_id);
    const Term var = context_.NewVar(sort);
    vars_[id] = var;
    lela::format::RegisterSymbol(var.symbol(), id);
    std::cout << "RegisterVar " << id << " -> " << sort_id << std::endl;
  }

  void RegisterName(const std::string& id, const std::string& sort_id) {
    if (IsRegistered(id))
      throw redeclared_error(id);
    const Symbol::Sort sort = LookupSort(sort_id);
    const Term name = context_.NewName(sort);
    names_[id] = name;
    lela::format::RegisterSymbol(name.symbol(), id);
    std::cout << "RegisterName " << id << " -> " << sort_id << std::endl;
  }

  void RegisterFun(const std::string& id, Symbol::Arity arity, const std::string& sort_id) {
    if (IsRegistered(id))
      throw redeclared_error(id);
    const Symbol::Sort sort = sorts_[sort_id];
    funs_.emplace(id, context_.NewFun(sort, arity));
    Symbol s = funs_.find(id)->second;
    lela::format::RegisterSymbol(s, id);
    std::cout << "RegisterFun " << id << " -> " << static_cast<int>(arity) << " -> " << sort_id << std::endl;
  }

  void RegisterFormula(const std::string& id, const Formula& phi) {
    if (IsRegistered(id))
      throw redeclared_error(id);
    formulas_.emplace(id, phi);
    std::cout << "RegisterFormula " << id << " -> " << phi << std::endl;
  }

  Solver& solver() { return solver_; }
  const Solver& solver() const { return solver_; }

 private:
  std::map<std::string, Symbol::Sort> sorts_;
  std::map<std::string, Term>         vars_;
  std::map<std::string, Term>         names_;
  std::map<std::string, Symbol>       funs_;
  std::map<std::string, Formula>      formulas_;
  Solver  solver_;
  Context context_;
};

class Token {
 public:
  enum Id { kSort, kVar, kName, kFun, kKB, kLet, kEntails, kConsistent, kColon, kComma, kSemicolon, kEqual, kInequal,
    kNot, kOr, kAnd, kForall, kExists, kAssign, kArrow, kSlash, kLeftParen, kRightParen, kUint, kIdentifier, kError };

  Token() : id_(kError) {}
  explicit Token(Id id) : id_(id) {}
  Token(Id id, const std::string& str) : id_(id), str_(str) {}

  Id id() const { return id_; }
  const std::string& str() const { return str_; }

 private:
  friend std::ostream& operator<<(std::ostream& os, const Token& t);

  Id id_;
  std::string str_;
};

template<typename Iter>
class Lexer {
 public:
  enum Match { kMismatch, kPrefixMatch, kFullMatch };
  typedef std::list<std::pair<Token::Id, std::function<Match(Iter, Iter)>>> LexemeVector;

  struct iterator {
    typedef std::ptrdiff_t difference_type;
    typedef Token value_type;
    typedef value_type reference;
    typedef value_type* pointer;
    typedef std::input_iterator_tag iterator_category;
    typedef lela::internal::iterator_proxy<iterator> proxy;

    iterator() = default;
    iterator(const LexemeVector* lexemes, Iter it, Iter end) : lexemes_(lexemes), it_(it), end_(end) {}
    iterator(const iterator&) = default;
    iterator& operator=(const iterator&) = default;

    bool operator==(const iterator& it) const { return it_ == it.it_ && end_ == it.end_; }
    bool operator!=(const iterator& it) const { return !(*this == it); }

    reference operator*() const {
      std::pair<Iter, Iter> substr = NextWord();
      std::pair<Match, Token::Id> match = LexemeMatch(substr.first, substr.second);
      return Token(match.second, std::string(substr.first, substr.second));
    }

    iterator& operator++() {
      it_ = NextWord().second;
      return *this;
    }

    proxy operator->() const { return proxy(operator*()); }
    proxy operator++(int) { proxy p(operator*()); operator++(); return p; }

    Iter char_iter() const { return it_; }

   private:
    std::pair<Iter, Iter> NextWord() const {
      // max-munch lexer
      assert(it_ != end_);
      Iter it;
      for (it = it_; it != end_ && IsWhitespace(*it); ++it) {
      }
      Iter jt;
      for (jt = it; jt != end_; ++jt) {
        if (LexemeMatch(it, std::next(jt)).first == kMismatch)
          break;
      }
      assert(it != jt);
      return std::make_pair(it, jt);
    }

    std::pair<Match, Token::Id> LexemeMatch(Iter begin, Iter end) const {
      Match best_match = kMismatch;
      Token::Id best_token = Token::kError;
      for (auto& p : *lexemes_) {
        const Match m = p.second(begin, end);
        if ((best_match == kMismatch && m == kPrefixMatch) || (best_match != kFullMatch && m == kFullMatch)) {
          best_match = m;
          best_token = p.first;
        }
      }
      return std::make_pair(best_match, best_token);
    }

    const LexemeVector* lexemes_;
    Iter it_;
    Iter end_;
  };

  Lexer(Iter begin, Iter end) : begin_(begin), end_(end) {
    lexemes_.push_back(std::make_pair(Token::kSort,       [](Iter begin, Iter end) { return IsPrefix(begin, end, "sort"); }));
    lexemes_.push_back(std::make_pair(Token::kVar,        [](Iter begin, Iter end) { return std::max( IsPrefix(begin, end, "var"), IsPrefix(begin, end, "variable") ); }));
    lexemes_.push_back(std::make_pair(Token::kName,       [](Iter begin, Iter end) { return std::max( IsPrefix(begin, end, "name"), IsPrefix(begin, end, "stdname") ); }));
    lexemes_.push_back(std::make_pair(Token::kFun,        [](Iter begin, Iter end) { return std::max( IsPrefix(begin, end, "fun"), IsPrefix(begin, end, "function") ); }));
    lexemes_.push_back(std::make_pair(Token::kKB,         [](Iter begin, Iter end) { return IsPrefix(begin, end, "kb"); }));
    lexemes_.push_back(std::make_pair(Token::kLet,        [](Iter begin, Iter end) { return IsPrefix(begin, end, "let"); }));
    lexemes_.push_back(std::make_pair(Token::kEntails,    [](Iter begin, Iter end) { return IsPrefix(begin, end, "entails"); }));
    lexemes_.push_back(std::make_pair(Token::kConsistent, [](Iter begin, Iter end) { return IsPrefix(begin, end, "consistent"); }));
    lexemes_.push_back(std::make_pair(Token::kColon,      [](Iter begin, Iter end) { return IsPrefix(begin, end, ":"); }));
    lexemes_.push_back(std::make_pair(Token::kSemicolon,  [](Iter begin, Iter end) { return IsPrefix(begin, end, ";"); }));
    lexemes_.push_back(std::make_pair(Token::kComma,      [](Iter begin, Iter end) { return IsPrefix(begin, end, ","); }));
    lexemes_.push_back(std::make_pair(Token::kEqual,      [](Iter begin, Iter end) { return IsPrefix(begin, end, "=="); }));
    lexemes_.push_back(std::make_pair(Token::kInequal,    [](Iter begin, Iter end) { return IsPrefix(begin, end, "!="); }));
    lexemes_.push_back(std::make_pair(Token::kNot,        [](Iter begin, Iter end) { return IsPrefix(begin, end, "!"); }));
    lexemes_.push_back(std::make_pair(Token::kOr,         [](Iter begin, Iter end) { return IsPrefix(begin, end, "||"); }));
    lexemes_.push_back(std::make_pair(Token::kAnd,        [](Iter begin, Iter end) { return IsPrefix(begin, end, "&&"); }));
    lexemes_.push_back(std::make_pair(Token::kForall,     [](Iter begin, Iter end) { return IsPrefix(begin, end, "fa"); }));
    lexemes_.push_back(std::make_pair(Token::kExists,     [](Iter begin, Iter end) { return IsPrefix(begin, end, "ex"); }));
    lexemes_.push_back(std::make_pair(Token::kAssign,     [](Iter begin, Iter end) { return IsPrefix(begin, end, ":="); }));
    lexemes_.push_back(std::make_pair(Token::kArrow,      [](Iter begin, Iter end) { return IsPrefix(begin, end, "->"); }));
    lexemes_.push_back(std::make_pair(Token::kSlash,      [](Iter begin, Iter end) { return IsPrefix(begin, end, "/"); }));
    lexemes_.push_back(std::make_pair(Token::kLeftParen,  [](Iter begin, Iter end) { return IsPrefix(begin, end, "("); }));
    lexemes_.push_back(std::make_pair(Token::kRightParen, [](Iter begin, Iter end) { return IsPrefix(begin, end, ")"); }));
    lexemes_.push_back(std::make_pair(Token::kUint,       [](Iter begin, Iter end) { return begin == end ? kPrefixMatch : (*begin != '0' || std::next(begin) == end) && std::all_of(begin, end, [](char c) { return IsDigit(c); }) ? kFullMatch : kMismatch; }));
    lexemes_.push_back(std::make_pair(Token::kIdentifier, [](Iter begin, Iter end) { return begin == end ? kPrefixMatch : IsAlpha(*begin) && std::all_of(begin, end, [](char c) { return IsAlnum(c); }) ? kFullMatch : kMismatch; }));
  }

  iterator begin() const { return iterator(&lexemes_, begin_, end_); }
  iterator end()   const { return iterator(&lexemes_, end_, end_); }

 private:
  LexemeVector lexemes_;

  static Match IsPrefix(Iter begin, Iter end, const std::string& foobar) {
    size_t len = std::distance(begin, end);
    auto r = internal::transform_range(begin, end, [](char c) { return std::tolower(c); });
    if (len > foobar.length() || std::mismatch(r.begin(), r.end(), foobar.begin()).first != r.end()) {
      return kMismatch;
    } else {
      return len < foobar.length() ? kPrefixMatch : kFullMatch;
    }
  }
  static bool IsWhitespace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
  static bool IsDigit(char c) { return '0' <= c && c <= '9'; }
  static bool IsAlpha(char c) { return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || c == '_'; }
  static bool IsAlnum(char c) { return IsAlpha(c) || IsDigit(c); }

  Iter begin_;
  Iter end_;
};

struct Announcer {
  virtual void AnnounceEntailment(int k, const Setup& s, const Formula& phi, bool yes) = 0;
  virtual void AnnounceConsistency(int k, const Setup& s, const Formula& phi, bool yes) = 0;
};

template<typename Iter>
class Parser {
 public:
  typedef Lexer<Iter> Lex;
  typedef typename Lex::iterator iterator;

  // Encapsulates a parsing result, either a Success, an Unapplicable, or a Failure.
  template<typename T>
  struct Result {
    Result() = default;
    explicit Result(const T& val) : ok(true), val(val) {}
    Result(bool unapplicable, const std::string& msg, Iter begin, Iter end, const T& val = T())
        : ok(false), val(val), unapplicable(unapplicable), msg(msg), begin_(begin), end_(end) {}
    Result(const Result&) = default;
    Result& operator=(const Result&) = default;

    explicit operator bool() const { return ok; }

    Iter begin() const { return begin_; }
    Iter end()   const { return end_; }

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
    Iter begin_;
    Iter end_;
  };

  Parser(Iter begin, Iter end, Announcer* announcer)
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
        Is(Symbol(1), Token::kIdentifier, [this](const std::string& s) { return !kb_.IsRegistered(s); }) &&
        Is(Symbol(2), Token::kSemicolon)) {
      kb_.RegisterSort(Symbol(1).val.str());
      Advance(2);
      return Success<bool>(true);
    }
    if (Is(Symbol(0), Token::kVar) &&
        Is(Symbol(1), Token::kIdentifier, [this](const std::string& s) { return !kb_.IsRegistered(s); }) &&
        Is(Symbol(2), Token::kArrow) &&
        Is(Symbol(3), Token::kIdentifier, [this](const std::string& s) { return kb_.IsRegisteredSort(s); }) &&
        Is(Symbol(4), Token::kSemicolon)) {
      kb_.RegisterVar(Symbol(1).val.str(), Symbol(3).val.str());
      Advance(4);
      return Success<bool>(true);
    }
    if (Is(Symbol(0), Token::kName) &&
        Is(Symbol(1), Token::kIdentifier, [this](const std::string& s) { return !kb_.IsRegistered(s); }) &&
        Is(Symbol(2), Token::kArrow) &&
        Is(Symbol(3), Token::kIdentifier, [this](const std::string& s) { return kb_.IsRegisteredSort(s); }) &&
        Is(Symbol(4), Token::kSemicolon)) {
      kb_.RegisterName(Symbol(1).val.str(), Symbol(3).val.str());
      Advance(4);
      return Success<bool>(true);
    }
    if (Is(Symbol(0), Token::kFun) &&
        Is(Symbol(1), Token::kIdentifier, [this](const std::string& s) { return !kb_.IsRegistered(s); }) &&
        Is(Symbol(2), Token::kSlash) &&
        Is(Symbol(3), Token::kUint) &&
        Is(Symbol(4), Token::kArrow) &&
        Is(Symbol(5), Token::kIdentifier, [this](const std::string& s) { return kb_.IsRegisteredSort(s); }) &&
        Is(Symbol(6), Token::kSemicolon)) {
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
  Result<Term> term() {
    if (Is(Symbol(0), Token::kIdentifier, [this](const std::string& s) { return kb_.IsRegisteredVar(s); })) {
      Term x = kb_.LookupVar(Symbol(0).val.str());
      Advance(0);
      return Success(x);
    }
    if (Is(Symbol(0), Token::kIdentifier, [this](const std::string& s) { return kb_.IsRegisteredName(s); })) {
      Term n = kb_.LookupName(Symbol(0).val.str());
      Advance(0);
      return Success(n);
    }
    if (Is(Symbol(0), Token::kIdentifier, [this](const std::string& s) { return kb_.IsRegisteredFun(s); })) {
      class Symbol s = kb_.LookupFun(Symbol(0).val.str());
      Advance(0);
      Term::Vector args;
      if (s.arity() > 0 || Is(Symbol(0), Token::kLeftParen)) {
        if (!Is(Symbol(0), Token::kLeftParen)) {
          return Failure<Term>(MSG("Expected left parenthesis '('"));
        }
        Advance(0);
        for (Symbol::Arity i = 0; i < s.arity(); ++i) {
          if (i > 0) {
            if (!Is(Symbol(0), Token::kComma)) {
              return Failure<Term>(MSG("Expected comma ','"));
            }
            Advance(0);
          }
          Result<Term> t = term();
          if (!t) {
            return Failure<Term>(MSG("Expected argument term"), t);
          }
          args.push_back(t.val);
        }
        if (!Is(Symbol(0), Token::kRightParen)) {
          return Failure<Term>(MSG("Expected right parenthesis ')'"));
        }
        Advance(0);
      }
      return Success(kb_.solver().tf()->CreateTerm(s, args));
    }
    return Failure<Term>(MSG("Expected a term"));
  }

  // literal --> term [ '==' | '!=' ] term
  Result<Literal> literal() {
    Result<Term> lhs = term();
    if (!lhs) {
      return Failure<Literal>(MSG("Expected a lhs term"), lhs, DUMMY_LITERAL_);
    }
    bool pos;
    if (Is(Symbol(0), Token::kEqual) ||
        Is(Symbol(0), Token::kInequal)) {
      pos = Is(Symbol(0), Token::kEqual);
      Advance(0);
    } else {
      return Failure<Literal>(MSG("Expected equality or inequality '=='/'!='"), DUMMY_LITERAL_);
    }
    Result<Term> rhs = term();
    if (!rhs) {
      return Failure<Literal>(MSG("Expected rhs term"), rhs, DUMMY_LITERAL_);
    }
    Literal a = pos ? Literal::Eq(lhs.val, rhs.val) : Literal::Neq(lhs.val, rhs.val);
    return Success<Literal>(a);
  }

  // kb_clause --> () ;
  //            |  ( literal [, literal]* ) ;
  Result<bool> kb_clause() {
    if (!Is(Symbol(0), Token::kKB)) {
      return Unapplicable<bool>(MSG("No kb_clause"));
    }
    Advance(0);
    std::vector<Literal> ls;
    for (int i = 0; (i == 0 && Is(Symbol(0), Token::kLeftParen)) ||
                    (i >  0 && (Is(Symbol(0), Token::kComma) || Is(Symbol(0), Token::kOr))); ++i) {
      Advance(0);
      Result<Literal> a = literal();
      if (!a) {
        return Failure<bool>(MSG("Expected literal"), a);
      }
      ls.push_back(a.val);
    }
    if (!Is(Symbol(0), Token::kRightParen)) {
      return Failure<bool>(MSG("Expected right parenthesis ')'"));
    }
    Advance(0);
    if (!Is(Symbol(0), Token::kSemicolon)) {
      return Failure<bool>(MSG("Expected end of line ';'"));
    }
    Advance(0);
    const Clause c(ls.begin(), ls.end());
    if (!std::all_of(c.begin(), c.end(), [](Literal a) { return (!a.lhs().function() && !a.rhs().function()) || a.quasiprimitive(); })) {
      std::stringstream ss;
      ss << c;
      return Failure<bool>(MSG("KB clause "+ ss.str() +" must only contain ewff/quasiprimitive literals"));
    }
    kb_.solver().AddClause(c);
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
  Result<Formula> primary_formula(int binary_connective_recursion = 0) {
    Result<Literal> a(DUMMY_LITERAL_);
    Result<Formula> phi(DUMMY_FORMULA_);
    Result<Formula> psi(DUMMY_FORMULA_);
    Result<Term> x;
    if (Is(Symbol(0), Token::kNot)) {
      Advance(0);
      phi = primary_formula();
      if (!phi) {
        return Failure<Formula>(MSG("Expected a primary formula within negation"), phi, DUMMY_FORMULA_);
      }
      return Success<Formula>(Formula::Not(phi.val));
    }
    if (Is(Symbol(0), Token::kExists) || Is(Symbol(0), Token::kForall)) {
      bool ex = Is(Symbol(0), Token::kExists);
      Advance(0);
      x = term();
      if (!x.val.variable()) {
        return Failure<Formula>(MSG("Expected variable in quantifier"), x, DUMMY_FORMULA_);
      }
      phi = primary_formula();
      if (!phi) {
        return Failure<Formula>(MSG("Expected primary formula within quantifier"), phi, DUMMY_FORMULA_);
      }
      return Success<Formula>(ex ? Formula::Exists(x.val, phi.val)
                                 : Formula::Not(Formula::Exists(x.val, Formula::Not(phi.val))));
    }
    if (Is(Symbol(0), Token::kLeftParen)) {
      Advance(0);
      phi = formula();
      if (!phi) {
        return Failure<Formula>(MSG("Expected formula within brackets"), phi, DUMMY_FORMULA_);
      }
      if (!Is(Symbol(0), Token::kRightParen)) {
        return Failure<Formula>(MSG("Expected closing right parenthesis ')'"), DUMMY_FORMULA_);
      }
      Advance(0);
      return phi;
    }
    if (Is(Symbol(0), Token::kIdentifier) && kb_.IsRegisteredFormula(Symbol(0).val.str())) {
      std::string id = Symbol(0).val.str();
      Advance(0);
      return Success<Formula>(kb_.LookupFormula(id));
    }
    if ((a = literal())) {
      return Success<Formula>(Formula::Clause(Clause{a.val}));
    }
    return Failure<Formula>("Expected formula", DUMMY_FORMULA_);
  }

  // conjunctive_formula --> primary_formula && primary_formula
  Result<Formula> conjunctive_formula() {
    Result<Formula> phi = primary_formula();
    if (!phi) {
      return Failure<Formula>(MSG("Expected left conjunctive formula"), phi, DUMMY_FORMULA_);
    }
    while (Is(Symbol(0), Token::kAnd)) {
      Advance(0);
      Result<Formula> psi = primary_formula();
      if (!psi) {
        return Failure<Formula>(MSG("Expected left conjunctive formula"), psi, DUMMY_FORMULA_);
      }
      phi = Success<Formula>(Formula::Not(Formula::Or(Formula::Not(phi.val), Formula::Not(psi.val))));
    }
    return phi;
  }

  // disjunctive_formula --> conjunctive_formula || conjunctive_formula
  Result<Formula> disjunctive_formula() {
    Result<Formula> phi = conjunctive_formula();
    if (!phi) {
      return Failure<Formula>(MSG("Expected left argument conjunctive formula"), phi, DUMMY_FORMULA_);
    }
    while (Is(Symbol(0), Token::kOr)) {
      Advance(0);
      Result<Formula> psi = conjunctive_formula();
      if (!psi) {
        return Failure<Formula>(MSG("Expected right argument conjunctive formula"), psi, DUMMY_FORMULA_);
      }
      phi = Success<Formula>(Formula::Or(phi.val, psi.val));
    }
    return phi;
  }

  // formula --> disjunctive_formula
  Result<Formula> formula() {
    return disjunctive_formula();
  }

  // abbreviation --> let identifier := formula
  Result<bool> abbreviation() {
    if (!Is(Symbol(0), Token::kLet)) {
      return Unapplicable<bool>(MSG("Expected abbreviation operator 'let'"));
    }
    Advance(0);
    if (!Is(Symbol(0), Token::kIdentifier, [this](const std::string& s) { return !kb_.IsRegistered(s); })) {
      return Failure<bool>(MSG("Expected fresh identifier"));
    }
    const std::string id = Symbol(0).val.str();
    Advance(0);
    if (!Is(Symbol(0), Token::kAssign)) {
      return Failure<bool>(MSG("Expected assignment operator ':='"));
    }
    Advance(0);
    const Result<Formula> phi = formula();
    if (!phi) {
      return Failure<bool>(MSG("Expected formula"), phi);
    }
    if (!Is(Symbol(0), Token::kSemicolon)) {
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
    Result<Formula> phi = formula();
    if (!phi) {
      return Failure<bool>(MSG("Expected query formula"));
    }
    if (!Is(Symbol(0), Token::kRightParen)) {
      return Failure<bool>(MSG("Expected right parenthesis ')'"));
    }
    Advance(0);
    if (!Is(Symbol(0), Token::kSemicolon)) {
      return Failure<bool>(MSG("Expected end of line ';'"));
    }
    Advance(0);
    const Formula phi_nf = phi.val.reader().NF();
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

  // start --> declarations kb_clauses
  Result<bool> start() {
    Result<bool> r;
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
    return begin_ == end_ ? Success<bool>(true) : Failure<bool>(MSG("Unparsed input"));
  }

  bool Is(const internal::Maybe<Token>& symbol, Token::Id id) const {
    return symbol && symbol.val.id() == id;
  }

  template<typename UnaryPredicate>
  bool Is(const internal::Maybe<Token>& symbol, Token::Id id, UnaryPredicate p) const {
    return symbol && symbol.val.id() == id && p(symbol.val.str());
  }

  internal::Maybe<Token> Symbol(int N = 0) const {
    auto it = begin_;
    for (int i = 0; i < N && it != end_; ++i) {
      assert(begin_ != end_);
      ++it;
    }
    return it != end_ ? internal::Just(*it) : internal::Nothing;
  }

  void Advance(int N = 0) {
    for (int i = 0; i <= N; ++i) {
      assert(begin_ != end_);
      ++begin_;
    }
  }

  Lex lexer_;
  iterator begin_;
  iterator end_;
  KB kb_;
  Announcer* announcer_;
  const Literal DUMMY_LITERAL_ = Literal::Eq(kb_.solver().tf()->CreateTerm(kb_.solver().sf()->CreateName(0)),
                                             kb_.solver().tf()->CreateTerm(kb_.solver().sf()->CreateName(0)));
  const Formula DUMMY_FORMULA_ = Formula::Clause(Clause{});
};

std::ostream& operator<<(std::ostream& os, Token::Id t) {
  switch (t) {
    case Token::kSort:       return os << "kSort";
    case Token::kVar:        return os << "kVar";
    case Token::kName:       return os << "kName";
    case Token::kFun:        return os << "kFun";
    case Token::kKB:         return os << "kKB";
    case Token::kLet:        return os << "kLet";
    case Token::kEntails:    return os << "kEntails";
    case Token::kConsistent: return os << "kConsistent";
    case Token::kColon:      return os << "kColon";
    case Token::kSemicolon:  return os << "kSemicolon";
    case Token::kComma:      return os << "kComma";
    case Token::kEqual:      return os << "kEqual";
    case Token::kInequal:    return os << "kInequal";
    case Token::kNot:        return os << "kNot";
    case Token::kOr:         return os << "kOr";
    case Token::kAnd:        return os << "kAnd";
    case Token::kForall:     return os << "kForall";
    case Token::kExists:     return os << "kExists";
    case Token::kArrow:      return os << "kArrow";
    case Token::kAssign:     return os << "kAssign";
    case Token::kSlash:      return os << "kSlash";
    case Token::kLeftParen:  return os << "(";
    case Token::kRightParen: return os << ")";
    case Token::kUint:       return os << "kUint";
    case Token::kIdentifier: return os << "kIdentifier";
    case Token::kError:      return os << "kError";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Token& t) {
  return os << "Token(" << t.id() << "," << t.str() << ")";
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const Parser<std::string::const_iterator>::Result<T>& r) {
  return os << r.to_string();
}

inline void parse_helper(const char* c_str) {
  struct PrintAnnouncer : public Announcer {
    void AnnounceEntailment(int k, const Setup& s, const Formula& phi, bool yes) override {
      std::cout << "Entails(" << k << ", " << phi << ") = " << std::boolalpha << yes << std::endl;
    }

    void AnnounceConsistency(int k, const Setup& s, const Formula& phi, bool yes) override {
      std::cout << "Consistent(" << k << ", " << phi << ") = " << std::boolalpha << yes << std::endl;
    }
  };
  std::string str = c_str;
  typedef Parser<std::string::const_iterator> StrParser;
  PrintAnnouncer announcer;
  StrParser parser(str.begin(), str.end(), &announcer);
  std::cout << parser.Parse() << std::endl;
}

int main() {
  std::string s = "Sort BOOL;"
                  "Sort HUMAN;"
                  "Var x -> HUMAN;"
                  "Variable y -> HUMAN;"
                  "Name F -> BOOL;"
                  "Name T -> BOOL;"
                  "Name Jesus -> HUMAN;"
                  "Name Mary -> HUMAN;"
                  "Name Joe -> HUMAN;"
                  "Name HolyGhost -> HUMAN;"
                  "Name God -> HUMAN;"
                  "Function dummy / 0 -> HUMAN;"
                  "Function fatherOf / 1 -> HUMAN;"
                  "Function motherOf/1 -> HUMAN;"
                  "KB (Mary == motherOf(Jesus));"
                  "KB (x != Mary || x == motherOf(Jesus));"
                  "KB (HolyGhost == fatherOf(Jesus) || God == fatherOf(Jesus) || Joe == fatherOf(Jesus));"
                  "Let phi := HolyGhost == fatherOf(Jesus) || God == fatherOf(Jesus) || Joe == fatherOf(Jesus);"
                  "Let psi := HolyGhost == fatherOf(Jesus) && God == fatherOf(Jesus) || Joe == fatherOf(Jesus);"
                  "Let xi := Ex x (x == fatherOf(Jesus));"
                  "Entails (0, phi);"
                  "Entails (0, psi);"
                  "Entails (0, xi);"
                  "Entails (1, xi);"
                  "";
  //typedef Lexer<std::string::const_iterator> StrLexer;
  //StrLexer lexer(s.begin(), s.end());
  //for (const Token& t : lexer) {
  //  if (t.id() == Token::kError) {
  //    std::cout << "ERROR ";
  //  }
  //  std::cout << t.str() << " ";
  //}
  //std::cout << std::endl;
  //std::cout << std::endl;
  //typedef Parser<std::string::const_iterator> StrParser;
  //PrintAnnouncer announcer;
  //StrParser parser(s.begin(), s.end(), &announcer);
  //std::cout << parser.Parse() << std::endl;
  parse_helper(s.c_str());
  return 0;
}

