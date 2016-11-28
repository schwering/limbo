// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering
//
// Implements a simple language to specify entailment problems.

#include <algorithm>
#include <iostream>
#include <iterator>
#include <list>
#include <functional>
#include <string>
#include <sstream>
#include <utility>

#if 0
#include <lela/setup.h>
#include <lela/formula.h>
#include <lela/format/output.h>

#include "lexer.h"
#include "kb.h"
#include "parser.h"

using lela::format::operator<<;

// Just for debugging purposes.
template<typename Iter>
static void lex(Iter begin, Iter end) {
  Lexer<Iter> lexer(begin, end);
  for (const Token& t : lexer) {
    if (t.id() == Token::kError) {
      std::cout << "ERROR ";
    }
    std::cout << t.str() << " ";
  }
}

template<typename Iter>
static bool parse(Iter begin, Iter end) {
  struct PrintAnnouncer : public Announcer {
    void AnnounceEntailment(int k, const lela::Setup& s, const lela::Formula& phi, bool yes) override {
      std::cout << "Entails(" << k << ", " << phi << ") = " << std::boolalpha << yes << std::endl;
    }

    void AnnounceConsistency(int k, const lela::Setup& s, const lela::Formula& phi, bool yes) override {
      std::cout << "Consistent(" << k << ", " << phi << ") = " << std::boolalpha << yes << std::endl;
    }
  };
  PrintAnnouncer announcer;
  typedef Parser<Iter> MyParser;
  MyParser parser(begin, end, &announcer);
  auto r = parser.Parse();
  std::cout << r << std::endl;
  return bool(r);
}

#endif

int main(int argc, char** argv) {
  bool succ;
{
    succ = true;
    //std::string s= "Arsch"; auto begin = s.begin(); auto end = s.end();
    std::istreambuf_iterator<char> begin(std::cin);
    std::istreambuf_iterator<char> end;
    for (; begin != end; ++begin) {
      //std::istreambuf_iterator<char> x = begin;
      //++x;
      //std::cout << *begin << *x << *begin;
      std::cout << *begin << *std::next(begin);
    }
    std::cout << std::endl;
    //succ = parse(begin, end);
  }
  return succ ? 0 : 1;
}

