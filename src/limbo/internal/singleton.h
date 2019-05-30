// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2017--2018 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// Singleton base class.

#ifndef LIMBO_INTERNAL_SINGLETON_H_
#define LIMBO_INTERNAL_SINGLETON_H_

#include <memory>

namespace limbo {
namespace internal {

template<typename T>
struct Singleton {
  static std::unique_ptr<T> instance_;
};

template<typename T>
std::unique_ptr<T> Singleton<T>::instance_;

}  // namespace internal
}  // namespace limbo

#endif  // LIMBO_INTERNAL_SINGLETON_H_

