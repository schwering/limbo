// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2017 Christoph Schwering

#ifndef EXAMPLES_TEXT_INTERFACE_SUDOKU_H_
#define EXAMPLES_TEXT_INTERFACE_SUDOKU_H_

#include <iostream>
#include <sstream>
#include <vector>

#include <lela/term.h>
#include <lela/clause.h>
#include <lela/literal.h>
#include <lela/formula.h>
#include <lela/format/output.h>

#include "timer.h"

struct SudokuCallbacks {
  template<typename T>
  bool operator()(T* ctx, const std::string& proc, const std::vector<lela::Term>& args) {
    if (proc == "su_init") {
      ns_ = args;
    } else if (proc == "su_print") {
      if (timer_.started()) {
        timer_.stop();
      }
      std::cout << "Sudoku:" << std::endl;
      std::size_t n_known = 0;
      for (size_t y = 0; y < ns_.size(); ++y) {
        for (size_t x = 0; x < ns_.size(); ++x) {
          bool known = false;
          for (size_t n = 0; n < ns_.size(); ++n) {
            const lela::Term Val = ctx->CreateTerm(ctx->LookupFunction("val"), {ns_[x], ns_[y]});
            const lela::Clause c{lela::Literal::Eq(Val, ns_[n])};
            bool b = ctx->kb()->Entails(*lela::Formula::Factory::Know(0, lela::Formula::Factory::Atomic(c)));
            if (b) {
              using lela::format::output::operator<<;
              std::stringstream ss;
              ss << ns_[n];
              std::cout << ss.str().substr(1);
              known = true;
              ++n_known;
            }
          }
          if (!known) {
            std::cout << " ";
          }
          std::cout << " ";
        }
        std::cout << std::endl;
      }
      std::cout << n_known << " cells known";
      if (timer_.started()) {
        std::cout << "("
                  << timer_.duration() << "s elapsed, "
                  << timer_.avg_duration() << "s on average over "
                  << timer_.rounds() << " moves"
                  << ")" << std::endl;
      }
      timer_.start();
    } else {
      return false;
    }
    return true;
  }

 private:
  std::vector<lela::Term> ns_;
  Timer timer_;
};

#endif  // EXAMPLES_TEXT_INTERFACE_SUDOKU_H_

