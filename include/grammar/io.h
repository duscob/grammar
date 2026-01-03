//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/31/18.
//

#ifndef GRAMMAR_IO_H
#define GRAMMAR_IO_H

#include <cstdint>
#include <iostream>
#include <utility>

#include <sdsl/io.hpp>

namespace grammar {

template <typename T, typename = void>
struct is_serializable : std::false_type {};

template <typename T>
struct is_serializable<T,
                       std::void_t<decltype(serialize(std::declval<T>(),
                                                      std::declval<std::ostream&>(),
                                                      std::declval<sdsl::structure_tree_node*>(),
                                                      std::declval<const std::string&>()))>> : std::true_type {};

template <typename T, typename = void>
struct is_loadable : std::false_type {};

template <typename T>
struct is_loadable<T, std::void_t<decltype(load(std::declval<T>(), std::declval<std::istream&>()))>> : std::true_type {
};

}  // namespace grammar

namespace std {

template <typename X, typename Y>
std::enable_if_t<!grammar::is_serializable<std::pair<X, Y>>::value, uint64_t> serialize(
    const std::pair<X, Y>& x,
    std::ostream& out,
    sdsl::structure_tree_node* v = nullptr,
    const std::string& name = "") {
  return serialize(x.first, out, v, name) + serialize(x.second, out, v, name);
}

template <typename X, typename Y>
std::enable_if_t<!grammar::is_loadable<std::pair<X, Y>>::value, void> load(std::pair<X, Y>& x, std::istream& in) {
  using sdsl::load;
  load(x.first, in);
  load(x.second, in);
}

}  // namespace std

#endif  // GRAMMAR_IO_H
