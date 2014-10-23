// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// schwering@kbsg.rwth-aachen.de

#ifndef _TERM_H_
#define _TERM_H_

#include <map>

class Term {
 public:
  typedef unsigned long NameId;

  static Term CreateVariable();
  static Term CreateStdName(NameId id);

  Term();
  Term(const Term&) = default;
  Term& operator=(const Term&) = default;

  bool operator==(const Term& t) const;
  bool operator<(const Term& t) const;

  const Term& Substitute(const std::map<Term,Term>& theta) const;

  inline bool is_variable() const { return type_ == VAR; }
  inline bool is_name() const { return type_ == NAME; }
  inline bool is_ground() const { return type_ == VAR; }

 private:
  typedef unsigned long long VarId;
  enum Type { DUMMY, VAR, NAME };

  static VarId var_id_;

  Term(Type type, int id);

  Type type_;
  int id_;
};

#endif

