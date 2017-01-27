// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 Christoph Schwering

#ifndef EXAMPLES_TEXTINTERFACE_TIMER_H_
#define EXAMPLES_TEXTINTERFACE_TIMER_H_

#include <ctime>

class Timer {
 public:
  Timer() : start_(std::clock()) {}

  void start() {
    start_ = std::clock() - (end_ != 0 ? end_ - start_ : 0);
    ++rounds_;
  }
  void stop() { end_ = std::clock(); }
  void reset() { start_ = 0; end_ = 0; rounds_ = 0; }

  bool started() const { return rounds_ > 0; }
  double duration() const { return (end_ - start_) / (double) CLOCKS_PER_SEC; }
  size_t rounds() const { return rounds_; }
  double avg_duration() const { return duration() / rounds_; }

 private:
  std::clock_t start_;
  std::clock_t end_ = 0;
  size_t rounds_ = 0;
};

#endif  // EXAMPLES_TEXTINTERFACE_TIMER_H_

