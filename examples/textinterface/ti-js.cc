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

#include "battleship.h"

using lela::format::output::operator<<;

template<typename T>
inline std::string to_string(const T& x) {
  std::stringstream ss;
  ss << x;
  return ss.str();
}

struct Logger : public lela::format::pdl::DefaultLogger {
  void operator()(const LogData&)                      const { std::cout << "Unknown log data" << std::endl; }
  void operator()(const RegisterData& d)               const { std::cout << "Registered " << d.id << std::endl; }
  void operator()(const RegisterSortData& d)           const { std::cout << "Registered sort " << d.id << std::endl; }
  void operator()(const RegisterVariableData& d)       const { std::cout << "Registered variable " << d.id << " of sort " << d.sort_id << std::endl; }
  void operator()(const RegisterNameData& d)           const { std::cout << "Registered name " << d.id << " of sort " << d.sort_id << std::endl; }
  void operator()(const RegisterFunctionData& d)       const { std::cout << "Registered function symbol " << d.id << " with arity " << int(d.arity) << " of sort " << d.sort_id << std::endl; }
  void operator()(const RegisterMetaVariableData& d)   const { std::cout << "Registered meta variable " << d.id << " for " << d.term << std::endl; }
  void operator()(const RegisterFormulaData& d)        const { std::cout << "Registered formula " << d.id << " as " << *d.phi << std::endl; }
  void operator()(const UnregisterData& d)             const { std::cout << "Unregistered " << d.id << std::endl; }
  void operator()(const UnregisterMetaVariableData& d) const { std::cout << "Unregistered meta variable " << d.id << std::endl; }
  void operator()(const AddToKbData& d)                const { std::cout << "Added " << d.alpha << " " << (d.ok ? "" : "un") << "successfully" << std::endl; }
  void operator()(const QueryData& d) const {
    std::string phi_str = to_string(*d.phi);
    for (lela::KnowledgeBase::sphere_index p = 0; p < d.kb.n_spheres(); ++p) {
      std::cout << "Setup[" << p << "] = " << std::endl << d.kb.sphere(p).setup() << std::endl;
    }
    std::cout << "Query: " << phi_str << " = " << std::boolalpha << d.yes << std::endl;
    EM_ASM_({
      announceQuery(Pointer_stringify($0), $1);
    }, phi_str.c_str(), d.yes);
  }
};

struct Callback : public lela::format::pdl::DefaultCallback {
  template<typename T>
  void operator()(T* ctx, const std::string& proc, const std::vector<lela::Term>& args) {
    if (proc == "print_kb") {
      for (lela::KnowledgeBase::sphere_index p = 0; p < ctx->kb()->n_spheres(); ++p) {
        std::cout << "Setup[" << p << "] = " << std::endl << ctx->kb()->sphere(p)->setup() << std::endl;
      }
    } else if (proc == "print") {
      lela::format::output::print_range(std::cout, args, "", "", " ");
      std::cout << std::endl;
    } else if (bs_(ctx, proc, args)) {
      // it's a call for Battleship
    } else {
      std::cerr << "Calling " << proc;
      lela::format::output::print_range(std::cerr, args, "(", ")", ",");
      std::cerr << " failed" << std::endl;
    }
  }

 private:
  BattleshipCallbacks bs_;
};

inline void parse(const char* c_str) {
  typedef lela::format::pdl::Context<Logger, Callback> Context;
  typedef lela::format::pdl::Parser<std::string::const_iterator, Context> Parser;

  std::string str = c_str;

  Context ctx;
  Parser parser(str.begin(), str.end());
  const Parser::Result<Parser::Action<>> parse_result = parser.Parse();
  const Parser::Result<> exec_result = parse_result.val.Run(&ctx);

  std::cout << exec_result.str() << std::endl;
  EM_ASM_({
    announceResult($0, Pointer_stringify($1), Pointer_stringify($2));
  }, bool(exec_result), exec_result.msg().c_str(), exec_result.remaining_input().c_str());
}

extern "C" void lela_parse(const char* s) {
  return parse(s);
}

