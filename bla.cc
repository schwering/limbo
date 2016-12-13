#include <iostream>
#include <vector>
#include <lela/clause.h>
#include <lela/term.h>

int main() {
  using namespace lela;
  Symbol::Factory sf;
  Term::Factory tf;

  const Symbol::Sort sort = sf.CreateSort();

  std::vector<Term> name;
  for (int i = 0; i < 3; ++i) {
    name.push_back(tf.CreateTerm(sf.CreateName(sort)));
  }
  std::cout << "name " << name.size() << std::endl;

  std::vector<Term> nullary;
  for (int i = 0; i < 2; ++i) {
    nullary.push_back(tf.CreateTerm(sf.CreateFunction(sort, 0)));
  }
  std::cout << "nullary " << nullary.size() << std::endl;

  std::vector<Term> unary;
  for (int i = 0; i < 2; ++i) {
    for (Term n : name) {
      unary.push_back(tf.CreateTerm(sf.CreateFunction(sort, 1), Term::Vector({n})));
    }
  }
  std::cout << "unary " << unary.size() << std::endl;

  std::vector<Term> binary;
//  for (int i = 0; i < 2; ++i) {
//    for (Term m : name) {
//      for (Term n : name) {
//        binary.push_back(tf.CreateTerm(sf.CreateFunction(sort, 2), Term::Vector({m,n})));
//      }
//    }
//  }
//  std::cout << "binary " << binary.size() << std::endl;

  std::vector<Literal> lits;
  for (auto t1 : binary) {
    for (Term n : name) {
      lits.push_back(Literal::Eq(t1, n));
      lits.push_back(Literal::Neq(t1, n));
    }
  }
  for (auto t1 : unary) {
    for (Term n : name) {
      lits.push_back(Literal::Eq(t1, n));
      lits.push_back(Literal::Neq(t1, n));
    }
  }
  for (auto t1 : nullary) {
    for (Term n : name) {
      lits.push_back(Literal::Eq(t1, n));
      lits.push_back(Literal::Neq(t1, n));
    }
  }
  std::cout << "lits " << lits.size() << std::endl;

  std::vector<Clause> clauses;
  for (Literal a : lits) {
    for (Literal b : lits) {
      for (Literal c : lits) {
        clauses.push_back(Clause({a,b,c}));
      }
    }
  }
  std::cout << "clauses " << clauses.size() << std::endl;

  size_t i = 0;
  size_t n = 0;
  for (const Clause& c : clauses) {
    for (const Clause& d : clauses) {
      const bool sub = c.Subsumes(d);
      if (sub) ++i;
      ++n;
    }
  }
  std::cout << i << " / " << n << std::endl;
  std::cout << double(i) / double(n) << std::endl;

  return 0;
}

