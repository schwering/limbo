// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering
//
// Implements a simple language to specify entailment problems.

#include <algorithm>
#include <iostream>
#include <list>
#include <functional>
#include <string>
#include <sstream>
#include <utility>

#include <lela/setup.h>
#include <lela/formula.h>
#include <lela/format/output.h>

#include "lexer.h"
#include "kb.h"
#include "parser.h"

using lela::format::operator<<;

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
  typedef Lexer<std::string::const_iterator> StrLexer;
  StrLexer lexer(s.begin(), s.end());
  for (const Token& t : lexer) {
    if (t.id() == Token::kError) {
      std::cout << "ERROR ";
    }
    std::cout << t.str() << " ";
  }
  std::cout << std::endl;
  std::cout << std::endl;

  struct PrintAnnouncer : public Announcer {
    void AnnounceEntailment(int k, const lela::Setup& s, const lela::Formula& phi, bool yes) override {
      std::cout << "Entails(" << k << ", " << phi << ") = " << std::boolalpha << yes << std::endl;
    }

    void AnnounceConsistency(int k, const lela::Setup& s, const lela::Formula& phi, bool yes) override {
      std::cout << "Consistent(" << k << ", " << phi << ") = " << std::boolalpha << yes << std::endl;
    }
  };

  typedef Parser<std::string::const_iterator> StrParser;
  PrintAnnouncer announcer;
  StrParser parser(s.begin(), s.end(), &announcer);
  std::cout << parser.Parse() << std::endl;
  return 0;
}

