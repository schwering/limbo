// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#ifndef EXAMPLES_MINESWEEPER_PLAY_H_
#define EXAMPLES_MINESWEEPER_PLAY_H_

#include <iostream>

#include "agent.h"
#include "game.h"
#include "kb.h"
#include "printer.h"

inline bool Play(size_t width, size_t height, size_t n_mines, size_t seed, int max_k, const Colors& colors) {
  Timer t;
  Game g(width, height, n_mines, seed);
  KnowledgeBase kb(&g, max_k);
  KnowledgeBaseAgent agent(&g, &kb);
  SimplePrinter printer(&colors);
  OmniscientPrinter final_printer(&colors);
  t.start();
  do {
    Timer t;
    t.start();
    agent.Explore();
    t.stop();
    std::cout << std::endl;
    printer.Print(std::cout, g);
    std::cout << std::endl;
    std::cout << "Last move took " << std::fixed << t.duration() << ", queries took " << std::fixed << kb.timer().duration() << " / " << std::setw(4) << kb.timer().rounds() << " = " << std::fixed << kb.timer().avg_duration() << std::endl;
    kb.ResetTimer();
  } while (!g.hit_mine() && !g.all_explored());
  t.stop();
  std::cout << "Final board:" << std::endl;
  std::cout << std::endl;
  final_printer.Print(std::cout, g);
  std::cout << std::endl;
  const bool win = !g.hit_mine();
  if (win) {
    std::cout << colors.green() << "You win :-)";
  } else {
    std::cout << colors.red() << "You loose :-(";
  }
  std::cout << "  [width: " << g.width() << ", height: " << g.height() << ", height: " << g.n_mines() << ", seed: " << g.seed() << ", runtime: " << t.duration() << " seconds]" << colors.reset() << std::endl;
  return win;
}

#endif  // EXAMPLES_MINESWEEPER_PLAY_H_

