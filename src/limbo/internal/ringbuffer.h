// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2019 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// RingBuffer that grows on demand.

#ifndef LIMBO_INTERNAL_RINGBUFFER_H_
#define LIMBO_INTERNAL_RINGBUFFER_H_

#include <cassert>
#include <utility>

template<typename T>
class RingBuffer {
 public:
  RingBuffer() = default;

  RingBuffer(const RingBuffer&)            = delete;
  RingBuffer& operator=(const RingBuffer&) = delete;

  RingBuffer(RingBuffer&& b) : xs_(b.xs_), capacity_(b.capacity_), begin_(b.begin_), end_(b.end_) { b.xs_ = nullptr; }
  RingBuffer& operator=(RingBuffer&& b) { xs_ = b.xs_; capacity_ = b.capacity_; begin_ = b.begin_; end_ = b.end_; b.xs_ = nullptr; }

  ~RingBuffer() { delete[] xs_; }

  int size()   const { return begin_ <= end_ ? end_ - begin_ : capacity_ - begin_ + end_; }
  bool empty() const { return begin_ == end_; }
  bool full()  const { return xs_ == nullptr || size() == capacity_ - 1; }

        T& operator[](int i)       { return xs_[(begin_ + i) % capacity_]; }
  const T& operator[](int i) const { return xs_[(begin_ + i) % capacity_]; }

  void PushFront(T&& x) {
    if (full()) {
      Grow();
    }
    assert(!full());
    begin_ = pred(begin_);
    xs_[begin_] = std::move(x);
  }

  void PushFront(const T& x) {
    if (full()) {
      Grow();
    }
    assert(!full());
    begin_ = pred(begin_);
    xs_[begin_] = x;
  }

  T PopFront() {
    assert(!empty());
    T x = std::move(xs_[begin_]);
    begin_ = succ(begin_);
    return x;
  }

  void PushBack(T&& x) {
    if (full()) {
      Grow();
    }
    assert(!full());
    xs_[end_] = std::move(x);
    end_ = succ(end_);
  }

  void PushBack(const T& x) {
    if (full()) {
      Grow();
    }
    assert(!full());
    xs_[end_] = x;
    end_ = succ(end_);
  }

  T PopBack() {
    assert(!empty());
    end_ = pred(end_);
    T x = std::move(xs_[end_]);
    return x;
  }

 private:
  int succ(int i) const { return (i + 1) % capacity_; }
  int pred(int i) const { return ((i - 1) + capacity_) % capacity_; }

  void Grow() {
    const int new_capacity = capacity_ * 3 / 2 + 2;
    T* xs = new T[new_capacity];
    const int s = size();
    for (int i = 0; i < s; ++i) {
      xs[i] = std::move((*this)[i]);
    }
    delete[] xs_;
    xs_ = xs;
    capacity_ = new_capacity;
    begin_ = 0;
    end_ = s;
  }

  T* xs_ = nullptr;
  int capacity_ = 0;
  int begin_ = 0;  // inclusive
  int end_ = 0;  // exclusive
};

#endif  // LIMBO_INTERNAL_RINGBUFFER_H_

