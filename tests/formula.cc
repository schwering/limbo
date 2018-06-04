// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 Christoph Schwering

#include <gtest/gtest.h>

#include <iostream>

#include <limbo/formula.h>

namespace limbo {

using Sort = Language::Sort;
using Var  = Language::Var;
using Name = Language::Name;
using Func = Language::Func;
using Symbol = Language::Symbol;
using Word = Language::Word;

std::ostream& operator<<(std::ostream& os, const Symbol& s) {
  switch (s.type) {
    case Symbol::kFunc:      os << 'f' << s.u.x.index(); break;
    case Symbol::kName:      os << 'n' << s.u.x.index(); break;
    case Symbol::kVar:       os << 'x' << s.u.x.index(); break;
    case Symbol::kTerm:      os << 't'; break;
    case Symbol::kEquals:    os << "=="; break;
    case Symbol::kNotEquals: os << "!="; break;
    case Symbol::kLiteral:   os << 'l'; break;
    case Symbol::kClause:    os << 'c'; break;
    case Symbol::kNot:       os << "not"; break;
    case Symbol::kExists:    os << "ex x" << s.u.x.index(); break;
    case Symbol::kForall:    os << "fa x" << s.u.x.index(); break;
    case Symbol::kOr:        os << "or_" << s.u.k; break;
    case Symbol::kAnd:       os << "and_" << s.u.k; break;
    case Symbol::kKnow:      os << "know_" << s.u.k; break;
    case Symbol::kMaybe:     os << "maybe_" << s.u.k; break;
    case Symbol::kBelieve:   os << "bel_" << s.u.k << ',' << s.u.l; break;
    case Symbol::kAction:    os << "A "; break;
  }
  os << ' ';
  return os;
}

std::ostream& operator<<(std::ostream& os, const Word& w) {
  for (const Symbol& s : w) {
    os << s;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Formula& f) {
  os << f.word();
  return os;
}

TEST(FormulaTest, Rectify) {
  Language* l = Language::Instance();
  Sort s = l->CreateSort(false);
  Var x = l->CreateVar(s);
  Var y = l->CreateVar(s);
  Var z = l->CreateVar(s);
  Name n = l->CreateName(s, 0);
  Func f = l->CreateFunc(s, 2);
  Func g = l->CreateFunc(s, 1);
  Word fxy = Word::Func(f, std::vector<Word>{Word::Var(x), Word::Var(y)});
  Word fyz = Word::Func(f, std::vector<Word>{Word::Var(y), Word::Var(z)});
  Word gfxy = Word::Func(f, std::vector<Word>{fxy});
  Word gfyz = Word::Func(f, std::vector<Word>{fyz});
  Word w = Word::Exists(x, Word::Or(Word::Forall(y, Word::Exists(z, Word::Equals(fxy.Clone(), fyz.Clone()))),
                                    Word::Exists(x, Word::Forall(y, Word::Exists(z, Word::Equals(gfxy.Clone(), gfyz.Clone()))))));
  Formula phi(w.Clone());
  std::cout << phi << std::endl;
  phi.Rectify();
  std::cout << phi << std::endl;
  phi.Skolemize();
  std::cout << phi << std::endl;
}

}  // namespace limbo

