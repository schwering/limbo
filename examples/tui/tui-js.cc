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

#include <limbo/setup.h>
#include <limbo/formula.h>
#include <limbo/format/output.h>
#include <limbo/format/pdl/context.h>
#include <limbo/format/pdl/parser.h>

#include "battleship.h"
#include "sudoku.h"

using limbo::format::operator<<;

template<typename T>
inline std::string to_string(const T& x) {
  std::stringstream ss;
  ss << x;
  return ss.str();
}

static std::string in_color(const std::string& text, int color) {
  std::stringstream ss;
  ss << "\033[" << color << "m" << text << "\033[0m";
  return ss.str();
}

static std::string in_color(const std::string& text, const std::string& color) {
  std::stringstream ss;
  ss << "[[b;" << color << ";]" << text << "]";
  return ss.str();
}

struct Logger : public limbo::format::pdl::DefaultLogger {
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
    //for (limbo::KnowledgeBase::sphere_index p = 0; p < d.kb.n_spheres(); ++p) {
    //  std::cerr << "Setup[" << p << "] = " << std::endl << d.kb.sphere(p).setup() << std::endl;
    //}
    const std::string r = in_color(d.yes ? "Yes" : "No", d.yes ? "#0c0" : "#c00");
    if (print_queries) {
      std::cout << r << std::endl;
    } else {
      std::cerr << r << *d.phi << std::endl;
    }
    //std::cerr << std::endl;
    //std::cerr << std::endl;
  }
  bool print_queries = true;
};

struct Callback : public limbo::format::pdl::DefaultCallback {
  template<typename T>
  void operator()(T* ctx, const std::string& proc, const std::vector<limbo::Term>& args) {
    if (proc == "print_kb") {
      for (limbo::KnowledgeBase::sphere_index p = 0; p < ctx->kb()->n_spheres(); ++p) {
        std::cout << "Setup[" << p << "] = " << std::endl << ctx->kb()->sphere(p)->setup() << std::endl;
      }
    } else if (proc == "print") {
      limbo::format::print_range(std::cout, args, "", "", " ");
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
      limbo::format::print_range(std::cerr, args, "(", ")", ",");
      std::cout << " failed" << std::endl;
    }
  }

 private:
  BattleshipCallbacks bs_;
  SudokuCallbacks su_;
};

typedef limbo::format::pdl::Context<Logger, Callback> Context;
typedef limbo::format::pdl::Parser<std::string::const_iterator, Context> Parser;

static Context* ctx = nullptr;

extern "C" void limbo_init() {
  if (ctx) {
    delete ctx;
  }
  ctx = new Context();
}

extern "C" void limbo_free() {
  if (ctx) {
    delete ctx;
  }
}

extern "C" void limbo_parse(const char* c_str) {
  const std::string str = c_str;

  Parser parser(str.begin(), str.end());
  auto parse_result = parser.Parse();
  if (!parse_result) {
    std::cout << in_color(parse_result.str(), 31) << std::endl;
  }
  auto exec_result = parse_result.val.Run(ctx);
  if (!exec_result) {
    std::cout << in_color(exec_result.str(), 31) << std::endl;
  }
}

