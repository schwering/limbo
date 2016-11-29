// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering
//
// Command line application that interprets a problem description and queries.

#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <string>
#include <sstream>
#include <utility>

#include <lela/setup.h>
#include <lela/formula.h>
#include <lela/internal/iter.h>
#include <lela/format/output.h>

#include "lexer.h"
#include "kb.h"
#include "parser.h"

using lela::format::operator<<;

#if 1
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
static bool parse(Iter begin, Iter end) {
  struct PrintAnnouncer : public Announcer {
    void AnnounceEntailment(int k, const lela::Setup& s, const lela::Formula& phi, bool yes) override {
      std::cout << "Entails(" << k << ", " << phi << ") = " << std::boolalpha << yes << std::endl;
    }

    void AnnounceConsistency(int k, const lela::Setup& s, const lela::Formula& phi, bool yes) override {
      std::cout << "Consistent(" << k << ", " << phi << ") = " << std::boolalpha << yes << std::endl;
    }
  };
  PrintAnnouncer announcer;
  typedef Parser<Iter> MyParser;
  MyParser parser(begin, end, &announcer);
  auto r = parser.Parse();
  std::cout << r << std::endl;
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
  if (argc >= 2) {
    std::cout << "Usage: " << argv[0] << std::endl;
    std::cout << "The program reads and interprets a problem description from stdin." << std::endl;
    return 2;
  } else {
    multi_pass_iterator<std::istreambuf_iterator<char>> begin(std::cin);
    multi_pass_iterator<std::istreambuf_iterator<char>> end;
    lex(begin, end);
    const bool succ = parse(begin, end);
    return succ ? 0 : 1;
  }
}

