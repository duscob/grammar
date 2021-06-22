//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 6/17/21.
//

#ifndef GRAMMAR_SLP_PARTITION_H_
#define GRAMMAR_SLP_PARTITION_H_

#include <cstddef>
#include <utility>
#include <vector>
#include <map>

#include <sdsl/io.hpp>
#include <sdsl/bit_vectors.hpp>
#include <sdsl/vectors.hpp>

#include "slp_helper.h"
#include "utility.h"

namespace grammar {

template<typename TBvPartitionMarks = sdsl::sd_vector<>,
    typename TBvPartitionMarksRank = typename TBvPartitionMarks::rank_1_type,
    typename TBvPartitionMarksSelect = typename TBvPartitionMarks::select_1_type>
class SLPPartition {
 public:
  typedef std::size_t size_type;

  SLPPartition() = default;

  template<typename TTBvMarks>
  SLPPartition(std::size_t t_l, const TTBvMarks &t_bv_marks)
      : n_partitions_{t_l}, partitions_(t_bv_marks), partitions_rank_{&partitions_}, partitions_select_{&partitions_} {}

  SLPPartition(std::size_t t_l, TBvPartitionMarks &&t_bv_marks)
      : n_partitions_{t_l}, partitions_(t_bv_marks), partitions_rank_{&partitions_}, partitions_select_{&partitions_} {}

  auto Partition(std::size_t t_pos) const { return partitions_rank_(t_pos + 1); }

  auto Position(std::size_t t_partition) const { return partitions_select_(t_partition); }

  auto size() const { return n_partitions_; }

  bool operator==(const SLPPartition &t_other) const {
    return n_partitions_ == t_other.n_partitions_
        && partitions_.size() == t_other.partitions_.size()
        && std::equal(partitions_.begin(), partitions_.end(), t_other.partitions_.begin());
  }

  bool operator!=(const SLPPartition &t_other) const {
    return !(*this == t_other);
  }

  std::size_t serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, const std::string &name = "") const {
    std::size_t written_bytes = 0;

    written_bytes += sdsl::serialize(n_partitions_, out);

    written_bytes += partitions_.serialize(out);
    written_bytes += partitions_rank_.serialize(out);
    written_bytes += partitions_select_.serialize(out);

    return written_bytes;
  }

  void load(std::istream &in) {
    sdsl::load(n_partitions_, in);

    partitions_.load(in);
    partitions_rank_.load(in, &partitions_);
    partitions_select_.load(in, &partitions_);
  }

 protected:
  std::size_t n_partitions_ = 0; // n_partitions_
  TBvPartitionMarks partitions_; // b_l
  TBvPartitionMarksRank partitions_rank_; // b_l_rank
  TBvPartitionMarksSelect partitions_select_; // partitions_select_
};

template<typename TBvFirstChildren = sdsl::sd_vector<>,
    typename TBvFirstChildrenRank = typename TBvFirstChildren::rank_1_type,
    typename TParents = sdsl::vlc_vector<>,
    typename TNextLeaves = sdsl::vlc_vector<>>
class SLPPartitionTree {
 public:
  typedef std::size_t size_type;

  SLPPartitionTree() = default;

  template<typename TTBvFirstChildren, typename TTParents, typename TTNextLeaves>
  SLPPartitionTree(std::size_t t_l,
                   const TTBvFirstChildren &t_first_children,
                   const TTParents &t_parents,
                   const TTNextLeaves &t_next_leaves)
      : n_partitions_{t_l},
        first_children_(t_first_children),
        first_children_rank_{&first_children_},
        parents_(t_parents),
        next_leaves_(t_next_leaves) {}

  SLPPartitionTree(std::size_t t_l,
                   TBvFirstChildren &&t_first_children,
                   TParents &&t_parents,
                   TNextLeaves &&t_next_leaves)
      : n_partitions_{t_l},
        first_children_(t_first_children),
        first_children_rank_{&first_children_},
        parents_(t_parents),
        next_leaves_(t_next_leaves) {}

  auto Parent(std::size_t _i) const {
    auto p = parents_[first_children_rank_(_i) - 1];
    return std::make_pair(n_partitions_ + p + 1, next_leaves_[p] + 1);
  }

  [[nodiscard]] bool IsFirstChild(std::size_t _i) const { return first_children_[_i - 1]; }

  bool operator==(const SLPPartitionTree &t_other) const {
    return n_partitions_ == t_other.n_partitions_
        && first_children_.size() == t_other.first_children_.size()
        && std::equal(first_children_.begin(), first_children_.end(), t_other.first_children_.begin())
        && parents_.size() == t_other.parents_.size()
        && std::equal(parents_.begin(), parents_.end(), t_other.parents_.begin())
        && next_leaves_.size() == t_other.next_leaves_.size()
        && std::equal(next_leaves_.begin(), next_leaves_.end(), t_other.next_leaves_.begin());
  }

  bool operator!=(const SLPPartitionTree &t_other) const {
    return !(*this == t_other);
  }

  std::size_t serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, const std::string &name = "") const {
    std::size_t written_bytes = 0;

    written_bytes += sdsl::serialize(n_partitions_, out);

    written_bytes += first_children_.serialize(out);
    written_bytes += first_children_rank_.serialize(out);

    written_bytes += sdsl::serialize(parents_, out);
    written_bytes += sdsl::serialize(next_leaves_, out);

    return written_bytes;
  }

  void load(std::istream &in) {
    sdsl::load(n_partitions_, in);

    first_children_.load(in);
    first_children_rank_.load(in, &first_children_);

    sdsl::load(parents_, in);
    sdsl::load(next_leaves_, in);
  }

 protected:
  std::size_t n_partitions_ = 0; // n_partitions_
  TBvFirstChildren first_children_; // first_children_
  TBvFirstChildrenRank first_children_rank_; // first_children_rank_
  TParents parents_; // f
  TNextLeaves next_leaves_; // n
};

template<typename TSLP, typename TLeafAction>
auto ComputeSLPPartition(const TSLP &t_slp, uint32_t t_block_size, TLeafAction &&t_leaf_action) {
  std::vector<typename TSLP::VariableType> nodes;
  auto leaf_action = [&nodes, &t_leaf_action](const auto &tt_slp, const auto &tt_var) {
    nodes.emplace_back(tt_var);
    t_leaf_action(tt_slp, tt_var);
  };

  ComputeSampledSLPLeaves(t_slp, t_block_size, leaf_action);

  auto n_leaves = nodes.size();

  sdsl::bit_vector b_leaves(t_slp.SpanLength(t_slp.Start()) + 1, 0);
  {
    std::size_t pos = 0;
    for (const auto &item : nodes) {
      b_leaves[pos] = true;
      pos += t_slp.SpanLength(item);
    }
    b_leaves[b_leaves.size() - 1] = true;
  }

  return std::make_tuple(n_leaves, b_leaves, nodes);
}

template<typename TSLP, typename TPredicate, typename TNodeAction>
auto ComputeSLPPartitionTree(const TSLP &t_slp,
                             uint32_t t_block_size,
                             const TPredicate &t_pred,
                             std::size_t t_n_leaves,
                             std::vector<typename TSLP::VariableType> &t_nodes, // init with leaves
                             TNodeAction &&t_node_action) {
  std::vector<bool> tmp_b_first_children(t_nodes.size(), 0);
  std::map<std::size_t, std::size_t> tmp_parents;
  std::map<std::size_t, std::size_t> tmp_next_leaves;

  auto build_inner_data = [&t_node_action, &tmp_b_first_children, &tmp_parents, &tmp_next_leaves, &t_n_leaves](
      const TSLP &_slp,
      std::size_t _curr_var,
      const auto &_nodes,
      std::size_t _new_node,
      const auto &_left_ranges,
      const auto &_right_ranges) {
    t_node_action(_slp, _curr_var, _nodes, _new_node, _left_ranges, _right_ranges);

    tmp_b_first_children.emplace_back(0);
    tmp_b_first_children[_left_ranges.front()] = 1;

    auto nn = _new_node - t_n_leaves;
    tmp_parents[_left_ranges.front()] = nn;
    auto last_child = _right_ranges.back() + 1;
    tmp_next_leaves[nn] = (last_child <= t_n_leaves) ? last_child : tmp_next_leaves[last_child - t_n_leaves - 1];
  };

  ComputeSampledSLPNodes(t_slp, t_block_size, t_nodes, t_pred, build_inner_data);

  sdsl::bit_vector b_first_children;
  Construct(b_first_children, tmp_b_first_children);

  sdsl::int_vector<> parents;
  Construct(parents, tmp_parents);
  sdsl::util::bit_compress(parents);

  sdsl::int_vector<> next_leaves;
  Construct(next_leaves, tmp_next_leaves);
  sdsl::util::bit_compress(next_leaves);

  return std::make_tuple(b_first_children, parents, next_leaves);
}

template<typename TSLP, typename TPredicate, typename TLeafAction, typename TNodeAction>
auto PartitionSLP(const TSLP &t_slp,
                  uint32_t t_block_size,
                  const TPredicate &t_pred,
                  TLeafAction &&t_leaf_action,
                  TNodeAction &&t_node_action) {
  auto [n_leaves, b_leaves, nodes] = ComputeSLPPartition(t_slp, t_block_size, t_leaf_action);

  auto [b_first_children, parents, next_leaves] = ComputeSLPPartitionTree(
      t_slp, t_block_size, t_pred, n_leaves, nodes, t_node_action);

  return std::make_tuple(n_leaves, b_leaves, b_first_children, parents, next_leaves);
}

template<typename TSLPPartition, typename TSLPPartitionTree, typename TReporter>
auto ComputePartitionTreeCover(const TSLPPartition &t_partition,
                               const TSLPPartitionTree &t_partition_tree,
                               std::size_t t_sp,
                               std::size_t t_ep,
                               TReporter &&t_reporter) {
  auto l = t_partition.Partition(t_sp);
  if (t_partition.Position(l) < t_sp) ++l;

  auto r = t_partition.Partition(t_ep) - 1;

  for (auto i = l, next = i + 1; i <= r; i = next, ++next) {
    while (t_partition_tree.IsFirstChild(i)) {
      auto p = t_partition_tree.Parent(i);
      if (p.second > r + 1)
        break;
      i = p.first;
      next = p.second;
    }

    t_reporter(i);
  }

  return std::make_pair(t_partition.Position(l), t_partition.Position(r + 1));
}

}

#endif //GRAMMAR_SLP_PARTITION_H_
