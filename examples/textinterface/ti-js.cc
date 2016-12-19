// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering

#include <algorithm>
#include <iostream>
#include <list>
#include <functional>
#include <string>
#include <sstream>
#include <utility>

#include <emscripten.h>

#include <lela/setup.h>
#include <lela/formula.h>
#include <lela/format/output.h>
#include <lela/format/pdl/context.h>
#include <lela/format/pdl/parser.h>

using lela::format::output::operator<<;

template<typename T>
inline std::string to_string(const T& x) {
  std::stringstream ss;
  ss << x;
  return ss.str();
}

inline void parse(const char* c_str) {
  struct Logger : public lela::format::pdl::Logger {
    void operator()(const LogData&) const {
      std::cout << "Unknown log data" << std::endl;
    }
    void operator()(const RegisterData& d) const {
      std::cout << "Registered " << d.id << std::endl;
    }
    void operator()(const RegisterSortData& d) const {
      std::cout << "Registered sort " << d.id << std::endl;
    }
    void operator()(const RegisterVariableData& d) const {
      std::cout << "Registered variable " << d.id << " of sort " << d.sort_id << std::endl;
    }
    void operator()(const RegisterNameData& d) const {
      std::cout << "Registered name " << d.id << " of sort " << d.sort_id << std::endl;
    }
    void operator()(const RegisterFunctionData& d) const {
      std::cout << "Registered function symbol " << d.id << " with arity " << int(d.arity) << " of sort " << d.sort_id << std::endl;
    }
    void operator()(const RegisterFormulaData& d) const {
      std::cout << "Registered formula " << d.id << " as " << *d.phi << std::endl;
    }
    void operator()(const AddClauseData& d) const {
      std::cout << "Added clause " << d.c << std::endl;
    }
    void operator()(const EntailmentData& d) const {
      std::string phi_str = to_string(*d.phi);
      std::cout << "Setup = " << std::endl << *d.s << std::endl;
      std::cout << "Entails(" << d.k << ", " << phi_str << ") = " << std::boolalpha << d.yes << std::endl;
      EM_ASM_({
        announceEntailment($0, Pointer_stringify($1), $2);
      }, d.k, phi_str.c_str(), d.yes);
    }
    void operator()(const ConsistencyData& d) const {
      std::string phi_str = to_string(*d.phi);
      std::cout << "Setup = " << std::endl << *d.s << std::endl;
      std::cout << "Consistent(" << d.k << ", " << phi_str << ") = " << std::boolalpha << d.yes << std::endl;
      EM_ASM_({
        announceConsistency($0, Pointer_stringify($1), $2);
      }, d.k, phi_str.c_str(), d.yes);
    }
  };

  typedef lela::format::pdl::Context<Logger> Context;
  typedef lela::format::pdl::Parser<std::string::const_iterator, Logger> Parser;

  std::string str = c_str;
  Context ctx;

  Parser parser(str.begin(), str.end(), &ctx);
  const Parser::Result<bool> r = parser.Parse();

  std::cout << r << std::endl;
  EM_ASM_({
    announceResult($0, Pointer_stringify($1), Pointer_stringify($2));
  }, bool(r), r.msg.c_str(), r.remaining_input().c_str());
}

extern "C" void lela_parse(const char* s) {
  return parse(s);
}

