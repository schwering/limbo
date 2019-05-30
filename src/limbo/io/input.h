// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2019 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.

#ifndef LIMBO_IO_INPUT_H_
#define LIMBO_IO_INPUT_H_

#include <iostream>

#include <limbo/formula.h>
#include <limbo/lit.h>
#include <limbo/io/parser.h>

namespace limbo {
namespace io {

// istreambuf_iterators are InputIterators, but our lexer wants a
// ForwardIterator, so we need to wrap it.
template<typename InputIt, typename Buffer = std::vector<typename InputIt::value_type>>
class MultiPassIterator {
 public:
  using difference_type   = typename InputIt::difference_type;
  using value_type        = typename InputIt::value_type;
  using reference         = value_type;
  using pointer           = value_type*;
  using iterator_category = std::forward_iterator_tag;

  struct Proxy {
   public:
    using value_type = typename InputIt::value_type;
    using reference  = typename InputIt::reference;
    using pointer    = typename InputIt::pointer;

    reference operator*() const { return v; }
    pointer operator->() const { return &v; }

    mutable typename std::remove_const<value_type>::type v;
  };


  MultiPassIterator() = default;
  MultiPassIterator(InputIt it) : data_(new Data(it)) {}

  bool operator==(const MultiPassIterator it) const {
    return (data_ == it.data_ && index_ == it.index_) || (dead() && it.dead());
  }
  bool operator!=(const MultiPassIterator it) const { return !(*this == it); }

  reference operator*() const {
    buffer_until_index();
    return data_->buf_[index_];
  }

  MultiPassIterator& operator++() {
    // When our index has reached the end, we also need to advance the input
    // iterator. Otherwise we wouldn't detect when we have reached the end
    // input iterator.
    buffer_until_index();
    ++index_;
    return *this;
  }

  Proxy operator->() const { return Proxy{operator*()}; }
  Proxy operator++(int) const { Proxy p{operator*()}; operator++(); return p; }

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

std::istream& operator>>(std::istream& is, Formula& f) {
  using StreamIterator = MultiPassIterator<std::istreambuf_iterator<char>>;
  using Parser = Parser<StreamIterator>;
  Parser parser = Parser(StreamIterator(is), StreamIterator());
  parser.set_default_if_undeclared(true);
  Parser::Result<Parser::Computation<Formula>> parse_result = parser.ParseFormula();
  if (!parse_result) {
    is.setstate(std::ios::failbit);
    return is;
  }
  Parser::Result<Formula> computation_result = parse_result.val.Compute();
  if (!computation_result) {
    is.setstate(std::ios::failbit);
    return is;
  }
  f = std::move(computation_result.val);
  return is;
}

}  // namespace io
}  // namespace limbo

using limbo::io::operator>>;  // I too often forget this, so for now, it's here

#endif  // LIMBO_IO_OUTPUT_H_

