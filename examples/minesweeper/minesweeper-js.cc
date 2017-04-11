// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering

#include <iostream>
#include <sstream>

#include <emscripten.h>

#include "agent.h"
#include "game.h"
#include "kb.h"
#include "printer.h"
#include "timer.h"

namespace game {

struct Logger {
  void explored(Point p, int k) const {
    EM_ASM_({
      updateMessage(0, $0, $1, $2);
    }, p.x, p.y, k);
  }

  void flagged(Point p, int k) const {
    EM_ASM_({
      updateMessage(1, $0, $1, $2);
    }, p.x, p.y, k);
  }
};

static Game* game = nullptr;
static KnowledgeBase* kb = nullptr;
static Agent<Logger>* agent = nullptr;
static Timer* timer_overall = nullptr;
static std::vector<int>* split_counts = nullptr;

void Finalize() {
  limbo::Symbol::Factory::Reset();
  limbo::Term::Factory::Reset();
  if (game)
    delete game;
  if (kb)
    delete kb;
  if (agent)
    delete agent;
  if (timer_overall)
    delete timer_overall;
  if (split_counts)
    delete split_counts;
}

void Init(size_t width, size_t height, size_t n_mines, size_t seed, size_t max_k) {
  Finalize();
  game = new Game(width, height, n_mines, seed);
  kb = new KnowledgeBase(game, max_k);
  timer_overall = new Timer();
  agent = new Agent<Logger>(game, kb);
  split_counts = new std::vector<int>();
  split_counts->resize(max_k + 2);  // last one is for guesses
}

void DisplayGame(const Game& g, bool omniscient = false) {
  std::stringstream ss;
  if (!omniscient) {
    SimplePrinter(&ss).Print(*game);
  } else {
    OmniscientPrinter(&ss).Print(*game);
  }
  EM_ASM_({
    if ($1) {
      updateMessageGameOver();
    }
    displayGame(Pointer_stringify($0));
  }, ss.str().c_str(), omniscient);
}

bool PlayTurn() {
  timer_overall->start();
  const int k = agent->Explore();
  timer_overall->stop();
  if (k >= 0) {
    ++(*split_counts)[k];
  }
  DisplayGame(*game);
  const bool game_over = game->hit_mine() || game->all_explored();
  if (game_over) {
    DisplayGame(*game, true);
  }
  return game_over;
}

}  // namespace game

extern "C" void limbo_init(size_t width, size_t height, size_t n_mines, size_t seed, int max_k) {
  game::Init(width, height, n_mines, seed, max_k);
}

extern "C" int limbo_play_turn() {
  return game::PlayTurn() ? 1 : 0;
}

