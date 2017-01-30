// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2017 Christoph Schwering
//
// Command line application that plays Sudoku.

#include <cassert>
#include <cstring>

#include <iostream>
#include <string>

#include "agent.h"
#include "game.h"
#include "kb.h"
#include "printer.h"
#include "timer.h"

inline void Play(const std::string& cfg, int max_k, const Colors& colors, std::ostream* os) {
  Timer t;
  Game g(cfg);
  KnowledgeBase kb(&g, max_k);
  KnowledgeBaseAgent agent(&g, &kb, os);
  SimplePrinter printer(&colors, os);
  std::vector<int> split_counts;
  split_counts.resize(max_k + 1);
  t.start();
  int k;
  *os << "Initial Sudoku:" << std::endl;
  *os << std::endl;
  printer.Print(g);
  *os << std::endl;
  do {
    Timer t;
    t.start();
    k = agent.Explore();
    t.stop();
    //++split_counts[k];
    *os << std::endl;
    printer.Print(g);
    *os << std::endl;
    *os << "Last move took " << std::fixed << t.duration() << std::endl;
    kb.ResetTimer();
  } while (!g.solved() && k >= 0);
  t.stop();
  if (g.solved() && g.legal_solution()) {
    std::cout << colors.green() << "Solution is legal";
  } else {
    std::cout << colors.red() << "Solution is illegal";
  }
  std::cout << "  [max-k: " << kb.max_k() << "; ";
  for (int k = 0; k < split_counts.size(); ++k) {
    std::cout << split_counts[k] << " splits at level " << k << "; ";
  }
  std::cout << "runtime: " << t.duration() << " seconds]" << colors.reset() << std::endl;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cout << "Usage: " << argv[0] << " <cfg> <max-k>" << std::endl;
    return 1;
  }
  if (std::strlen(argv[1]) != 9*9) {
    std::cerr << "Config '" << argv[1] << "' is not 9*9 but " << std::strlen(argv[1]) << " characters long" << std::endl;
    return 2;
  }
  const char* cfg = argv[1];
  int max_k = atoi(argv[2]);
  Play(cfg, max_k, TerminalColors(), &std::cout);
  return 0;
}

