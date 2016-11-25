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
#define MSG(msg)      (std::string(__FUNCTION__) +":"+ S__LINE__ +": " + msg)

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

struct Entailment {
  Entailment() : context_(solver_.sf(), solver_.tf()) {}
  ~Entailment() {
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

  bool IsRegistered(const std::string& id) const {
    return IsRegisteredSort(id) || IsRegisteredVar(id) || IsRegisteredName(id) || IsRegisteredFun(id);
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

  void RegisterSort(const std::string& id) {
    const Symbol::Sort sort = context_.NewSort();
    lela::format::RegisterSort(sort, id);
    sorts_[id] = sort;
    std::cout << "RegisterSort " << id << std::endl;
  }

  void RegisterVar(const std::string& id, const std::string& sort_id) {
    if (vars_.find(id) != vars_.end() || names_.find(id) != names_.end() || funs_.find(id) != funs_.end())
      throw redeclared_error(id);
    const Symbol::Sort sort = LookupSort(sort_id);
    const Term var = context_.NewVar(sort);
    vars_[id] = var;
    lela::format::RegisterSymbol(var.symbol(), id);
    std::cout << "RegisterVar " << id << " / " << sort_id << std::endl;
  }

  void RegisterName(const std::string& id, const std::string& sort_id) {
    if (vars_.find(id) != vars_.end() || names_.find(id) != names_.end() || funs_.find(id) != funs_.end())
      throw redeclared_error(id);
    const Symbol::Sort sort = LookupSort(sort_id);
    const Term name = context_.NewName(sort);
    names_[id] = name;
    lela::format::RegisterSymbol(name.symbol(), id);
    std::cout << "RegisterName " << id << " / " << sort_id << std::endl;
  }

  void RegisterFun(const std::string& id, Symbol::Arity arity, const std::string& sort_id) {
    if (vars_.find(id) != vars_.end() || names_.find(id) != names_.end() || funs_.find(id) != funs_.end())
      throw redeclared_error(id);
    const Symbol::Sort sort = sorts_[sort_id];
    funs_.emplace(id, context_.NewFun(sort, arity));
    Symbol s = funs_.find(id)->second;
    lela::format::RegisterSymbol(s, id);
    std::cout << "RegisterFun " << id << " / " << static_cast<int>(arity) << " / " << sort_id << std::endl;
  }

  Solver& solver() { return solver_; }
  const Solver& solver() const { return solver_; }

 private:
  std::map<std::string, Symbol::Sort> sorts_;
  std::map<std::string, Term>         vars_;
  std::map<std::string, Term>         names_;
  std::map<std::string, Symbol>       funs_;
  Solver  solver_;
  Context context_;
};

template<typename Iter>
class Lexer {
 public:
  enum Match { kMismatch, kPrefixMatch, kFullMatch };
  enum TokenId { kSort, kVar, kName, kFun, kColon, kComma, kEqual, kInequal, kLogicNot, kLogicOr, kLogicAnd,
    kForall, kExists, kArrow, kSlash, kLeftParen, kRightParen, kEOL, kUint, kIdentifier, kEOF, kError };
  typedef std::list<std::pair<TokenId, std::function<Match(Iter, Iter)>>> LexemeVector;

  class Token {
   public:
    Token() : id_(kError) {}
    explicit Token(TokenId id) : id_(id) {}
    Token(TokenId id, const std::string& str) : id_(id), str_(str) {}
    TokenId id() const { return id_; }
    const std::string& str() const { return str_; }
   private:
    TokenId id_;
    std::string str_;
  };

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
      std::pair<Match, TokenId> match = LexemeMatch(substr.first, substr.second);
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

    std::pair<Match, TokenId> LexemeMatch(Iter begin, Iter end) const {
      Match best_match = kMismatch;
      TokenId best_token = kError;
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
    lexemes_.push_back(std::make_pair(kSort,       [](Iter begin, Iter end) { return IsPrefix(begin, end, "sort"); }));
    lexemes_.push_back(std::make_pair(kVar,        [](Iter begin, Iter end) { return std::max( IsPrefix(begin, end, "var"), IsPrefix(begin, end, "variable") ); }));
    lexemes_.push_back(std::make_pair(kName,       [](Iter begin, Iter end) { return std::max( IsPrefix(begin, end, "name"), IsPrefix(begin, end, "stdname") ); }));
    lexemes_.push_back(std::make_pair(kFun,        [](Iter begin, Iter end) { return std::max( IsPrefix(begin, end, "fun"), IsPrefix(begin, end, "function") ); }));
    lexemes_.push_back(std::make_pair(kColon,      [](Iter begin, Iter end) { return IsPrefix(begin, end, ":"); }));
    lexemes_.push_back(std::make_pair(kComma,      [](Iter begin, Iter end) { return IsPrefix(begin, end, ","); }));
    lexemes_.push_back(std::make_pair(kEqual,      [](Iter begin, Iter end) { return IsPrefix(begin, end, "=="); }));
    lexemes_.push_back(std::make_pair(kInequal,    [](Iter begin, Iter end) { return IsPrefix(begin, end, "!="); }));
    lexemes_.push_back(std::make_pair(kLogicNot,   [](Iter begin, Iter end) { return IsPrefix(begin, end, "!"); }));
    lexemes_.push_back(std::make_pair(kLogicOr,    [](Iter begin, Iter end) { return IsPrefix(begin, end, "||"); }));
    lexemes_.push_back(std::make_pair(kLogicAnd,   [](Iter begin, Iter end) { return IsPrefix(begin, end, "&&"); }));
    lexemes_.push_back(std::make_pair(kForall,     [](Iter begin, Iter end) { return IsPrefix(begin, end, "Fa"); }));
    lexemes_.push_back(std::make_pair(kExists,     [](Iter begin, Iter end) { return IsPrefix(begin, end, "Ex"); }));
    lexemes_.push_back(std::make_pair(kArrow,      [](Iter begin, Iter end) { return IsPrefix(begin, end, "->"); }));
    lexemes_.push_back(std::make_pair(kSlash,      [](Iter begin, Iter end) { return IsPrefix(begin, end, "/"); }));
    lexemes_.push_back(std::make_pair(kLeftParen,  [](Iter begin, Iter end) { return IsPrefix(begin, end, "("); }));
    lexemes_.push_back(std::make_pair(kRightParen, [](Iter begin, Iter end) { return IsPrefix(begin, end, ")"); }));
    lexemes_.push_back(std::make_pair(kEOL,        [](Iter begin, Iter end) { return IsPrefix(begin, end, ";"); }));
    lexemes_.push_back(std::make_pair(kUint,       [](Iter begin, Iter end) { return begin == end ? kPrefixMatch : (*begin != '0' || std::next(begin) == end) && std::all_of(begin, end, [](char c) { return IsDigit(c); }) ? kFullMatch : kMismatch; }));
    lexemes_.push_back(std::make_pair(kIdentifier, [](Iter begin, Iter end) { return begin == end ? kPrefixMatch : IsAlpha(*begin) && std::all_of(begin, end, [](char c) { return IsAlnum(c); }) ? kFullMatch : kMismatch; }));
  }

  iterator begin() const { return iterator(&lexemes_, begin_, end_); }
  iterator end()   const { return iterator(&lexemes_, end_, end_); }

 private:
  LexemeVector lexemes_;

  static Match IsPrefix(Iter begin, Iter end, const std::string& foobar) {
    size_t len = std::distance(begin, end);
    if (len > foobar.length() || std::mismatch(begin, end, foobar.begin()).first != end) {
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

//template<typename T>
std::ostream& operator<<(std::ostream& os, typename Lexer<std::string::const_iterator>::TokenId t) {
  typedef Lexer<std::string::const_iterator> Lex;
  switch (t) {
    case Lex::kSort:       return os << "kSort";
    case Lex::kVar:        return os << "kVar";
    case Lex::kName:       return os << "kName";
    case Lex::kFun:        return os << "kFun";
    case Lex::kColon:      return os << "kColon";
    case Lex::kComma:      return os << "kComma";
    case Lex::kEqual:      return os << "kEqual";
    case Lex::kInequal:    return os << "kInequal";
    case Lex::kLogicNot:   return os << "kLogicNot";
    case Lex::kLogicOr:    return os << "kLogicOr";
    case Lex::kLogicAnd:   return os << "kLogicAnd";
    case Lex::kForall:     return os << "kForall";
    case Lex::kExists:     return os << "kExists";
    case Lex::kArrow:      return os << "kArrow";
    case Lex::kSlash:      return os << "kSlash";
    case Lex::kLeftParen:  return os << "(";
    case Lex::kRightParen: return os << ")";
    case Lex::kEOL:        return os << "kEOL";
    case Lex::kUint:       return os << "kUint";
    case Lex::kIdentifier: return os << "kIdentifier";
    case Lex::kEOF:        return os << "kEOF";
    case Lex::kError:      return os << "kError";
  }
  return os;
}

//template<typename T>
std::ostream& operator<<(std::ostream& os, typename Lexer<std::string::const_iterator>::Token t) {
  return os << "Token(" << t.id() << "," << t.str() << ")";
}

template<typename Iter>
class Parser {
 public:
  typedef Lexer<Iter> Lex;
  typedef typename Lex::iterator iterator;
  typedef typename Lex::TokenId TokenId;
  typedef typename Lex::Token Token;

  template<typename T>
  struct Result {
    Result() = default;
    explicit Result(const T& val) : ok(true), val(val) {}
    explicit Result(T&& val) : ok(true), val(std::forward<T>(val)) {}  // NOLINT
    Result(const std::string& msg, Iter begin, Iter end) : ok(false), msg(msg), begin_(begin), end_(end) {}
    Result(const Result&) = default;
    Result& operator=(const Result&) = default;

    explicit operator bool() const { return ok; }

    Iter begin() const { return begin_; }
    Iter end()   const { return end_; }

    std::string str() const {
      std::stringstream ss;
      if (ok) {
        ss << "Success(" << val << ")";
      } else {
        ss << "Failure(" << msg << ", \"" << std::string(begin(), end()) << "\")";
      }
      return ss.str();
    }

    bool ok;
    T val;
    std::string msg;

   private:
    template<typename U>
    friend std::ostream& operator<<(std::ostream& os, const Result<U>& r);

    Iter begin_;
    Iter end_;
  };

  Parser(Iter begin, Iter end) : lexer_(begin, end), begin_(lexer_.begin()), end_(lexer_.end()) {}

  Result<bool> Parse() { return start(); }

 private:
  template<typename T>
  Result<T> Success(const T& result) {
    return Result<T>(result);
  }

  template<typename T>
  Result<T> Failure(const std::string& msg) const {
    return Result<T>(msg, begin_.char_iter(), end_.char_iter());
  }

  template<typename T, typename U>
  static Result<T> Failure(const std::string& msg, const Result<U>& r) {
    return Result<T>(msg + " [because] " + r.msg, r.begin(), r.end());
  }

  // declaration --> sort <sort-id> ;
  //              |  var <id> => <sort-id> ;
  //              |  name <id> => <sort-id> ;
  //              |  fun <id> / <arity> => <sort-id> ;
  Result<bool> declaration() {
    if (Is(Symbol(0), Lex::kSort) &&
        Is(Symbol(1), Lex::kIdentifier, [this](const std::string& s) { return !e_.IsRegistered(s); }) &&
        Is(Symbol(2), Lex::kEOL)) {
      e_.RegisterSort(Symbol(1).val.str());
      Advance(2);
      return Success<bool>(true);
    }
    if (Is(Symbol(0), Lex::kVar) &&
        Is(Symbol(1), Lex::kIdentifier, [this](const std::string& s) { return !e_.IsRegistered(s); }) &&
        Is(Symbol(2), Lex::kArrow) &&
        Is(Symbol(3), Lex::kIdentifier, [this](const std::string& s) { return e_.IsRegisteredSort(s); }) &&
        Is(Symbol(4), Lex::kEOL)) {
      e_.RegisterVar(Symbol(1).val.str(), Symbol(3).val.str());
      Advance(4);
      return Success<bool>(true);
    }
    if (Is(Symbol(0), Lex::kName) &&
        Is(Symbol(1), Lex::kIdentifier, [this](const std::string& s) { return !e_.IsRegistered(s); }) &&
        Is(Symbol(2), Lex::kArrow) &&
        Is(Symbol(3), Lex::kIdentifier, [this](const std::string& s) { return e_.IsRegisteredSort(s); }) &&
        Is(Symbol(4), Lex::kEOL)) {
      e_.RegisterName(Symbol(1).val.str(), Symbol(3).val.str());
      Advance(4);
      return Success<bool>(true);
    }
    if (Is(Symbol(0), Lex::kFun) &&
        Is(Symbol(1), Lex::kIdentifier, [this](const std::string& s) { return !e_.IsRegistered(s); }) &&
        Is(Symbol(2), Lex::kSlash) &&
        Is(Symbol(3), Lex::kUint) &&
        Is(Symbol(4), Lex::kArrow) &&
        Is(Symbol(5), Lex::kIdentifier, [this](const std::string& s) { return e_.IsRegisteredSort(s); }) &&
        Is(Symbol(6), Lex::kEOL)) {
      e_.RegisterFun(Symbol(1).val.str(), std::stoi(Symbol(3).val.str()), Symbol(5).val.str());
      Advance(6);
      return Success<bool>(true);
    }
    return Failure<bool>(MSG("No declaration found"));
  }

  // declarations --> declarations*
  Result<bool> declarations() {
    while (declaration()) {
    }
    return Success<bool>(true);
  }

  // term --> x
  //       |  n
  //       |  f
  //       |  f(term, ..., term)
  Result<Term> term() {
    if (Is(Symbol(0), Lex::kIdentifier, [this](const std::string& s) { return e_.IsRegisteredVar(s); })) {
      Term x = e_.LookupVar(Symbol(0).val.str());
      Advance(0);
      return Success(x);
    }
    if (Is(Symbol(0), Lex::kIdentifier, [this](const std::string& s) { return e_.IsRegisteredName(s); })) {
      Term n = e_.LookupName(Symbol(0).val.str());
      Advance(0);
      return Success(n);
    }
    if (Is(Symbol(0), Lex::kIdentifier, [this](const std::string& s) { return e_.IsRegisteredFun(s); })) {
      class Symbol s = e_.LookupFun(Symbol(0).val.str());
      Advance(0);
      Term::Vector args;
      if (s.arity() > 0 || Is(Symbol(0), Lex::kLeftParen)) {
        if (!Is(Symbol(0), Lex::kLeftParen)) {
          return Failure<Term>(MSG("Expected left parenthesis"));
        }
        Advance(0);
        for (Symbol::Arity i = 0; i < s.arity(); ++i) {
          if (i > 0) {
            if (!Is(Symbol(0), Lex::kComma)) {
              return Failure<Term>(MSG("Expected comma"));
            }
            Advance(0);
          }
          Result<Term> t = term();
          if (!t) {
            return Failure<Term>(MSG("Expected argument term"), t);
          }
          args.push_back(t.val);
        }
        if (!Is(Symbol(0), Lex::kRightParen)) {
          return Failure<Term>(MSG("Expected right parenthesis"));
        }
        Advance(0);
      }
      return Success(e_.solver().tf()->CreateTerm(s, args));
    }
    return Failure<Term>(MSG("Expected a term"));
  }

  // literal --> term [ '==' | '!=' ] term
  Result<Clause> literal() {
    Result<Term> lhs = term();
    if (!lhs) {
      return Failure<Clause>(MSG("Expected a lhs term"), lhs);
    }
    bool pos;
    if (Is(Symbol(0), Lex::kEqual) ||
        Is(Symbol(0), Lex::kInequal)) {
      pos = Is(Symbol(0), Lex::kEqual);
      Advance(0);
    } else {
      return Failure<Clause>(MSG("Expected equality or inequality"));
    }
    Result<Term> rhs = term();
    if (!rhs) {
      return Failure<Clause>(MSG("Expected rhs term"), rhs);
    }
    Literal a = pos ? Literal::Eq(lhs.val, rhs.val) : Literal::Neq(lhs.val, rhs.val);
    return Success<Clause>(Clause{a});
  }

  // clause --> ()
  //         |  ( literal [, literal]* )
  Result<Clause> clause() {
    std::vector<Literal> ls;
    if (!Is(Symbol(0), Lex::kLeftParen)) {
      return Failure("Expected left parenthesis");
    }
    Advance(0);
    for (int i = 0; ; ++i) {
      if (i != 0) {
        if (!Is(Symbol(0), Lex::kComma)) {
          return Failure("Expected comma");
        }
        Advance(0);
      }
      Result<Clause> c = literal();
      if (!c) {
        return Failure("Expected literal", c);
      }
      ls.insert(ls.end(), c.val.begin(), c.val.end());
    }
    if (!Is(Symbol(0), Lex::kRightParen)) {
      return Failure("Expected right parenthesis");
    }
    Advance(0);
    return Success(Clause(ls.begin(), ls.end()));
  }

  // start --> declarations
  Result<bool> start() {
    Result<bool> r;
    r = declarations();
    if (!r) {
      return Failure<bool>(MSG("Declarations failed"), r);
    }
    std::cout << literal().str() << std::endl;
    return r;
  }

  bool Is(const internal::Maybe<Token>& symbol, TokenId id) const {
    return symbol && symbol.val.id() == id;
  }

  template<typename UnaryPredicate>
  bool Is(const internal::Maybe<Token>& symbol, TokenId id, UnaryPredicate p) const {
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
  Entailment e_;
};

template<typename T>
std::ostream& operator<<(std::ostream& os, const Parser<std::string::const_iterator>::Result<T>& r) {
  return os << r.str();
}

int main() {
  std::string s = "sort BOOL;"\
                  "var x -> BOOL;"\
                  "sort HUMAN;"\
                  "variable y -> HUMAN;"\
                  "name F -> BOOL;"\
                  "name T -> BOOL;"\
                  "function dummy / 0 -> HUMAN;"\
                  "function fatherOf / 3 -> HUMAN;"\
                  "function fatherOf2/3 -> HUMAN;"\
                  "y == fatherOf(dummy(), dummy,x,z)";
  typedef Lexer<std::string::const_iterator> StrLexer;
  typedef Parser<std::string::const_iterator> StrParser;
  StrLexer lexer(s.begin(), s.end());
  for (const StrLexer::Token& t : lexer) {
    if (t.id() == StrLexer::kError) {
      std::cout << "ERROR ";
    }
    std::cout << t.str() << " ";
  }
  std::cout << std::endl;
  std::cout << std::endl;

  StrParser parser(s.begin(), s.end());
  std::cout << parser.Parse() << std::endl;
  std::cout << std::endl;
  return 0;
}

