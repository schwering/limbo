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
    case Symbol::kFunc:      os << 'f' << s.u.f.index(); break;
    case Symbol::kName:      os << 'n' << s.u.n.index(); break;
    case Symbol::kVar:       os << 'x' << s.u.x.index(); break;
    case Symbol::kTerm:      os << 't'; break;
    case Symbol::kEquals:    os << "\u003D"; break;
    case Symbol::kNotEquals: os << "\u2260"; break;
    case Symbol::kLiteral:   os << 'l'; break;
    case Symbol::kClause:    os << 'c'; break;
    case Symbol::kNot:       os << "\u2227 "; break;
    case Symbol::kExists:    os << "\u2203 x" << s.u.x.index(); break;
    case Symbol::kForall:    os << "\u2200 x" << s.u.x.index(); break;
    case Symbol::kOr:        os << "\u2228"; break;
    case Symbol::kAnd:       os << "\u2227"; break;
    case Symbol::kKnow:      os << "know_" << s.u.k; break;
    case Symbol::kMaybe:     os << "maybe_" << s.u.k; break;
    case Symbol::kBelieve:   os << "bel_" << s.u.k << ',' << s.u.l; break;
    case Symbol::kAction:    os << "A "; break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Word& w) {
  for (const Symbol& s : w) {
    os << s << ' ';
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Formula::Reader& r) {
  switch (r.type()) {
    case Symbol::kFunc:
    case Symbol::kVar:
    case Symbol::kName: {
      os << r.head();
      for (int i = 0; i < r.arity(); ++i) {
        if (i == 0) {
          os << '(';
        }
        os << r.arg(i);
        if (i+1 < r.arity()) {
          os << ',';
        } else {
          os << ')';
        }
      }
      break;
    }
    case Symbol::kEquals:
    case Symbol::kNotEquals: {
      os << r.arg(0) << ' ' << r.head() << ' ' << r.arg(1);
      break;
    }
    case Symbol::kTerm:
    case Symbol::kLiteral:
    case Symbol::kClause:
      os << r.head();
      break;
    case Symbol::kNot:
    case Symbol::kExists:
    case Symbol::kForall:
    case Symbol::kKnow:
    case Symbol::kMaybe:
      os << r.head() << ' ' << r.arg(0);
      break;
    case Symbol::kBelieve:
      os << r.head() << ' ' << r.arg(0) << " \u27FE " << r.arg(1);
      break;
    case Symbol::kOr:
    case Symbol::kAnd:
      os << (r.type() == Symbol::kOr ? '[' : '(');
      for (int i = 0; i < r.arity(); ++i) {
        os << r.arg(i);
        if (i+1 < r.arity()) {
          os << ' ' << r.head() << ' ';
        }
      }
      os << (r.type() == Symbol::kOr ? ']' : ')');
      break;
    case Symbol::kAction:
      os << '[' << r.arg(0) << ']' << ' ' << r.arg(1);
      break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Formula& f) {
  os << f.reader();
  return os;
}

TEST(FormulaTest, Rectify) {
  Language* l = Language::Instance();
  Sort s = l->CreateSort(false);
  Var x = l->CreateVar(s);
  Var y = l->CreateVar(s);
  Var z = l->CreateVar(s);
  Var u = l->CreateVar(s);
  Name n = l->CreateName(s, 0);
  Func c = l->CreateFunc(s, 0);
  Func f = l->CreateFunc(s, 2);
  Func g = l->CreateFunc(s, 1);
  Word fxy = Word::Func(f, std::vector<Word>{Word::Var(x), Word::Var(y)});
  Word fyz = Word::Func(f, std::vector<Word>{Word::Var(y), Word::Var(z)});
  Word gfxy = Word::Func(g, std::vector<Word>{fxy});
  Word gfyz = Word::Func(g, std::vector<Word>{fyz});
  Word w = Word::Exists(x, Word::Or(Word::Forall(y, Word::Exists(z, Word::Equals(fxy.Clone(), fyz.Clone()))),
                                    Word::Exists(x, Word::Forall(y, Word::Exists(z, Word::Exists(u, Word::Equals(gfxy.Clone(), gfyz.Clone())))))));

  // Skolemize
  // Swearize
  // Rectify
  // Flatten
  // PushInwards

  {
    std::cout << "" << std::endl;
    Formula phi(Word::Exists(x, Word::Equals(Word::Func(c, std::vector<Word>{}), Word::Name(n, std::vector<Word>{}))));
    std::cout << "Orig: " << phi << std::endl;
    phi.Rectify();
    std::cout << "Rect: " << phi << std::endl;
    phi.Skolemize();
    std::cout << "Skol: " << phi << std::endl;
    phi.PushInwards();
    std::cout << "Push: " << phi << std::endl;
  }

  {
    std::cout << "" << std::endl;
    Formula phi(w.Clone());
    std::cout << "Orig: " << phi << std::endl;
    phi.Rectify();
    std::cout << "Rect: " << phi << std::endl;
    phi.Flatten();
    std::cout << "Flat: " << phi << std::endl;
    phi.PushInwards();
    std::cout << "Push: " << phi << std::endl;
  }
}

}  // namespace limbo

