// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering

#include <iostream>

#include <emscripten.h>

#include "agent.h"
#include "game.h"
#include "kb.h"
#include "printer.h"
#include "timer.h"

namespace logging {

template<typename TernaryPredicate, class CharT = char, class Traits = std::char_traits<CharT>>
class StreamRedirector : public std::basic_streambuf<CharT, Traits> {
 public:
  StreamRedirector(std::ostream* stream, TernaryPredicate writer = TernaryPredicate())
      : stream_(stream), writer_(writer) {
    old_buf_ = stream_->rdbuf(this);
  }

  ~StreamRedirector() { stream_->rdbuf(old_buf_); }

  std::streamsize xsputn(const CharT* ptr, std::streamsize count) {
    writer_(ptr, count);
    return count;
  }

  typename Traits::int_type overflow(typename Traits::int_type i) {
    CharT c = Traits::to_char_type(i);
    writer_(&c, 1);
    return Traits::not_eof(i);
  }

 private:
  std::basic_ostream<CharT, Traits>* stream_;
  std::streambuf* old_buf_;
  TernaryPredicate writer_;
};

struct JsLogger {
  void operator()(const char* buf, std::streamsize n) {
    bool lf = false;
    for (std::streamsize i = 0; i < n; ++i) {
      if (buf[i] == '\r' || buf[i] == '\n') {
        lf = true;
      }
    }
    buf_ += std::string(buf, n);
    if (lf) {
      flush();
    }
  }

  void flush() {
    EM_ASM_({
      printLine(Pointer_stringify($0));
    }, buf_.c_str());
    buf_ = std::string();
  }

 private:
  std::string buf_;
};

static StreamRedirector<JsLogger>* redirector_ = nullptr;

}  // namespace logging


namespace game {

static HtmlColors colors;

static Game* game = nullptr;
static KnowledgeBase* kb = nullptr;
static KnowledgeBaseAgent* agent = nullptr;
static Timer* timer_overall = nullptr;
static SimplePrinter* printer = nullptr;
static OmniscientPrinter* final_printer = nullptr;
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
  if (printer)
    delete printer;
  if (final_printer)
    delete final_printer;
  if (split_counts)
    delete split_counts;
}

void Init(size_t width, size_t height, size_t n_mines, size_t seed, int max_k) {
  Finalize();
  game = new Game(width, height, n_mines, seed);
  kb = new KnowledgeBase(game, max_k);
  timer_overall = new Timer();
  agent = new KnowledgeBaseAgent(game, kb, &std::cout);
  printer = new SimplePrinter(&colors, &std::cout);
  final_printer = new OmniscientPrinter(&colors, &std::cout);
  split_counts = new std::vector<int>();
  split_counts->resize(max_k + 2);  // last one is for guesses
}

bool PlayTurn() {
  timer_overall->start();
  Timer timer_turn;
  timer_turn.start();
  const int k = agent->Explore();
  if (k >= 0) {
    ++(*split_counts)[k];
  }
  timer_turn.stop();
  std::cout << std::endl;
  printer->Print(*game);
  std::cout << std::endl;
  std::cout << "Last move took " << std::fixed << timer_turn.duration() << ", queries took " << std::fixed << kb->timer().duration() << " / " << std::setw(4) << kb->timer().rounds() << " = " << std::fixed << kb->timer().avg_duration() << std::endl;
  kb->ResetTimer();
  const bool game_over = game->hit_mine() || game->all_explored();
  timer_overall->stop();

  if (game_over) {
    std::cout << "Final board:" << std::endl;
    std::cout << std::endl;
    final_printer->Print(*game);
    std::cout << std::endl;
    const bool win = !game->hit_mine();
    if (win) {
      std::cout << colors.green() << "You win :-)";
    } else {
      std::cout << colors.red() << "You loose :-(";
    }
    std::cout << "  [width: " << game->width() << "; height: " << game->height() << "; height: " << game->n_mines() << "; seed: " << game->seed() << "; max-k: " << kb->max_k() << "; ";
    for (int k = 0; k < split_counts->size(); ++k) {
      const int n = (*split_counts)[k];
      if (n > 0) {
        if (k == kb->max_k() + 1) {
          std::cout << "guesses: " << n << "; ";
        } else {
          std::cout << "level " << k << ": " << n << "; ";
        }
      }
    }
    std::cout << "runtime: " << timer_overall->duration() << " seconds]" << colors.reset() << std::endl;
  }

  return game_over;
}

}  // namespace game

extern "C" void lela_init(size_t width, size_t height, size_t n_mines, size_t seed, int max_k) {
  if (!logging::redirector_) {
    logging::redirector_ = new logging::StreamRedirector<logging::JsLogger>(&std::cout);
  }
  game::Init(width, height, n_mines, seed, max_k);
}

extern "C" int lela_play_turn() {
  return game::PlayTurn() ? 1 : 0;
}

