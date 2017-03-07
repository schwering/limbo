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
#include "sudoku.h"

using lela::format::operator<<;

template<typename T>
inline std::string to_string(const T& x) {
  std::stringstream ss;
  ss << x;
  return ss.str();
}

static std::string in_color(const std::string& text, const std::string& color) {
  std::stringstream ss;
  ss << "[[;" << color << ";]" << text << "]";
  return ss.str();
}

struct Logger : public lela::format::pdl::DefaultLogger {
  void operator()(const LogData&)                      const { std::cerr << "Unknown log data" << std::endl; }
  void operator()(const RegisterData& d)               const { std::cerr << "Registered " << d.id << std::endl; }
  void operator()(const RegisterSortData& d)           const { std::cerr << "Registered sort " << d.id << std::endl; }
  void operator()(const RegisterVariableData& d)       const { std::cerr << "Registered variable " << d.id << " of sort " << d.sort_id << std::endl; }
  void operator()(const RegisterNameData& d)           const { std::cerr << "Registered name " << d.id << " of sort " << d.sort_id << std::endl; }
  void operator()(const RegisterFunctionData& d)       const { std::cerr << "Registered function symbol " << d.id << " with arity " << int(d.arity) << " of sort " << d.sort_id << std::endl; }
  void operator()(const RegisterMetaVariableData& d)   const { std::cerr << "Registered meta variable " << d.id << " for " << d.term << std::endl; }
  void operator()(const RegisterFormulaData& d)        const { std::cerr << "Registered formula " << d.id << " as " << *d.phi << std::endl; }
  void operator()(const UnregisterData& d)             const { std::cerr << "Unregistered " << d.id << std::endl; }
  void operator()(const UnregisterMetaVariableData& d) const { std::cerr << "Unregistered meta variable " << d.id << std::endl; }
  void operator()(const AddToKbData& d)                const { std::cerr << "Added " << d.alpha << " " << (d.ok ? "" : "un") << "successfully" << std::endl; }
  void operator()(const QueryData& d)                  const {
    //for (lela::KnowledgeBase::sphere_index p = 0; p < d.kb.n_spheres(); ++p) {
    //  std::cerr << "Setup[" << p << "] = " << std::endl << d.kb.sphere(p).setup() << std::endl;
    //}
    if (print_queries) {
      std::cout << in_color(d.yes ? "Yes" : "No", d.yes ? "green" : "#c00") << std::endl;
    } else {
      std::cerr << in_color(d.yes ? "Yes:  " : "No: ", d.yes ? "green" : "#c00") << *d.phi << std::endl;
    }
    //std::cerr << std::endl;
    //std::cerr << std::endl;
  }
  bool print_queries = true;
};

struct Callback : public lela::format::pdl::DefaultCallback {
  template<typename T>
  void operator()(T* ctx, const std::string& proc, const std::vector<lela::Term>& args) {
    if (proc == "print_kb") {
      for (lela::KnowledgeBase::sphere_index p = 0; p < ctx->kb()->n_spheres(); ++p) {
        std::cout << "Setup[" << p << "] = " << std::endl << ctx->kb()->sphere(p)->setup() << std::endl;
      }
    } else if (proc == "print") {
      lela::format::print_range(std::cout, args, "", "", " ");
      std::cout << std::endl;
    } else if (proc == "enable_query_logging") {
      ctx->logger()->print_queries = true;
    } else if (proc == "disable_query_logging") {
      ctx->logger()->print_queries = false;
    } else if (bs_(ctx, proc, args)) {
      // it's a call for Battleship
    } else if (su_(ctx, proc, args)) {
      // it's a call for Sudoku
    } else {
      std::cout << "Calling " << proc;
      lela::format::print_range(std::cerr, args, "(", ")", ",");
      std::cout << " failed" << std::endl;
    }
  }

 private:
  BattleshipCallbacks bs_;
  SudokuCallbacks su_;
};

typedef lela::format::pdl::Context<Logger, Callback> Context;
typedef lela::format::pdl::Parser<std::string::const_iterator, Context> Parser;

static Context* ctx = nullptr;

extern "C" void lela_init() {
  if (ctx) {
    delete ctx;
  }
  ctx = new Context();
}

extern "C" void lela_free() {
  if (ctx) {
    delete ctx;
  }
}

extern "C" void lela_parse(const char* c_str) {
  const std::string str = c_str;

  Parser parser(str.begin(), str.end());
  auto parse_result = parser.Parse();
  if (!parse_result) {
    std::cout << in_color(parse_result.str(), "#c00") << std::endl;
  }
  auto exec_result = parse_result.val.Run(ctx);
  if (!exec_result) {
    std::cout << in_color(exec_result.str(), "#c00") << std::endl;
  }
}

