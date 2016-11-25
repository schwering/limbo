// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <cassert>

#include <algorithm>
#include <iostream>
#include <list>
#include <functional>
#include <string>
#include <utility>

//#include <lela/solver.h>
//#include <lela/internal/maybe.h>
//#include <lela/format/output.h>
//#include <lela/format/syntax.h>
//
//using namespace lela;
//using namespace lela::format;
//
//struct Entailment {
//  Entailment() : context_(solver_.sf(), solver_.tf()) {}
//  ~Entailment() {
//    for (auto p : funs_) {
//      if (p.second) {
//        delete p.second;
//      }
//    }
//  }
//
//  void RegisterSort(const std::string& id) {
//    const Symbol::Sort sort = context_.NewSort();
//    lela::format::RegisterSort(sort, id);
//    sorts_[id] = sort;
//    std::cout << "RegisterSort " << id << std::endl;
//  }
//
//  Symbol::Sort LookupSort(const std::string& id) const {
//    const auto it = sorts_.find(id);
//    if (it == sorts_.end())
//      throw undeclared_error(id);
//    return it->second;
//  }
//
//  void RegisterVar(const boost::fusion::vector<std::string, std::string>& d) {
//    const std::string id = boost::fusion::at_c<0>(d);
//    const std::string sort_id = boost::fusion::at_c<1>(d);
//    if (vars_.find(id) != vars_.end() || names_.find(id) != names_.end() || funs_.find(id) != funs_.end())
//      throw redeclared_error(id);
//    const Symbol::Sort sort = LookupSort(sort_id);
//    const Term var = context_.NewVar(sort);
//    vars_[id] = var;
//    lela::format::RegisterSymbol(var.symbol(), id);
//    std::cout << "RegisterVar " << id << " / " << sort_id << std::endl;
//  }
//
//  Term LookupVar(const std::string& id) const {
//    const auto it = vars_.find(id);
//    if (it == vars_.end())
//      throw undeclared_error(id);
//    return it->second;
//  }
//
//  void RegisterName(const boost::fusion::vector<std::string, std::string>& d) {
//    const std::string id = boost::fusion::at_c<0>(d);
//    const std::string sort_id = boost::fusion::at_c<1>(d);
//    if (vars_.find(id) != vars_.end() || names_.find(id) != names_.end() || funs_.find(id) != funs_.end())
//      throw redeclared_error(id);
//    const Symbol::Sort sort = LookupSort(sort_id);
//    const Term name = context_.NewName(sort);
//    names_[id] = name;
//    lela::format::RegisterSymbol(name.symbol(), id);
//    std::cout << "RegisterName " << id << " / " << sort_id << std::endl;
//  }
//
//  Term LookupName(const std::string& id) const {
//    const auto it = names_.find(id);
//    if (it == names_.end())
//      throw undeclared_error(id);
//    return it->second;
//  }
//
//  const Symbol& LookupFun(const std::string& id) const {
//    const auto it = funs_.find(id);
//    if (it == funs_.end())
//      throw undeclared_error(id);
//    return *it->second;
//  }
//
//  void RegisterFun(const boost::fusion::vector<std::string, int, std::string>& d) {
//    const std::string id = boost::fusion::at_c<0>(d);
//    const std::string sort_id = boost::fusion::at_c<2>(d);
//    const int arity = boost::fusion::at_c<1>(d);
//    if (vars_.find(id) != vars_.end() || names_.find(id) != names_.end() || funs_.find(id) != funs_.end())
//      throw redeclared_error(id);
//    const Symbol::Sort sort = sorts_[sort_id];
//    const Symbol* symbol = new Symbol(context_.NewFun(sort, arity));
//    funs_[id] = symbol;
//    lela::format::RegisterSymbol(*symbol, id);
//    std::cout << "RegisterFun " << id << " / " << arity << " / " << sort_id << std::endl;
//  }
//
// private:
//  std::map<std::string, Symbol::Sort>  sorts_;
//  std::map<std::string, Term>          vars_;
//  std::map<std::string, Term>          names_;
//  std::map<std::string, const Symbol*> funs_;
//  Solver  solver_;
//  Context context_;
//};

struct syntax_error : public std::exception {
  syntax_error(const std::string& id) : id_(id) {}
  virtual const char* what() const noexcept override { return id_.c_str(); }
 private:
  std::string id_;
};
struct redeclared_error : public syntax_error { using syntax_error::syntax_error; };
struct undeclared_error : public syntax_error { using syntax_error::syntax_error; };

template<typename Iter>
class Lexer {
 public:
  enum Match { kMismatch, kPrefixMatch, kFullMatch };
  enum TokenId { kSort, kVar, kName, kFun, kColon, kSlash, kEOL, kUint, kIdentifier, kEOF, kError };
  typedef std::list<std::pair<TokenId, std::function<Match(Iter, Iter)>>> LexemeVector;

  class Token {
   public:
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
    //typedef lela::internal::iterator_proxy<iterator> proxy;

    iterator(const LexemeVector* lexemes, Iter it, Iter end) : lexemes_(lexemes), it_(it), end_(end) {}

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

   private:
    std::pair<Iter, Iter> NextWord() const {
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
        if ((best_match == kMismatch && m == kPrefixMatch) || (best_match == kMismatch && m == kFullMatch)) {
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
    lexemes_.push_back(std::make_pair(kSlash,      [](Iter begin, Iter end) { return IsPrefix(begin, end, "/"); }));
    lexemes_.push_back(std::make_pair(kEOL,        [](Iter begin, Iter end) { return IsPrefix(begin, end, ";"); }));
    lexemes_.push_back(std::make_pair(kUint,       [](Iter begin, Iter end) { return begin == end ? kPrefixMatch : *begin != '0' && std::all_of(begin, end, [](char c) { return IsDigit(c); }) ? kFullMatch : kMismatch; }));
    lexemes_.push_back(std::make_pair(kIdentifier, [](Iter begin, Iter end) { return begin == end ? kPrefixMatch : IsAlpha(*begin) && std::all_of(begin, end, [](char c) { return IsAlnum(c); }) ? kFullMatch : kMismatch; }));
  }

  iterator begin() const { return iterator(&lexemes_, begin_, end_); }
  iterator end()   const { return iterator(&lexemes_, end_, end_); }

 private:
  LexemeVector lexemes_;

  static Match IsPrefix(Iter begin, Iter end, const std::string& foobar) {
    size_t len = std::distance(begin, end);
    if (len > foobar.length() || std::mismatch(begin, end, foobar.begin()).first != end)
      return kMismatch;
    else
      return len < foobar.length() ? kPrefixMatch : kFullMatch;
  }
  static bool IsWhitespace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
  static bool IsDigit(char c) { return '0' <= c && c <= '9'; }
  static bool IsAlpha(char c) { return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || c == '_'; }
  static bool IsAlnum(char c) { return IsAlpha(c) || IsDigit(c); }

  Iter begin_;
  Iter end_;
};

int main() {
  const std::string s = "BOOL    :    sort;"\
                        "x : variable BOOL;"\
                        "y : variable BOOL;"\
                        "HUMAN:sort;"\
                        "T : name BOOL;"\
                        "F : name BOOL;"\
                        "fatherOf : function / 3 HUMAN;"\
                        "fatherOf2 : function/3 HUMAN;";
  typedef Lexer<std::string::const_iterator> StrLexer;
  for (const StrLexer::Token& t : StrLexer(s.begin(), s.end())) {
    if (t.id() == StrLexer::kError) {
      std::cout << "ERROR ";
    }
    std::cout << t.str() << " ";
  }
  std::cout << std::endl;
}
 


