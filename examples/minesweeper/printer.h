// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 Christoph Schwering

#ifndef EXAMPLES_MINESWEEPER_PRINTER_H_
#define EXAMPLES_MINESWEEPER_PRINTER_H_

#include <iostream>
#include <iomanip>
#include <string>

#include <lela/format/output.h>

#include "game.h"
#include "kb.h"

typedef std::string Color;

class Colors {
 public:
  virtual ~Colors() {}
  virtual Color reset() const = 0;
  virtual Color dim()   const = 0;
  virtual Color black() const = 0;
  virtual Color red()   const = 0;
  virtual Color green() const = 0;
};

class TerminalColors : public Colors {
 public:
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
  Color reset()   const override { return s("reset"); }
  Color dim()     const override { return s("dim"); }
  Color black()   const override { return s("black"); }
  Color red()     const override { return s("red"); }
  Color green()   const override { return s("green"); }

 private:
  static Color s(const std::string& cls) {
    // Hack: on iOS Safari and Chrome, double spaces are not displayed correctly sometimes;
    // so let's use '_' as fill character and re-replace it with "&nbsp;" in the Javascript
    // code.
    std::cout.fill('_');
    std::stringstream ss;
    ss << "</span><span class='"+ cls +"'>";
    return ss.str();
  }
};

class Printer {
 public:
  struct Label {
    Label(const Color& c, const std::string& s) : color(c), text(s) {}

    Color color;
    std::string text;
  };

  Printer(const Colors* colors, std::ostream* os) : colors_(colors), os_(os) {}
  virtual ~Printer() = default;

  void Print(const Game& g) {
    const int width = 3;
    *os_ << std::setw(width) << "";
    for (size_t x = 0; x < g.width(); ++x) {
      *os_ << colors_->dim() << std::setw(width) << x << colors_->reset();
    }
    *os_ << std::endl;
    for (size_t y = 0; y < g.height(); ++y) {
      *os_ << colors_->dim() << std::setw(width) << y << colors_->reset();
      for (size_t x = 0; x < g.width(); ++x) {
        Label l = label(g, Point(x, y));
        *os_ << l.color << std::setw(width) << l.text << colors_->reset();
      }
      *os_ << std::endl;
    }
  }

  virtual Label label(const Game&, Point) = 0;

 protected:
  const Colors* colors_;
  std::ostream* os_;
};

class OmniscientPrinter : public Printer {
 public:
  using Printer::Printer;

  Label label(const Game& g, const Point p) {
    assert(g.valid(p));
    switch (g.state(p)) {
      case Game::UNEXPLORED: return Label(colors_->reset(), g.mine(p) ? "X" : "");
      case Game::FLAGGED:    return Label(colors_->green(), "X");
      case Game::HIT_MINE:   return Label(colors_->red(), "X");
      case 0:                return Label(colors_->reset(), ".");
      default: {
        std::stringstream ss;
        ss << g.state(p);
        return Label(colors_->reset(), ss.str());
      }
    }
    throw;
  }
};

class SimplePrinter : public Printer {
 public:
  using Printer::Printer;

  Label label(const Game& g, const Point p) override {
    assert(g.valid(p));
    switch (g.state(p)) {
      case Game::UNEXPLORED: return Label(colors_->reset(), "");
      case Game::FLAGGED:    return Label(colors_->green(), "X");
      case Game::HIT_MINE:   return Label(colors_->red(), "X");
      case 0:                return Label(colors_->reset(), ".");
      default: {
        std::stringstream ss;
        ss << g.state(p);
        return Label(colors_->reset(), ss.str());
      }
    }
    throw;
  }
};

class KnowledgeBasePrinter : public Printer {
 public:
  explicit KnowledgeBasePrinter(const Colors* colors, std::ostream* os, KnowledgeBase* kb)
      : Printer(colors, os), kb_(kb) {}

  Label label(const Game& g, const Point p) override {
    kb_->Sync();
    assert(g.valid(p));
    switch (g.state(p)) {
      case Game::UNEXPLORED: {
        if (g.frontier(p)) {
          const lela::internal::Maybe<bool> r = kb_->IsMine(p, kb_->max_k());
          if (r.yes) {
            assert(g.mine(p) == r.val);
            if (r.val) {
              return Label(colors_->red(), "X");
            } else if (!r.val) {
              return Label(colors_->green(), "O");
            }
          }
        }
        return Label(colors_->reset(), "");
      }
      case Game::FLAGGED: {
        return Label(colors_->green(), "X");
      }
      case Game::HIT_MINE: {
        return Label(colors_->red(), "X");
      }
      default: {
        const int m = g.state(p);
        if (m == 0) {
          return Label(colors_->reset(), ".");
        }
        std::stringstream ss;
        ss << m;
        return Label(colors_->reset(), ss.str());
      }
    }
    throw;
  }

 private:
  KnowledgeBase* kb_;
};

#endif  // EXAMPLES_MINESWEEPER_PRINTER_H_

