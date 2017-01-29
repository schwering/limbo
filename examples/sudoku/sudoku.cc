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

inline void Play(const char* cfg, int max_k, const Colors& colors, std::ostream* os) {
  Timer t;
  Game g(cfg);
  KnowledgeBase kb(&g, max_k);
  KnowledgeBaseAgent agent(&g, &kb, os);
  SimplePrinter printer(&colors, os);
  t.start();
  bool cont;
  *os << "Initial Sudoku:" << std::endl;
  *os << std::endl;
  printer.Print(g);
  *os << std::endl;
  do {
    Timer t;
    t.start();
    cont = agent.Explore();
    t.stop();
    *os << std::endl;
    printer.Print(g);
    *os << std::endl;
    *os << "Last move took " << std::fixed << t.duration() << std::endl;
    kb.ResetTimer();
  } while (!g.solved() && cont);
  t.stop();
  *os << "Solution is " << (g.legal_solution() ? "" : "il") << "legal" << std::endl;
  *os << std::endl;
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

