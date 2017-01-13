// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Command line application that plays minesweeper.

#include <cassert>

#include <iostream>
#include <string>

#include "agent.h"
#include "game.h"
#include "kb.h"
#include "printer.h"
#include "timer.h"

inline bool Play(size_t width, size_t height, size_t n_mines, size_t seed, int max_k,
                 const Colors& colors, std::ostream* os) {
  Timer t;
  Game g(width, height, n_mines, seed);
  KnowledgeBase kb(&g, max_k);
  KnowledgeBaseAgent agent(&g, &kb, os);
  SimplePrinter printer(&colors, os);
  OmniscientPrinter final_printer(&colors, os);
  t.start();
  do {
    Timer t;
    t.start();
    agent.Explore();
    t.stop();
    *os << std::endl;
    printer.Print(g);
    *os << std::endl;
    *os << "Last move took " << std::fixed << t.duration() << ", queries took " << std::fixed << kb.timer().duration() << " / " << std::setw(4) << kb.timer().rounds() << " = " << std::fixed << kb.timer().avg_duration() << std::endl;
    kb.ResetTimer();
  } while (!g.hit_mine() && !g.all_explored());
  t.stop();
  *os << "Final board:" << std::endl;
  *os << std::endl;
  final_printer.Print(g);
  *os << std::endl;
  const bool win = !g.hit_mine();
  if (win) {
    *os << colors.green() << "You win :-)";
  } else {
    *os << colors.red() << "You loose :-(";
  }
  *os << "  [width: " << g.width() << ", height: " << g.height() << ", height: " << g.n_mines() << ", seed: " << g.seed() << ", max-k: " << max_k << ", runtime: " << t.duration() << " seconds]" << colors.reset() << std::endl;
  return win;
}

int main(int argc, char *argv[]) {
  size_t width = 8;
  size_t height = 8;
  size_t n_mines = 10;
  size_t seed = 0;
  size_t max_k = 2;
  if (argc >= 2) {
    width = atoi(argv[1]);
  }
  if (argc >= 3) {
    height = atoi(argv[2]);
  }
  n_mines = (width + 1) * (height + 1) / 10;
  if (argc >= 4) {
    n_mines = atoi(argv[3]);
  }
  if (argc >= 5) {
    seed = atoi(argv[4]);
  }
  if (argc >= 6) {
    max_k = atoi(argv[5]);
  }
  Play(width, height, n_mines, seed, max_k, TerminalColors(), &std::cout);
  return 0;
}

