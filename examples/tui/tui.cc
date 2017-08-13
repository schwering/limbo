// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering
//
// Command line application that interprets a problem description and queries.

//#define PRINT_ABBREVIATIONS

#include <dirent.h>

#include <fstream>
#include <ios>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include <limbo/setup.h>
#include <limbo/formula.h>
#include <limbo/internal/iter.h>
#include <limbo/format/output.h>
#include <limbo/format/pdl/context.h>
#include <limbo/format/pdl/parser.h>

#include "linenoise/linenoise.h"

#include "battleship.h"
#include "sudoku.h"

using limbo::format::operator<<;

static constexpr int RED = 31;
static constexpr int GREEN = 32;

static std::string in_color(const std::string& text, int color) {
  std::stringstream ss;
  ss << "\033[" << color << "m" << text << "\033[0m";
  return ss.str();
}

template<typename ForwardIt, typename Context>
static bool parse(ForwardIt begin, ForwardIt end, Context* ctx) {
  typedef limbo::format::pdl::Parser<ForwardIt, Context> Parser;
  Parser parser(begin, end);
  auto parse_result = parser.Parse();
  if (!parse_result) {
    std::cout << in_color(parse_result.str(), RED) << std::endl;
    return false;
  }
  auto exec_result = parse_result.val.Run(ctx);
  if (!exec_result) {
    std::cout << in_color(exec_result.str(), RED) << std::endl;
    return false;
  }
  return true;
}

template<typename Context>
static bool parse_line_by_line(std::istream* stream, Context* ctx) {
  for (std::string line; std::getline(*stream, line); ) {
    if (!parse(line.begin(), line.end(), ctx)) {
      return false;
    }
  }
  return true;
}

// istreambuf_iterators are InputIterators, but our lexer wants a ForwardIterator, so we need to wrap it.
template<typename InputIt, typename Buffer = std::vector<typename InputIt::value_type>>
class multi_pass_iterator {
 public:
  typedef typename InputIt::difference_type difference_type;
  typedef typename InputIt::value_type value_type;
  typedef value_type reference;
  typedef value_type* pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef limbo::internal::iterator_proxy<multi_pass_iterator> proxy;

  multi_pass_iterator() = default;
  multi_pass_iterator(InputIt it) : data_(new Data(it)) {}

  bool operator==(const multi_pass_iterator it) const {
    return (data_ == it.data_ && index_ == it.index_) || (dead() && it.dead());
  }
  bool operator!=(const multi_pass_iterator it) const { return !(*this == it); }

  reference operator*() const {
    buffer_until_index();
    return data_->buf_[index_];
  }

  multi_pass_iterator& operator++() {
    // When our index has reached the end, we also need to advance the input
    // iterator. Otherwise we wouldn't detect when we have reached the end
    // input iterator.
    buffer_until_index();
    ++index_;
    return *this;
  }

  proxy operator->() const { return proxy(operator*()); }
  proxy operator++(int) const { proxy p(operator*()); operator++(); return p; }

 private:
  struct Data {
    explicit Data(InputIt it) : it_(it) {}
    InputIt it_;
    Buffer buf_;
  };

  bool buffered() const { return index_ < data_->buf_.size(); }
  bool dead() const { return !data_ || (data_->it_ == InputIt() && !buffered()); }

  void buffer_until_index() const {
    for (; !buffered(); ++(data_->it_)) {
      data_->buf_.push_back(*data_->it_);
    }
  }

  std::shared_ptr<Data> data_;
  size_t index_ = 0;
};

struct Callback : public limbo::format::pdl::DefaultCallback {
  template<typename T>
  void operator()(T* ctx, const std::string& proc, const std::vector<limbo::Term>& args) {
    if (proc == "print_kb") {
      for (limbo::KnowledgeBase::sphere_index p = 0; p < ctx->kb().n_spheres(); ++p) {
        std::cerr << "Setup[" << p << "] = " << std::endl << ctx->kb().sphere(p).setup() << std::endl;
      }
    } else if (proc == "print") {
      limbo::format::print_range(std::cout, args, "", "", " ");
      std::cout << std::endl;
    } else if (proc == "enable_query_logging") {
      ctx->logger().print_queries = true;
    } else if (proc == "disable_query_logging") {
      ctx->logger().print_queries = false;
    } else if (proc == "enable_distribute") {
      ctx->set_distribute(true);
    } else if (proc == "disable_distribute") {
      ctx->set_distribute(false);
    } else if (bs_(ctx, proc, args)) {
      // it's a call for Battleship
    } else if (su_(ctx, proc, args)) {
      // it's a call for Sudoku
    } else {
      std::cerr << "Calling " << proc;
      limbo::format::print_range(std::cerr, args, "(", ")", ",");
      std::cerr << " failed" << std::endl;
    }
  }

 private:
  BattleshipCallbacks bs_;
  SudokuCallbacks su_;
};

struct Logger : public limbo::format::pdl::DefaultLogger {
  void operator()(const LogData&)                      const { std::cerr << "Unknown log data" << std::endl; }
  void operator()(const RegisterData& d)               const { std::cerr << "Registered " << d.id << std::endl; }
  void operator()(const RegisterSortData& d)           const { std::cerr << "Registered sort " << d.id << std::endl; }
  void operator()(const RegisterVariableData& d)       const { std::cerr << "Registered variable " << d.id << " of sort " << d.sort_id << std::endl; }
  void operator()(const RegisterNameData& d)           const { std::cerr << "Registered name " << d.id << " of sort " << d.sort_id << std::endl; }
  void operator()(const RegisterFunctionData& d)       const { std::cerr << "Registered function symbol " << d.id << " with arity " << int(d.arity) << " of sort " << d.sort_id << std::endl; }
  void operator()(const RegisterSensorFunctionData& d) const { std::cerr << "Registered sensor function symbol " << d.id << " for sort " << d.sensor_id << " of sort " << d.sort_id << std::endl; }
  void operator()(const RegisterMetaVariableData& d)   const { std::cerr << "Registered meta variable " << d.id << " for " << d.term << std::endl; }
  void operator()(const RegisterFormulaData& d)        const { std::cerr << "Registered formula " << d.id << " as " << *d.phi << std::endl; }
  void operator()(const UnregisterData& d)             const { std::cerr << "Unregistered " << d.id << std::endl; }
  void operator()(const UnregisterMetaVariableData& d) const { std::cerr << "Unregistered meta variable " << d.id << std::endl; }
  void operator()(const AddRealData& d)                const { std::cerr << "Added " << d.a << " to real world" << std::endl; }
  void operator()(const AddToKbData& d)                const { std::cerr << "Added " << d.alpha << " to knowledge base " << (d.ok ? "" : "un") << "successfully" << std::endl; }
  void operator()(const AddToAtData& d)                const { std::cerr << "Added [] "; if (!d.t.null()) { std::cerr << "[" << d.t << "] "; } std::cerr << d.a << " <-> " << d.alpha << " to action theory " << (d.ok ? "" : "un") << "successfully" << std::endl; }
  void operator()(const QueryData& d)                  const {
    //for (limbo::KnowledgeBase::sphere_index p = 0; p < d.kb.n_spheres(); ++p) {
    //  std::cout << "Setup[" << p << "] = " << std::endl << d.kb.sphere(p).setup() << std::endl;
    //}
    std::ostream* os;
    if (print_queries) {
      os = &std::cout;
    } else {
      os = &std::cerr;
    }
    *os << "Query: " << d.phi->NF(ctx->sf(), ctx->tf()) << " = " << in_color(d.yes ? "Yes" : "No", d.yes ? GREEN : RED) << std::endl;
    //std::cout << std::endl;
    //std::cout << std::endl;
  }
  bool print_queries = true;
  limbo::format::pdl::Context<Logger, Callback>* ctx;
};

int main(int argc, char** argv) {
  const int kFailCode = 1;
  const int kHelpCode = 2;

  enum ReadBehavior { kNothing, kStdin, kInteractive };
  typedef multi_pass_iterator<std::istreambuf_iterator<char>> stream_iterator;

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
  limbo::format::pdl::Context<Logger, Callback> ctx;
  ctx.logger().ctx = &ctx;

  for (const std::string& arg : args) {
    std::ifstream stream(arg);
    if (!stream.is_open()) {
      std::cerr << "Cannot open file " << arg << std::endl;
      return kFailCode;
    }
    const bool succ = line_by_line ? parse_line_by_line(&stream, &ctx)
        : parse(stream_iterator(stream), stream_iterator(), &ctx);
    if (!succ) {
      return kFailCode;
    }
  }

  if (read_behavior == kStdin) {
    const bool succ = line_by_line ? parse_line_by_line(&std::cin, &ctx)
        : parse(stream_iterator(std::cin), stream_iterator(), &ctx);
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
        parse(stream_iterator(stream), stream_iterator(), &ctx);
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
        parse(line.begin(), line.end(), &ctx);
        linenoiseHistoryAdd(line.c_str());
      }
    }
  }
  std::cout << "Bye." << std::endl;
  return 0;
}

