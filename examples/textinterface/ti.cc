// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering
//
// Command line application that interprets a problem description and queries.

#include <fstream>
#include <ios>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>

#include <lela/setup.h>
#include <lela/formula.h>
#include <lela/internal/iter.h>
#include <lela/format/output.h>
#include <lela/format/pdl/context.h>
#include <lela/format/pdl/parser.h>

using lela::format::output::operator<<;

#if 0
#include "lexer.h"

template<typename ForwardIt>
static void lex(ForwardIt begin, ForwardIt end) {
  Lexer<ForwardIt> lexer(begin, end);
  for (const Token& t : lexer) {
    if (t.id() == Token::kError) {
      std::cout << "ERROR ";
    }
    //std::cout << t.str() << " ";
    std::cout << t << " ";
  }
}
#endif

template<typename ForwardIt, typename Context>
static bool parse(ForwardIt begin, ForwardIt end, Context* ctx) {
  typedef lela::format::pdl::Parser<ForwardIt, Context> Parser;
  Parser parser(begin, end);
  auto parse_result = parser.Parse();
  if (!parse_result) {
    std::cout << parse_result.str() << std::endl;
    return false;
  }
  auto exec_result = parse_result.val.Run(ctx);
  if (!exec_result) {
    std::cout << exec_result.str() << std::endl;
    return false;
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
  typedef lela::internal::iterator_proxy<multi_pass_iterator> proxy;

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

struct Logger : public lela::format::pdl::Logger {
  void operator()(const LogData&)                      const { std::cerr << "Unknown log data" << std::endl; }
  void operator()(const RegisterData& d)               const { std::cerr << "Registered " << d.id << std::endl; }
  void operator()(const RegisterSortData& d)           const { std::cerr << "Registered sort " << d.id << std::endl; }
  void operator()(const RegisterVariableData& d)       const { std::cerr << "Registered variable " << d.id << " of sort " << d.sort_id << std::endl; }
  void operator()(const RegisterNameData& d)           const { std::cerr << "Registered name " << d.id << " of sort " << d.sort_id << std::endl; }
  void operator()(const RegisterFunctionData& d)       const { std::cerr << "Registered function symbol " << d.id << " with arity " << int(d.arity) << " of sort " << d.sort_id << std::endl; }
  void operator()(const RegisterMetaVariableData& d)   const { std::cerr << "Registered meta variable " << d.id << " for " << d.term << std::endl; }
  void operator()(const RegisterFormulaData& d)        const { std::cerr << "Registered formula " << d.id << " as " << *d.phi << std::endl; }
  void operator()(const UnregisterData& d)             const { std::cerr << "Unregistered " << d.id << std::endl; }
  void operator()(const UnregisterMetaVariableData& d) const { std::cerr << "Unregistered meta variable " << d.id << std::endl; }
  void operator()(const AddToKbData& d)                const { std::cerr << "Added " << d.alpha << " " << (d.ok ? "" : "un") << "successfully" << std::endl; }
  void operator()(const QueryData& d)                  const {
    for (lela::KnowledgeBase::sphere_index p = 0; p < d.kb.n_spheres(); ++p) {
      std::cout << "Setup[" << p << "] = " << std::endl << d.kb.sphere(p).setup() << std::endl;
    }
    std::cout << "Query: " << *d.phi << ") = " << std::boolalpha << d.yes << std::endl;
    std::cout << std::endl;
    std::cout << std::endl;
  }
};

int main(int argc, char** argv) {
  bool help = false;
  bool wait = false;
  bool after_flags = false;
  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    std::string s(argv[i]);
    if (!after_flags && (s == "-h" || s == "--help")) {
      help = true;
    } else if (!after_flags && (s == "-w" || s == "--wait")) {
      wait = true;
    } else if (!after_flags && s == "--") {
      after_flags = true;
    } else if (!after_flags && !s.empty() && s[0] == 'w') {
      std::cerr << "Unknown flag: " << s << " (use '--' to separate flags from arguments)" << std::endl;
    } else {
      args.push_back(s);
    }
  }
  wait |= args.empty();

  if (help) {
    std::cout << "Usage: " << argv[0] << " [[-w | --wait]] file [file ...]]" << std::endl;
    std::cout << "The flag -w or --wait specifies that after reading the files the program reads to stdin." << std::endl;
    std::cout << "If there is no file argument, content is read from stdin." << std::endl;
    return 2;
  }

  std::ios_base::sync_with_stdio(true);
  bool succ = true;
  lela::format::pdl::Context<Logger> ctx;
  for (const std::string& arg : args) {
    if (!succ) {
      break;
    }
    std::ifstream stream(arg);
    if (!stream.is_open()) {
      std::cerr << "Cannot open file " << arg << std::endl;
      return 2;
    }
    multi_pass_iterator<std::istreambuf_iterator<char>> begin(stream);
    multi_pass_iterator<std::istreambuf_iterator<char>> end;
    succ &= succ && parse(begin, end, &ctx);
  }
  if (wait && succ) {
    multi_pass_iterator<std::istreambuf_iterator<char>> begin(std::cin);
    multi_pass_iterator<std::istreambuf_iterator<char>> end;
    succ &= parse(begin, end, &ctx);
  }
  std::cout << "Bye." << std::endl;
  return succ ? 0 : 1;
}

