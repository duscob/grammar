//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 25-06-18.
//

#ifndef GRAMMAR_COMPLETE_TREE_H
#define GRAMMAR_COMPLETE_TREE_H

#include <vector>
#include <iostream>
#include <bitset>


namespace grammar {

template<typename Weight>
struct GrammarTreeNode {
  int id;
  Weight weight;
  std::pair<int, int> cover;
  std::pair<int, int> children;
  std::pair<Weight, Weight> children_weight;
};


template<typename II, typename Output, typename GetWeight, typename CombineWeight>
void BalanceTreeByWeight(II _begin, II _end, Output &_out, GetWeight _get_weight, CombineWeight _combine_weight) {
  using Node = GrammarTreeNode<decltype(_get_weight(*_begin))>;

  auto minor_weight = [](const Node &_n1, const Node &_n2) -> bool {
    return _n1.weight > _n2.weight
        || (_n1.weight == _n2.weight && std::min(_n1.children_weight.first, _n1.children_weight.second)
            > std::min(_n2.children_weight.first, _n2.children_weight.second));
  };

  std::vector<Node> nodes;
  int length = 0;
  for (auto it = _begin; it != _end; ++it, ++length) {
    Node leaf = {length, _get_weight(*it), {length, length}, {-1, -1}, {0, 0}};
    nodes.emplace_back(leaf);
  }

  std::vector<Node> heap;
  heap.reserve(length * 3);

  int id = length;
  for (int i = 1; i < length; ++i) {
    Node &left = nodes[i - 1];
    Node &right = nodes[i];

    heap.emplace_back(Node{id,
                           _combine_weight(left.weight, right.weight),
                           {left.cover.first, right.cover.second},
                           {i - 1, i},
                           {left.weight, right.weight}});
  }

  std::make_heap(heap.begin(), heap.end(), minor_weight);

  for (int j = 1; j < length; ++j) {
    Node node;

    do {
      std::pop_heap(heap.begin(), heap.end(), minor_weight);
      node = heap.back();
      heap.pop_back();
    } while (nodes[node.children.first].id >= node.id || nodes[node.children.second].id >= node.id);

    node.id = id;
    _out(node.id, nodes[node.children.first].id, nodes[node.children.second].id, node.weight);
    nodes[node.cover.first] = nodes[node.cover.second] = node;

    ++id;
    if (node.cover.first > 0) {
      const auto &left = nodes[node.cover.first - 1];
      heap.emplace_back(Node{id,
                             _combine_weight(left.weight, node.weight),
                             {left.cover.first, node.cover.second},
                             {node.cover.first - 1, node.cover.first},
                             {left.weight, node.weight}});
      std::push_heap(heap.begin(), heap.end(), minor_weight);
    }
    if (node.cover.second < length - 1) {
      const auto &right = nodes[node.cover.second + 1];
      heap.emplace_back(Node{id,
                             _combine_weight(node.weight, right.weight),
                             {node.cover.first, right.cover.second},
                             {node.cover.second, node.cover.second + 1},
                             {node.weight, right.weight}});
      std::push_heap(heap.begin(), heap.end(), minor_weight);
    }
  }
}

}

#endif //GRAMMAR_COMPLETE_TREE_H
