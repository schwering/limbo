// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <string>
#include <iostream>

#include <boost/bind.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/fusion/include/boost_tuple.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/recursive_variant.hpp>

#include <lela/solver.h>
#include <lela/internal/maybe.h>
#include <lela/format/output.h>
#include <lela/format/syntax.h>

using namespace lela;
using namespace lela::format;

namespace fusion = boost::fusion;
namespace phoenix = boost::phoenix;
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

struct syntax_error : public std::exception {
  syntax_error(const std::string& id) : id_(id) {}
  virtual const char* what() const noexcept override { return id_.c_str(); }
 private:
  std::string id_;
};
struct redeclared_error : public syntax_error { using syntax_error::syntax_error; };
struct undeclared_error : public syntax_error { using syntax_error::syntax_error; };

struct Entailment {
  Entailment() : context_(solver_.sf(), solver_.tf()) {}
  ~Entailment() {
    for (auto p : funs_) {
      if (p.second) {
        delete p.second;
      }
    }
  }

  void RegisterSort(const std::string& id) {
    const Symbol::Sort sort = context_.NewSort();
    lela::format::RegisterSort(sort, id);
    sorts_[id] = sort;
    std::cout << "RegisterSort " << id << std::endl;
  }

  Symbol::Sort LookupSort(const std::string& id) const {
    const auto it = sorts_.find(id);
    if (it == sorts_.end())
      throw undeclared_error(id);
    return it->second;
  }

  void RegisterVar(const boost::fusion::vector<std::string, std::string>& d) {
    const std::string id = boost::fusion::at_c<0>(d);
    const std::string sort_id = boost::fusion::at_c<1>(d);
    if (vars_.find(id) != vars_.end() || names_.find(id) != names_.end() || funs_.find(id) != funs_.end())
      throw redeclared_error(id);
    const Symbol::Sort sort = LookupSort(sort_id);
    const Term var = context_.NewVar(sort);
    vars_[id] = var;
    lela::format::RegisterSymbol(var.symbol(), id);
    std::cout << "RegisterVar " << id << " / " << sort_id << std::endl;
  }

  Term LookupVar(const std::string& id) const {
    const auto it = vars_.find(id);
    if (it == vars_.end())
      throw undeclared_error(id);
    return it->second;
  }

  void RegisterName(const boost::fusion::vector<std::string, std::string>& d) {
    const std::string id = boost::fusion::at_c<0>(d);
    const std::string sort_id = boost::fusion::at_c<1>(d);
    if (vars_.find(id) != vars_.end() || names_.find(id) != names_.end() || funs_.find(id) != funs_.end())
      throw redeclared_error(id);
    const Symbol::Sort sort = LookupSort(sort_id);
    const Term name = context_.NewName(sort);
    names_[id] = name;
    lela::format::RegisterSymbol(name.symbol(), id);
    std::cout << "RegisterName " << id << " / " << sort_id << std::endl;
  }

  Term LookupName(const std::string& id) const {
    const auto it = names_.find(id);
    if (it == names_.end())
      throw undeclared_error(id);
    return it->second;
  }

  const Symbol& LookupFun(const std::string& id) const {
    const auto it = funs_.find(id);
    if (it == funs_.end())
      throw undeclared_error(id);
    return *it->second;
  }

  void RegisterFun(const boost::fusion::vector<std::string, int, std::string>& d) {
    const std::string id = boost::fusion::at_c<0>(d);
    const std::string sort_id = boost::fusion::at_c<2>(d);
    const int arity = boost::fusion::at_c<1>(d);
    if (vars_.find(id) != vars_.end() || names_.find(id) != names_.end() || funs_.find(id) != funs_.end())
      throw redeclared_error(id);
    const Symbol::Sort sort = sorts_[sort_id];
    const Symbol* symbol = new Symbol(context_.NewFun(sort, arity));
    funs_[id] = symbol;
    lela::format::RegisterSymbol(*symbol, id);
    std::cout << "RegisterFun " << id << " / " << arity << " / " << sort_id << std::endl;
  }

 private:
  std::map<std::string, Symbol::Sort>  sorts_;
  std::map<std::string, Term>          vars_;
  std::map<std::string, Term>          names_;
  std::map<std::string, const Symbol*> funs_;
  Solver  solver_;
  Context context_;
};

template<typename Iter>
class Syntax : public qi::grammar<Iter, void(), ascii::space_type> {
 public:
  Syntax() : Syntax::base_type(decls_) {
    identifier_      %= qi::alpha >> *qi::alnum;
    sort_keyword_     = "sort";
    var_keyword_      = "variable";
    name_keyword_     = "name";
    fun_keyword_      = "function";
    sort_decl_        = (identifier_ >> ':' >> sort_keyword_ >> ';') [boost::bind(&Entailment::RegisterSort, &e, ::_1)];
    var_decl_         = (identifier_ >> ':' >> var_keyword_  >> identifier_ >> ';') [boost::bind(&Entailment::RegisterVar, &e, ::_1)];
    name_decl_        = (identifier_ >> ':' >> name_keyword_ >> identifier_ >> ';') [boost::bind(&Entailment::RegisterName, &e, ::_1)];
    fun_decl_         = (identifier_ >> ':' >> fun_keyword_  >> '/' >> qi::uint_ >> identifier_ >> ';') [boost::bind(&Entailment::RegisterFun, &e, ::_1)];
    decls_            = *(sort_decl_ | var_decl_ | name_decl_ | fun_decl_);
    term_             = identifier_ >> -('(' >> (term_ % ',') >> ')');
    literal_          = term_ >> ("==" || "!=") >> term_;
    clause_           = literal_ >> *("||" >> literal_);
  }

 private:
  Entailment e;
  qi::rule<Iter, void(),                                        ascii::space_type> sort_keyword_;
  qi::rule<Iter, void(),                                        ascii::space_type> var_keyword_;
  qi::rule<Iter, void(),                                        ascii::space_type> name_keyword_;
  qi::rule<Iter, void(),                                        ascii::space_type> fun_keyword_;
  qi::rule<Iter, void(),                                        ascii::space_type> eol_;
  qi::rule<Iter, std::string(),                                 ascii::space_type> identifier_;
  qi::rule<Iter, std::string(),                                 ascii::space_type> sort_decl_;
  qi::rule<Iter, boost::tuple<std::string, std::string>(),      ascii::space_type> var_decl_;
  qi::rule<Iter, boost::tuple<std::string, std::string>(),      ascii::space_type> name_decl_;
  qi::rule<Iter, boost::tuple<std::string, int, std::string>(), ascii::space_type> fun_decl_;
  qi::rule<Iter, void(),                                        ascii::space_type> decls_;
  qi::rule<Iter, Term(),                                        ascii::space_type> term_;
  qi::rule<Iter, Literal(),                                     ascii::space_type> literal_;
  qi::rule<Iter, Clause(),                                      ascii::space_type> clause_;
};

#if 0
class Parser {
 public:
  Parser() = default;
  virtual ~Parser() = default;

  virtual internal::Maybe<std::string> ReadLine() = 0;

 private:
};

class StreamParser : public Parser {
  explicit StreamParser(std::istream& stream) : stream_(stream) {}

  virtual internal::Maybe<std::string> ReadLine() override {
    internal::Maybe<std::string> str;
    std::getline(stream_, str.val);
    return internal::Just(str);
  }

 private:
  std::istream& stream_;
};
#endif

int main() {
  const std::string s = "BOOL    :    sort;"\
                        "x : variable BOOL;"\
                        "y : variable BOOL;"\
                        "HUMAN:sort;"\
                        "T : name BOOL;"\
                        "F : name BOOL;"\
                        "fatherOf : function / 3 HUMAN;"\
                        "fatherOf2 : function/3 HUMAN;";
  Syntax<std::string::const_iterator> syntax;
  std::vector<std::string> d;
  qi::phrase_parse(s.begin(), s.end(), syntax, qi::ascii::space, d);
}
 


