// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2017 Christoph Schwering

#include <cstring>

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
static std::vector<int>* split_counts = nullptr;

void Finalize() {
  lela::Symbol::Factory::Reset();
  lela::Term::Factory::Reset();
  if (agent)
    delete agent;
  if (kb)
    delete kb;
  if (game)
    delete game;
  if (timer_overall)
    delete timer_overall;
  if (printer)
    delete printer;
  if (split_counts)
    delete split_counts;
}

void Init(const std::string& cfg, int max_k) {
  Finalize();
  game = new Game(cfg);
  kb = new KnowledgeBase(game, max_k);
  timer_overall = new Timer();
  agent = new KnowledgeBaseAgent(game, kb);
  printer = new SimplePrinter(&colors, &std::cout);
  split_counts = new std::vector<int>();
  split_counts->resize(max_k + 1);
  std::cout << "Initial configuration:" << std::endl;
  std::cout << std::endl;
  printer->Print(*game);
  std::cout << std::endl;
  std::cout << "Ready to play" << std::endl;
}

bool PlayTurn() {
  timer_overall->start();
  Timer timer_turn;
  timer_turn.start();
  const lela::internal::Maybe<Agent::Result> r = agent->Explore();
  timer_turn.stop();
  if (r) {
    ++(*split_counts)[r.val.k];
    std::cout << r.val.p << " = " << r.val.n << " found at split level " << r.val.k << std::endl;
  }
  std::cout << std::endl;
  printer->Print(*game);
  std::cout << std::endl;
  std::cout << "Last move took " << std::fixed << timer_turn.duration() << std::endl;
  kb->ResetTimer();
  const bool game_over = game->solved() || !r;
  timer_overall->stop();

  if (game_over) {
    std::cout << "Final board:" << std::endl;
    std::cout << std::endl;
    printer->Print(*game);
    std::cout << std::endl;
    if (game->solved() && game->legal_solution()) {
      std::cout << colors.green() << "Solution is legal";
    } else {
      std::cout << colors.red() << "Solution is illegal";
    }
    std::cout << "  [max-k: " << kb->max_k() << "; ";
    for (size_t k = 0; k < split_counts->size(); ++k) {
      const int n = (*split_counts)[k];
      if (n > 0) {
        std::cout << "level " << k << ": " << n << "; ";
      }
    }
    std::cout << "runtime: " << timer_overall->duration() << " seconds]" << colors.reset() << std::endl;
  }

  return game_over;
}

}  // namespace game

extern "C" void lela_init(const char* cfg, int max_k) {
  if (!logging::redirector_) {
    logging::redirector_ = new logging::StreamRedirector<logging::JsLogger>(&std::cout);
  }
  game::Init(cfg, max_k);
}

extern "C" int lela_play_turn() {
  return game::PlayTurn() ? 1 : 0;
}

