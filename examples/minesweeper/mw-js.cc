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
  void operator()(const std::string& str) { str_ = str; UpdateStatus(); }
  void Log(const std::string& str) { str_ = str; UpdateStatus(); }
  void Append(const std::string& str) { str_ += str; UpdateStatus(); }

 private:
  void UpdateStatus() {
    EM_ASM_({
      updateStatus(Pointer_stringify($0));
    }, str_.c_str());
  }

  std::string str_ = "";
};

static Game* game = nullptr;
static KnowledgeBase* kb = nullptr;
static Agent<Logger>* agent = nullptr;
static Timer* timer_overall = nullptr;
static std::vector<int>* split_counts = nullptr;

void Finalize() {
  lela::Symbol::Factory::Reset();
  lela::Term::Factory::Reset();
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
    displayGame(Pointer_stringify($0));
  }, ss.str().c_str());
}

bool PlayTurn() {
  //Timer timer_turn;
  //timer_turn.start();
  timer_overall->start();
  const int k = agent->Explore();
  timer_overall->stop();
  //timer_turn.stop();
  if (k >= 0) {
    ++(*split_counts)[k];
  }
  DisplayGame(*game);
#if 0
  {
    std::stringstream str;
    str << " in " << std::fixed << timer_turn.duration() << " s";
    agent->logger().Append(str.str());
  }
  //kb->ResetTimer();
#endif
  const bool game_over = game->hit_mine() || game->all_explored();

#if 0
  if (game_over) {
    HtmlColors colors;
    std::stringstream str;
    str << "Final board:" << std::endl;
    str << std::endl;
    DisplayGame(*game, true);
    str << std::endl;
    const bool win = !game->hit_mine();
    if (win) {
      str << colors.green() << "You win :-)";
    } else {
      str << colors.red() << "You loose :-(";
    }
    str << "  [width: " << game->width() << "; height: " << game->height() << "; height: " << game->n_mines() << "; seed: " << game->seed() << "; max-k: " << kb->max_k() << "; ";
    for (size_t k = 0; k < split_counts->size(); ++k) {
      const int n = (*split_counts)[k];
      if (n > 0) {
        if (k == kb->max_k() + 1) {
          str << "guesses: " << n << "; ";
        } else {
          str << "level " << k << ": " << n << "; ";
        }
      }
    }
    str << "runtime: " << timer_overall->duration() << " seconds]" << colors.reset() << std::endl;
    agent->logger().Log(str.str());
  }
#endif

  return game_over;
}

}  // namespace game

extern "C" void lela_init(size_t width, size_t height, size_t n_mines, size_t seed, int max_k) {
  game::Init(width, height, n_mines, seed, max_k);
}

extern "C" int lela_play_turn() {
  return game::PlayTurn() ? 1 : 0;
}

