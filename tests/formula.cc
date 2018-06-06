// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 Christoph Schwering

#include <gtest/gtest.h>

#include <iostream>

#include <limbo/formula.h>

namespace limbo {

using Abc = Alphabet;
using Symbol = Abc::Symbol;
using Word = Abc::Word;
using F = Formula;

std::ostream& operator<<(std::ostream& os, const Symbol& s) {
  switch (s.tag) {
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

std::ostream& operator<<(std::ostream& os, const RFormula& r) {
  switch (r.tag()) {
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
      os << (r.tag() == Symbol::kOr ? '[' : '(');
      for (int i = 0; i < r.arity(); ++i) {
        os << r.arg(i);
        if (i+1 < r.arity()) {
          os << ' ' << r.head() << ' ';
        }
      }
      os << (r.tag() == Symbol::kOr ? ']' : ')');
      break;
    case Symbol::kAction:
      os << '[' << r.arg(0) << ']' << ' ' << r.arg(1);
      break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const F& f) {
  os << f.readable();
  return os;
}

TEST(FormulaTest, Rectify) {
  Alphabet* abc = Alphabet::Instance();
  Abc::Sort s = abc->CreateSort(false);
  Abc::Var x = abc->CreateVar(s);
  Abc::Var y = abc->CreateVar(s);
  Abc::Var z = abc->CreateVar(s);
  Abc::Var u = abc->CreateVar(s);
  Abc::Name n = abc->CreateName(s, 0);
  Abc::Func c = abc->CreateFunc(s, 0);
  Abc::Func f = abc->CreateFunc(s, 2);
  Abc::Func g = abc->CreateFunc(s, 1);
  F fxy = F::Func(f, std::vector<F>{F::Var(x), F::Var(y)});
  F fyz = F::Func(f, std::vector<F>{F::Var(y), F::Var(z)});
  F gfxy = F::Func(g, std::vector<F>{fxy});
  F gfyz = F::Func(g, std::vector<F>{fyz});
  F w = F::Exists(x, F::Or(F::Forall(y, F::Exists(z, F::Equals(fxy, fyz))),
                                    F::Exists(x, F::Forall(y, F::Exists(z, F::Exists(u, F::Equals(gfxy, gfyz)))))));

  // Skolemize
  // Swearize
  // Rectify
  // Flatten
  // PushInwards

  {
    std::cout << "" << std::endl;
    F phi(F::Exists(x, F::Equals(F::Func(c, std::vector<F>{}), F::Name(n, std::vector<F>{}))));
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
    F phi(w);
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

