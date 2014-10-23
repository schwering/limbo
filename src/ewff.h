// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// schwering@kbsg.rwth-aachen.de

#ifndef _EWFF_H_
#define _EWFF_H_

#include "term.h"
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>

class Ewff {
 public:
  enum Truth { kTrue, kFalse, kUnknown };

  Ewff(const Ewff&) = delete;
  Ewff& operator=(const Ewff&) = delete;
  virtual ~Ewff() = 0;

  static std::unique_ptr<Ewff> True();
  static std::unique_ptr<Ewff> False();
  static std::unique_ptr<Ewff> Equal(const Term& t1, const Term& t2);
  static std::unique_ptr<Ewff> Unequal(const Term& t1, const Term& t2);
  static std::unique_ptr<Ewff> Neg(const std::unique_ptr<Ewff> e);
  static std::unique_ptr<Ewff> Or(const std::unique_ptr<Ewff> e1,
                                  const std::unique_ptr<Ewff> e2);
  static std::unique_ptr<Ewff> And(const std::unique_ptr<Ewff> e);
  static std::unique_ptr<Ewff> Sort(const Term& t,
                                    const std::function<bool(const Term& t)> f);

  virtual std::unique_ptr<Ewff> Substitute(
      const std::map<Term,Term>& theta) const = 0;
  Truth Eval() const;
  Truth Eval(const std::map<Term,Term>& theta) const;
  std::list<std::map<Term,Term>> FindModels(const std::set<Term>& c_vars,
                                            const std::set<Term>& hplus);

  virtual bool is_ground() const;
  std::set<Term> variables() const;
  std::set<Term> names() const;

 protected:
  Ewff() = default;
  virtual Truth Eval(bool sign, const std::map<Term, Term>& theta) const = 0;
  virtual void CollectVariables(std::set<Term>& vs) const = 0;
  virtual void CollectNames(std::set<Term>& ns) const = 0;

 private:
  class Equality;
  class Negation;
  class Disjunction;
  class SortCheck;

  void GenerateModels(const std::vector<Term>& vars,
                      const std::set<Term>& hplus,
                      std::map<Term,Term> theta,
                      std::list<std::map<Term,Term>> models);
};

#endif

