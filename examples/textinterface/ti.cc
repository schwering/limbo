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
#include <lela/format/parser.h>

using lela::format::output::operator<<;

#if 0
#include "lexer.h"

template<typename Iter>
static void lex(Iter begin, Iter end) {
  Lexer<Iter> lexer(begin, end);
  for (const Token& t : lexer) {
    if (t.id() == Token::kError) {
      std::cout << "ERROR ";
    }
    //std::cout << t.str() << " ";
    std::cout << t << " ";
  }
}
#endif

template<typename Iter>
static bool parse(Iter begin, Iter end, lela::format::pdl::Context* ctx) {
  struct PrintAnnouncer : public lela::format::pdl::Announcer {
    void AnnounceEntailment(int k, const lela::Setup& s, const lela::Formula& phi, bool yes) override {
      std::cout << "Entails(" << k << ", " << phi << ") = " << std::boolalpha << yes << std::endl;
      std::cout.flush();
    }

    void AnnounceConsistency(int k, const lela::Setup& s, const lela::Formula& phi, bool yes) override {
      std::cout << "Consistent(" << k << ", " << phi << ") = " << std::boolalpha << yes << std::endl;
      std::cout.flush();
    }
  };
  PrintAnnouncer announcer;
  typedef lela::format::pdl::Parser<Iter> Parser;
  Parser parser(begin, end, ctx, &announcer);
  auto r = parser.Parse();
  std::cout << r << std::endl;
  std::cout.flush();
  return bool(r);
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
  lela::format::pdl::Context ctx;
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
  return succ ? 0 : 1;
}

