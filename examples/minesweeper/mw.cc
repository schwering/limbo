// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <cassert>

#include <iostream>
#include <string>

#include "agent.h"
#include "game.h"
#include "kb.h"
#include "play.h"
#include "printer.h"
#include "timer.h"

int main(int argc, char *argv[]) {
  size_t width = 8;
  size_t height = 8;
  size_t n_mines = 10;
  size_t seed = 0;
  size_t max_k = 2;
  bool print_knowledge = false;
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
  if (argc >= 7) {
    print_knowledge = argv[6] == std::string("know");
  }

  Play(width, height, n_mines, seed, max_k, TerminalColors());
  return 0;
}

