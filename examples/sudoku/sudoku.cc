// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2017 Christoph Schwering
//
// Command line application that plays Sudoku.

#include <cassert>
#include <cstring>

#include <iostream>
#include <string>

#include <limbo/internal/maybe.h>

#include "agent.h"
#include "game.h"
#include "kb.h"
#include "printer.h"
#include "timer.h"

inline bool Play(const std::string& cfg, int max_k, const Colors& colors, std::ostream* os, bool print_dimacs = false) {
  Timer timer_overall;
  Game g(cfg);
  KnowledgeBase kb(max_k);
  if (print_dimacs) { kb.PrintDimacs(&std::cerr); g.PrintDimacs(&std::cerr); }
  kb.InitGame(&g);
  KnowledgeBaseAgent agent(&g, &kb);
  SimplePrinter printer(&colors, os);
  std::vector<int> split_counts;
  split_counts.resize(max_k + 1);
  limbo::internal::Maybe<Agent::Result> r;
  *os << "Initial Sudoku:" << std::endl;
  *os << std::endl;
  printer.Print(g);
  do {
    Timer timer_turn;
    timer_turn.start();
    timer_overall.start();
    r = agent.Explore();
    timer_overall.stop();
    timer_turn.stop();
    if (r) {
      ++split_counts[r.val.k];
      *os << r.val.p << " = " << r.val.n << " found at split level " << r.val.k << std::endl;
    }
    *os << std::endl;
    printer.Print(g);
    *os << std::endl;
    *os << "Last move took " << std::fixed << timer_turn.duration() << std::endl;
    kb.ResetTimer();
  } while (!g.solved() && g.legal() && r);
  const bool solved = g.solved() && g.legal();
  std::cout << (solved ? colors.green() : colors.red()) << "Solution is " << (solved ? "" : "il") << "legal";
  std::cout << "  [max-k: " << kb.max_k() << "; ";
  for (size_t k = 0; k < split_counts.size(); ++k) {
    const int n = split_counts[k];
    if (n > 0) {
      std::cout << "level " << k << ": " << n << "; ";
    }
  }
  std::cout << "runtime: " << timer_overall.duration() << " seconds]" << colors.reset() << std::endl;
  return solved;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cout << "Usage: " << argv[0] << " <cfg> <max-k>" << std::endl;
    return 2;
  }
  if (std::strlen(argv[1]) != 9*9) {
    std::cerr << "Config '" << argv[1] << "' is not 9*9 but " << std::strlen(argv[1]) << " characters long" << std::endl;
    return 3;
  }
  const char* cfg = argv[1];
  int max_k = atoi(argv[2]);
  bool solved = Play(cfg, max_k, TerminalColors(), &std::cout);
  return solved ? 0 : 1;
}

