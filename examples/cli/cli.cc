// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <string>
#include <iostream>

#include <boost/bind.hpp>
#include <boost/config/warning_disable.hpp>
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

typedef std::string SortDecl;
//struct SortDecl { std::string id; };
struct VarDecl  { std::string id; std::string sort; };
struct NameDecl { std::string id; std::string sort; };
struct FunDecl  { std::string id; std::string sort; int arity; };
//BOOST_FUSION_ADAPT_STRUCT(
//  SortDecl,
//  (std::string, id)
//)
BOOST_FUSION_ADAPT_STRUCT(
  VarDecl,
  (std::string, id)
  (std::string, sort)
)
BOOST_FUSION_ADAPT_STRUCT(
  NameDecl,
  (std::string, id)
  (std::string, sort)
)
BOOST_FUSION_ADAPT_STRUCT(
  FunDecl,
  (std::string, id)
  (std::string, sort)
  (int, arity)
)
typedef boost::variant<//boost::recursive_wrapper<SortDecl>,
                       boost::recursive_wrapper<VarDecl>,
                       boost::recursive_wrapper<NameDecl>,
                       boost::recursive_wrapper<FunDecl>> Decl;

struct Entailment {
  Entailment() : context_(solver_.sf(), solver_.tf()) {}
  ~Entailment() {
    for (auto p : funs_) {
      if (p.second)
        delete p.second;
    }
  }

  void RegisterSort(const std::string& id) { std::cout << "RegisterSort " << id << std::endl; sorts_[id] = context_.NewSort(); }
  void RegisterVar(const std::string& id, const std::string& sort) { std::cout << "RegisterVar " << id << " / " << sort << std::endl; vars_[id]  = context_.NewVar(sorts_[sort]); }
  void RegisterName(const std::string& id, const std::string& sort) { std::cout << "RegisterName " << id << " / " << sort << std::endl; names_[id] = context_.NewName(sorts_[sort]); }
  void RegisterFun(const std::string& id, const std::string& sort, int arity) { std::cout << "RegisterFun " << id << " / " << sort << " / " << arity << std::endl; funs_[id] = new Symbol(context_.NewFun(sorts_[sort], arity)); }

  std::map<std::string, Symbol::Sort> sorts_;
  std::map<std::string, Term>         vars_;
  std::map<std::string, Term>         names_;
  std::map<std::string, Symbol*>      funs_;
  Solver  solver_;
  Context context_;
};

template<typename Iter>
class Syntax : public qi::grammar<Iter, std::vector<std::string>(), ascii::space_type> {
 public:
  Syntax() : Syntax::base_type(decls_) {
    using qi::char_;
    using qi::int_;
    using qi::lit;
    using qi::alpha;
    using qi::alnum;
    using qi::lexeme;
    using namespace qi::labels;
    identifier_      %= alpha >> *alnum;
    sort_decl_        = (identifier_ >> lit(':') >> lit("sort") >> lit(';')) [boost::bind(&Entailment::RegisterSort, &e, ::_1)];
    var_decl_         = (identifier_ >> lit(':') >> lit("BOOL") >> lit("variable")  >> lit(';')) [boost::bind(&Entailment::RegisterVar, &e, "", "")];
    name_decl_        = (identifier_ >> lit(':') >> identifier_ >> lit("name") >> lit(';')) [boost::bind(&Entailment::RegisterName, &e, "", "")];
    fun_decl_         = (identifier_ >> lit(':') >> identifier_ >> lit("function") >> lit("of") >> lit("arity") >> int_ >> lit(';')) [boost::bind(&Entailment::RegisterFun, &e, "", "", 0)];
    decls_            = *(sort_decl_ | var_decl_ | name_decl_ | fun_decl_);
  }

 private:
  Entailment e;
  qi::rule<Iter, std::string(),       ascii::space_type> identifier_;
  qi::rule<Iter, std::string(),       ascii::space_type> sort_decl_;
  qi::rule<Iter, std::string(),       ascii::space_type> var_decl_;
  qi::rule<Iter, std::string(),       ascii::space_type> name_decl_;
  qi::rule<Iter, std::string(),       ascii::space_type> fun_decl_;
  qi::rule<Iter, std::vector<std::string>(), ascii::space_type> decls_;
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
                        "x : BOOL variable;"\
                        "HUMAN : sort;"\
                        "T : BOOL name;"\
                        "F : BOOL name;"\
                        "fatherOf : HUMAN function of arity 3";
  Syntax<std::string::const_iterator> syntax;
  std::vector<std::string> d;
  qi::phrase_parse(s.begin(), s.end(), syntax, ascii::space, d);
}
 


