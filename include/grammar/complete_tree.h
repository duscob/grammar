//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 25-06-18.
//

#ifndef GRAMMAR_COMPLETE_TREE_H
#define GRAMMAR_COMPLETE_TREE_H

#include <vector>


namespace grammar {

/**
 * Tree Node
 *
 * Used in BalanceTreeByWeight to represent the nodes of tree.
 * @tparam Weight
 */
template<typename Weight>
struct TreeNode;


/**
 * Default Tree Nodes Comparator
 */
class CompareTreeNode;


/**
 * Default Combine Weight of Subtrees
 */
class TreeHeight;


/**
 * Default Balancer Tree by Weight
 */
class BalanceTreeByWeight {
 public:
/**
 * Takes a sequence of leaves and completes the binary tree using the given heuristic. The new nodes are reported using _out object.
 *
 * @tparam II Input iterator
 * @tparam Output Reporter
 * @tparam GetWeight Functor to calculate the weight of input nodes
 * @tparam CombineWeights Functor to combine weights of subtrees
 * @tparam CompareNodes Functor to compare nodes
 *
 * @param _begin
 * @param _end
 * @param _out
 * @param _get_weight
 * @param _combine_weight
 * @param _compare_nodes
 */
  template<typename II, typename Output, typename GetWeight, typename CombineWeights = TreeHeight, typename CompareNodes = CompareTreeNode>
  void operator()(II _begin,
                  II _end,
                  Output &_out,
                  const GetWeight &_get_weight,
                  const CombineWeights &_combine_weight = CombineWeights(),
                  const CompareNodes &_compare_nodes = CompareNodes()) const {
    using Node = TreeNode<decltype(_get_weight(*_begin))>;

    std::vector<Node> nodes;
    std::size_t length = 0;
    // Add leaves
    for (auto it = _begin; it != _end; ++it, ++length) {
      nodes.emplace_back(Node{length, _get_weight(*it), {length, length}, {-1, -1}, {0, 0}});
    }

    std::vector<Node> heap;
    heap.reserve(length * 3);

    auto id = length;
    // Add nodes of height 1
    for (int i = 1; i < length; ++i) {
      Node &left = nodes[i - 1];
      Node &right = nodes[i];

      heap.emplace_back(Node{id,
                             _combine_weight(left.weight, right.weight),
                             {left.cover.first, right.cover.second},
                             {i - 1, i},
                             {left.weight, right.weight}});
    }

    // Create heap
    std::make_heap(heap.begin(), heap.end(), _compare_nodes);

    for (int j = 1; j < length; ++j) {
      Node node;

      // Extract lighter subtree
      do {
        std::pop_heap(heap.begin(), heap.end(), _compare_nodes);
        node = heap.back();
        heap.pop_back();
      } while (nodes[node.children.first].id >= node.id || nodes[node.children.second].id >= node.id);

      node.id = id;
      // Report subtree root
      _out(node.id, nodes[node.children.first].id, nodes[node.children.second].id, node.weight);
      nodes[node.cover.first] = nodes[node.cover.second] = node;

      ++id;
      // Add new possible subtrees to the heap
      if (node.cover.first > 0) {
        const auto &left = nodes[node.cover.first - 1];
        heap.emplace_back(Node{id,
                               _combine_weight(left.weight, node.weight),
                               {left.cover.first, node.cover.second},
                               {node.cover.first - 1, node.cover.first},
                               {left.weight, node.weight}});
        std::push_heap(heap.begin(), heap.end(), _compare_nodes);
      }
      if (node.cover.second < length - 1) {
        const auto &right = nodes[node.cover.second + 1];
        heap.emplace_back(Node{id,
                               _combine_weight(node.weight, right.weight),
                               {node.cover.first, right.cover.second},
                               {node.cover.second, node.cover.second + 1},
                               {node.weight, right.weight}});
        std::push_heap(heap.begin(), heap.end(), _compare_nodes);
      }
    }
  }
};

/**
 * Tree Node
 *
 * Used in BalanceTreeByWeight to represent the nodes of tree.
 * @tparam Weight
 */
template<typename Weight>
struct TreeNode {
  std::size_t id;
  Weight weight;
  std::pair<std::size_t, std::size_t> cover;
  std::pair<std::size_t, std::size_t> children;
  std::pair<Weight, Weight> children_weight;
};


/**
 * Default Tree Nodes Comparator
 */
class CompareTreeNode {
 public:
  /**
   *
   * @tparam Node
   * @param _n1
   * @param _n2
   * @return true (if _n1 is heavier than _n2) or
   *              (if both have the same weight and (lighter subtree of _n1 is heavier than lighter subtree of _n2 or
   *              (both have the same weight and _n1 appears after _n2 (more to the right))))
   */
  template<typename Node>
  bool operator()(const Node &_n1, const Node &_n2) const {
    decltype(_n1.weight) n1_min_child, n2_min_child;
    return _n1.weight > _n2.weight
        || (_n1.weight == _n2.weight
            && ((n1_min_child = std::min(_n1.children_weight.first, _n1.children_weight.second))
                > (n2_min_child = std::min(_n2.children_weight.first, _n2.children_weight.second)) ||
                (n1_min_child == n2_min_child && _n1.children.first > _n2.children.first)));
  }
};


/**
 * Default Combine Weight of Subtrees
 */
class TreeHeight {
 public:
  /**
   *
   * @tparam Value
   * @param _v1
   * @param _v2
   * @return height of tree formed by join the given subtrees
   */
  template<typename Value>
  Value operator()(const Value &_v1, const Value &_v2) const {
    return std::max(_v1, _v2) + 1;
  }
};

}

#endif //GRAMMAR_COMPLETE_TREE_H
