// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering

#include <emscripten.h>

#include "printer.h"
#include "play.h"

inline void play(size_t width, size_t height, size_t n_mines, size_t seed, int max_k) {
  Play(width, height, n_mines, seed, max_k, HtmlColors());
}

extern "C" void lela_play(size_t width, size_t height, size_t n_mines, size_t seed, int max_k) {
  play(width, height, n_mines, seed, max_k);
}

