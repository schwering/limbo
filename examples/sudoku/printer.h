// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2017 Christoph Schwering

#ifndef EXAMPLES_SUDOKU_PRINTER_H_
#define EXAMPLES_SUDOKU_PRINTER_H_

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>

#include <lela/format/output.h>

#include "game.h"
#include "kb.h"

typedef std::string Color;

class Colors {
 public:
  virtual ~Colors() {}
  virtual std::string fill(int) const = 0;
  virtual Color reset() const = 0;
  virtual Color dim()   const = 0;
  virtual Color black() const = 0;
  virtual Color red()   const = 0;
  virtual Color green() const = 0;
};

class TerminalColors : public Colors {
 public:
  std::string fill(int n) const override { return std::string(n, ' '); }
  Color reset() const override { return s(0); }
  Color dim()   const override { return s(2); }
  Color black() const override { return s(30); }
  Color red()   const override { return s(31); }
  Color green() const override { return s(32); }

 private:
  static Color s(int c) {
    std::stringstream ss;
    ss << "\033[" << c << "m";
    return ss.str();
  }
};

class HtmlColors : public Colors {
 public:
  std::string fill(int n) const override { return std::string(n, '_'); }
  Color reset()   const override { return s("reset"); }
  Color dim()     const override { return s("dim"); }
  Color black()   const override { return s("black"); }
  Color red()     const override { return s("red"); }
  Color green()   const override { return s("green"); }

 private:
  static Color s(const std::string& cls) {
    std::stringstream ss;
    ss << "</span><span class='"+ cls +"'>";
    return ss.str();
  }
};

class Printer {
 public:
  typedef std::string Label;

  Printer(const Colors* colors, std::ostream* os) : colors_(colors), os_(os) {}
  virtual ~Printer() = default;

  void Print(const Game& g) {
    *os_ << colors_->fill(3);
    for (size_t x = 0; x < 9; ++x) {
      *os_ << colors_->dim() << colors_->dim() << colors_->fill(1) << (x+1) << colors_->fill(1) << colors_->reset();
      if (x == 2 || x == 5) {
        *os_ << colors_->dim() << colors_->fill(1) << colors_->reset();
      }
    }
    *os_ << std::endl;
    for (size_t y = 0; y < 9; ++y) {
      *os_ << colors_->dim() << colors_->fill(1) << (y+1) << colors_->fill(1) << colors_->reset();
      for (size_t x = 0; x < 9; ++x) {
        Label l = label(g, Point(x, y));
        *os_ << colors_->fill(1) << l << colors_->fill(1);
        if (x == 2 || x == 5) {
          *os_ << colors_->dim() << "|" << colors_->reset();
        }
      }
      *os_ << std::endl;
      if (y == 2 || y == 5) {
        *os_ << colors_->fill(1) << colors_->dim() << colors_->fill(2);
        for (size_t x = 0; x < 9; ++x) {
          *os_ << "---";
          if (x == 2 || x == 5) {
            *os_ << "+";
          }
        }
        *os_ << colors_->reset() << std::endl;
      }
    }
  }

  virtual Label label(const Game&, Point) = 0;

 protected:
  const Colors* colors_;
  std::ostream* os_;
};

class SimplePrinter : public Printer {
 public:
  using Printer::Printer;

  Label label(const Game& g, const Point p) override {
    if (g.get(p) > 0) {
      std::stringstream ss;
      ss << g.get(p);
      return Label(ss.str());
    } else {
      return Label(" ");
    }
  }
};

#endif  // EXAMPLES_SUDOKU_PRINTER_H_

