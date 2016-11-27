// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering
//
// Max-munch lexer for our text interface.

#ifndef EXAMPLES_CLI_LEXER_H_
#define EXAMPLES_CLI_LEXER_H_

#include <cassert>

#include <algorithm>
#include <functional>
#include <list>
#include <string>
#include <utility>

#include <lela/internal/iter.h>

class Token {
 public:
  enum Id { kError, kSort, kVar, kName, kFun, kKB, kLet, kEntails, kConsistent, kColon, kComma, kSemicolon, kEqual,
    kInequal, kNot, kOr, kAnd, kForall, kExists, kAssign, kRArrow, kLRArrow, kSlash, kSlashAst, kAstSlash,
    kLeftParen, kRightParen, kUint, kIdentifier };

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
    iterator(const LexemeVector* lexemes, Iter it, Iter end) : lexemes_(lexemes), it_(it), end_(end) {
      SkipWhitespace();
    }
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
      SkipWhitespace();
      return *this;
    }

    proxy operator->() const { return proxy(operator*()); }
    proxy operator++(int) { proxy p(operator*()); operator++(); return p; }

    Iter char_iter() const { return it_; }

   private:
    void SkipWhitespace() {
      for (; it_ != end_ && IsWhitespace(*it_); ++it_) {
      }
    }

    std::pair<Iter, Iter> NextWord() const {
      // Max-munch lexer
      assert(it_ != end_);
      Iter it = it_;
      Iter jt;
      std::pair<Match, Token::Id> m;
      for (jt = it; jt != end_; ++jt) {
        m = LexemeMatch(it, std::next(jt));
        if (m.first == kMismatch) {
          break;
        }
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
    lexemes_.push_back(std::make_pair(Token::kRArrow,     [](Iter begin, Iter end) { return IsPrefix(begin, end, "->"); }));
    lexemes_.push_back(std::make_pair(Token::kLRArrow,    [](Iter begin, Iter end) { return IsPrefix(begin, end, "<->"); }));
    lexemes_.push_back(std::make_pair(Token::kSlash,      [](Iter begin, Iter end) { return IsPrefix(begin, end, "/"); }));
    lexemes_.push_back(std::make_pair(Token::kSlashAst,   [](Iter begin, Iter end) { return IsPrefix(begin, end, "/*"); }));
    lexemes_.push_back(std::make_pair(Token::kAstSlash,   [](Iter begin, Iter end) { return IsPrefix(begin, end, "*/"); }));
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
    auto r = lela::internal::transform_range(begin, end, [](char c) { return std::tolower(c); });
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
    case Token::kRArrow:     return os << "kRArrow";
    case Token::kLRArrow:    return os << "kLRArrow";
    case Token::kAssign:     return os << "kAssign";
    case Token::kSlash:      return os << "kSlash";
    case Token::kSlashAst:   return os << "kSlashAst";
    case Token::kAstSlash:   return os << "kAstSlash";
    case Token::kLeftParen:  return os << "kLeftParen";
    case Token::kRightParen: return os << "kRightParen";
    case Token::kUint:       return os << "kUint";
    case Token::kIdentifier: return os << "kIdentifier";
    case Token::kError:      return os << "kError";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Token& t) {
  return os << "Token(" << t.id() << "," << t.str() << ")";
}

#endif // EXAMPLES_LEXER_H_

