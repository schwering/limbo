// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering
//
// Command line application that interprets a problem description and queries.

#include <dirent.h>

#include <fstream>
#include <ios>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include <limbo/formula.h>
#include <limbo/limsat.h>
#include <limbo/io/input.h>
#include <limbo/io/output.h>
#include <limbo/io/parser.h>

#include "linenoise/linenoise.h"

using namespace limbo;
using namespace limbo::io;

static constexpr int RED = 31;
static constexpr int GREEN = 32;

static std::string in_color(const std::string& text, int color) {
  std::stringstream ss;
  ss << "\033[" << color << "m" << text << "\033[0m";
  return ss.str();
}

class EventHandler {
 public:
  explicit EventHandler(LimSat* lim_sat) : lim_sat_(lim_sat) {}

  void SortRegistered(Alphabet::Sort sort) {
    std::cout << "Registered sort " << sort << std::endl;
  }

  void FunSymbolRegistered(Alphabet::FunSymbol f) {
    std::cout << "Registered function symbol " << f << std::endl;
  }

  void SensorFunSymbolRegistered(Alphabet::FunSymbol f) {
    std::cout << "Registered sensor function symbol " << f << std::endl;
  }

  void NameSymbolRegistered(Alphabet::NameSymbol n) {
    std::cout << "Registered name symbol " << n << std::endl;
  }

  void VarSymbolRegistered(Alphabet::VarSymbol x) {
    std::cout << "Registered variable symbol " << x << std::endl;
  }

  void MetaSymbolRegistered(IoContext::MetaSymbol m) {
    std::cout << "Registered meta symbol " << IoContext::instance()->meta_registry().ToString(m, "m") << std::endl;
  }

  bool Add(const Formula& f) {
    std::vector<Lit> c;
    Formula ff = f.Clone();
    ff.Strip();
    ff.Reduce([&c](const Alphabet::Symbol s) { if (s.tag == Alphabet::Symbol::kStrippedLit) { c.push_back(s.u.a); } return false; },
              [](const RFormula&) { return Formula(); });
    if (!ff.readable().proper_plus()) {
      return false;
    } else {
      std::cout << "Added " << c << std::endl;
      lim_sat_->add_clause(c);
      return true;
    }
  }

  bool Query(const Formula& f) {
    std::cout << "Queried " << f << std::endl;
    int belief_level = 0;
    const Alphabet::Symbol s = f.readable().head();
    if (s.tag == Alphabet::Symbol::kKnow) {
      belief_level = s.u.k;
    }
    lim_sat_->set_query(f.readable());
    const bool succ = lim_sat_->Solve(belief_level);
    std::cout << "Query: " << f << "  =  " << in_color(succ ? "Yes" : "No", succ ? GREEN : RED) << std::endl;
    return succ;
  }

 private:
  Alphabet*  abc() const { return Alphabet::instance(); }
  IoContext* io()  const { return IoContext::instance(); }

  LimSat* lim_sat_;
};

template<typename ForwardIt>
static bool parse(ForwardIt begin, ForwardIt end) {
  using Parser = limbo::io::Parser<ForwardIt, EventHandler>;
  LimSat lim_sat;
  Parser parser(begin, end, EventHandler(&lim_sat));
  auto parse_result = parser.Parse();
  if (!parse_result) {
    std::cout << in_color(parse_result.str(), RED) << std::endl;
    return false;
  }
  auto exec_result = parse_result.val.Compute();
  if (!exec_result) {
    std::cout << in_color(exec_result.str(), RED) << std::endl;
    return false;
  }
  return true;
}

static bool parse_line_by_line(std::istream* stream) {
  for (std::string line; std::getline(*stream, line); ) {
    if (!parse(line.begin(), line.end())) {
      return false;
    }
  }
  return true;
}

int main(int argc, char** argv) {
  const int kFailCode = 1;
  const int kHelpCode = 2;

  enum ReadBehavior { kNothing, kStdin, kInteractive };
  using StreamIterator = limbo::io::MultiPassIterator<std::istreambuf_iterator<char>>;

  ReadBehavior read_behavior = kNothing;
  bool help = false;
  bool after_flags = false;
  bool line_by_line = false;
  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    std::string s(argv[i]);
    if (!after_flags && (s == "-h" || s == "--help")) {
      help = true;
    } else if (!after_flags && (s == "-l" || s == "--line-by-line")) {
      line_by_line = true;
    } else if (!after_flags && (s == "-a" || s == "--all-at-once")) {
      line_by_line = false;
    } else if (!after_flags && (s == "-s" || s == "--stdin")) {
      read_behavior = kStdin;
    } else if (!after_flags && (s == "-i" || s == "--interactive")) {
      read_behavior = kInteractive;
    } else if (!after_flags && s == "--") {
      after_flags = true;
    } else if (!after_flags && !s.empty() && s[0] == 'w') {
      std::cerr << "Unknown flag: " << s << " (use '--' to separate flags from arguments)" << std::endl;
    } else {
      args.push_back(s);
    }
  }
  if (read_behavior == kNothing && args.empty()) {
    read_behavior = kStdin;
  }

  if (help) {
    std::cout << "Usage: " << argv[0] << " [[-s | --stdin]] file [file ...]]" << std::endl;
    std::cout << "      -i   --interactive   after reading the files the program reads to stdin interactively" << std::endl;
    std::cout << "      -s   --stdin         after reading the files the program reads to stdin" << std::endl;
    std::cout << "      -l   --line-by-line  read and parse input line by line (fast than all-at-once)" << std::endl;
    std::cout << "      -a   --all-at-once   read and parse input all at once (can deal with new-lines, default)" << std::endl;
    std::cout << "If there is no file argument, content is read from stdin." << std::endl;
    return kHelpCode;
  }

  std::ios_base::sync_with_stdio(true);

  for (const std::string& arg : args) {
    std::ifstream stream(arg);
    if (!stream.is_open()) {
      std::cerr << "Cannot open file " << arg << std::endl;
      return kFailCode;
    }
    const bool succ = line_by_line ? parse_line_by_line(&stream)
        : parse(StreamIterator(stream), StreamIterator());
    if (!succ) {
      return kFailCode;
    }
  }

  if (read_behavior == kStdin) {
    const bool succ = line_by_line ? parse_line_by_line(&std::cin)
        : parse(StreamIterator(std::cin), StreamIterator());
    if (!succ) {
      return kFailCode;
    }
  }

  if (read_behavior == kInteractive) {
    for (const std::string& arg : args) {
      linenoiseHistoryLoad(arg.c_str());
    }
    char* line_ptr;
    auto is_prefix = [](const std::string& needle, const std::string& haystack) {
      return needle.length() <= haystack.length() &&
          std::mismatch(needle.begin(), needle.end(), haystack.begin()).first == needle.end();
    };
    auto is_postfix = [](const std::string& needle, const std::string& haystack) {
      return needle.length() <= haystack.length() &&
          std::mismatch(needle.rbegin(), needle.rend(), haystack.rbegin()).first == needle.rend();
    };
    const std::string kIncludeCommand = ":r ";
    const std::string kListCommand = ":ls";
    const std::string kPrompt = "tui> ";
    while ((line_ptr = linenoise(kPrompt.c_str())) != nullptr) {
      const std::string line = line_ptr;
      if (is_prefix(kIncludeCommand, line)) {
        const std::string file = line.substr(kIncludeCommand.length());
        std::ifstream stream(file);
        if (!stream.is_open()) {
          std::cerr << "Cannot open file " << file << std::endl;
          return kFailCode;
        }
        parse(StreamIterator(stream), StreamIterator());
        linenoiseHistoryLoad(file.c_str());
      } else if (is_prefix(kListCommand, line)) {
        std::string directory = line.substr(kListCommand.length());
        if (directory.empty()) {
          directory = ".";
        }
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir(directory.c_str())) != NULL) {
          while ((ent = readdir(dir)) != NULL) {
            const std::string file = ent->d_name;
            if (is_postfix(".limbo", file)) {
              std::cout << file << std::endl;
            }
          }
          closedir(dir);
        } else {
          std::cerr << "No such directory: " << directory << std::endl;
        }
      } else {
        parse(line.begin(), line.end());
        linenoiseHistoryAdd(line.c_str());
      }
    }
  }
  std::cout << "Bye." << std::endl;
  return 0;
}

