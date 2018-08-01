//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/31/18.
//

#ifndef GRAMMAR_IO_H
#define GRAMMAR_IO_H

#include <iostream>
#include <utility>
#include <cstdint>

#include <sdsl/io.hpp>


namespace std {

template<typename X, typename Y>
uint64_t serialize(const std::pair<X, Y> &x,
                   std::ostream &out,
                   sdsl::structure_tree_node *v = nullptr,
                   const std::string &name = "") {
  return serialize(x.first, out, v, name) + serialize(x.second, out, v, name);
}


template<typename X, typename Y>
void load(std::pair<X, Y> &x, std::istream &in) {
  using sdsl::load;
  load(x.first, in);
  load(x.second, in);
}

}

#endif //GRAMMAR_IO_H
