//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/26/18.
//

#ifndef GRAMMAR_SAMPLED_SLP_H
#define GRAMMAR_SAMPLED_SLP_H

#include <cstdint>
#include <vector>
#include <iterator>

#include <sdsl/bit_vectors.hpp>


namespace grammar {

class NoAction {
 public:
  template<typename ...Args>
  void operator()(Args... args) {}
};


template<typename _PTS>
class AddSet {
 public:
  AddSet(_PTS &_pts) : pts_(_pts) {}

  template<typename _SLP>
  void operator()(const _SLP &_slp, std::size_t _curr_var) {
    auto set = _slp.Span(_curr_var);
    sort(set.begin(), set.end());
    set.erase(unique(set.begin(), set.end()), set.end());

    pts_.AddSet(set);
  }

  template<typename _SLP, typename _Nodes, typename _RangeContainer>
  void operator()(const _SLP &_slp,
                  std::size_t _curr_var,
                  const _Nodes &_nodes,
                  std::size_t _new_node,
                  const _RangeContainer &_left_ranges,
                  const _RangeContainer &_right_ranges) {
    (*this)(_slp, _curr_var);
  }

 private:
  _PTS &pts_;
};


template<typename _SLP, typename _OutputIterator, typename _Action = NoAction>
void ComputeSampledSLPLeaves(const _SLP &_slp,
                             uint64_t _block_size,
                             _OutputIterator _out,
                             _Action _action = NoAction()) {
  ComputeSampledSLPLeaves(_slp, _block_size, _out, _slp.Start(), _action);
}


template<typename _SLP, typename _OutputIterator, typename _Action = NoAction>
void ComputeSampledSLPLeaves(const _SLP &_slp,
                             uint64_t _block_size,
                             _OutputIterator _out,
                             std::size_t _curr_var,
                             _Action _action = NoAction()) {
  if (_curr_var == 0)
    _curr_var = _slp.Start();

  auto length = _slp.SpanLength(_curr_var);
  if (length <= _block_size) {
    _out = _curr_var;
    ++_out;

    _action(_slp, _curr_var);

    return;
  }

  const auto &children = _slp[_curr_var];

  ComputeSampledSLPLeaves(_slp, _block_size, _out, children.first, _action);
  ComputeSampledSLPLeaves(_slp, _block_size, _out, children.second, _action);
}


/**
 * Predicate Must Be Sampled
 *
 * @tparam _PTS Precomputed Terminal Set
 */
template<typename _PTS>
class MustBeSampled {
 public:
  MustBeSampled(const _PTS &_pts, float _storing_factor) : pts_(_pts), storing_factor_(_storing_factor) {}

  template<typename _SLP, typename _RangeContainer>
  bool operator()(const _SLP &_slp,
                  std::size_t _curr_var,
                  const _RangeContainer &left_ranges,
                  const _RangeContainer &right_ranges) const {

    auto Count = [this](const auto &ranges) -> auto {
      std::size_t length = 0;
      for (const auto &range : ranges) {
        for (int i = 0; i < range.second; ++i) {
          // Added sets ranges start in 1, not in 0.
          auto set = pts_[range.first + i + 1];
          length += set.second - set.first;
        }
      }

      return length;
    };

    auto set = _slp.Span(_curr_var);
    sort(set.begin(), set.end());
    set.erase(unique(set.begin(), set.end()), set.end());

    auto expanded_length = Count(left_ranges) + Count(right_ranges);

    return set.size() * storing_factor_ < expanded_length;
  }

 private:
  const _PTS &pts_;
  float storing_factor_;
};


template<typename _SLP, typename _SLPValueType = uint32_t, typename _Predicate, typename _Action = NoAction>
void ComputeSampledSLPNodes(const _SLP &_slp,
                            uint32_t _block_size,
                            std::vector<_SLPValueType> &_nodes,
                            const _Predicate &_pred,
                            _Action _action = NoAction()) {
  std::size_t curr_leaf = 0;
  ComputeSampledSLPNodes(_slp, _slp.Start(), _block_size, _nodes, curr_leaf, _pred, _action);
}


template<typename _SLP, typename _SLPValueType = uint32_t, typename _Predicate, typename _Action = NoAction>
std::vector<std::pair<std::size_t, std::size_t>> ComputeSampledSLPNodes(const _SLP &_slp,
                                                                        std::size_t _curr_var,
                                                                        uint32_t _block_size,
                                                                        std::vector<_SLPValueType> &_nodes,
                                                                        std::size_t &_curr_leaf,
                                                                        const _Predicate &_pred,
                                                                        _Action _action = NoAction()) {
  auto length = _slp.SpanLength(_curr_var);
  if (length <= _block_size) {
    return {{_curr_leaf++, 1}};
  }

  const auto &children = _slp[_curr_var];

  auto left_children = ComputeSampledSLPNodes(_slp, children.first, _block_size, _nodes, _curr_leaf, _pred, _action);
  auto right_children = ComputeSampledSLPNodes(_slp, children.second, _block_size, _nodes, _curr_leaf, _pred, _action);

  if (_pred(_slp, _curr_var, left_children, right_children) || _curr_var == _slp.Start()) {
    _nodes.emplace_back(_curr_var);
    auto new_node = _nodes.size() - 1;
    _action(_slp, _curr_var, _nodes, new_node, left_children, right_children);
    return {{new_node, 1}};
  } else {
    left_children.insert(left_children.end(), right_children.begin(), right_children.end());
    return left_children;
  }
}


template<typename _BVLeafNodesMarks,
    typename _BVLeafNodesMarksRank,
    typename _PTS>
class SampledSLP {
 public:
  SampledSLP() = default;

  template<typename _SLP, typename _SLPValueType = uint32_t>
  SampledSLP(const _SLP &_slp, uint32_t _block_size, float _storing_factor) {
    std::vector<_SLPValueType> nodes;

    AddSet<_PTS> add_set(pts_);
    ComputeSampledSLPLeaves(_slp, _block_size, back_inserter(nodes), add_set);

    sdsl::bit_vector tmp_bv(_slp.SpanLength(_slp.Start()), 0);
    std::size_t pos = 0;
    for (const auto &item : nodes) {
      tmp_bv[pos] = 1;
      pos += _slp.SpanLength(item);
    }
    b_l = _BVLeafNodesMarks(tmp_bv);
    b_l_rank = _BVLeafNodesMarksRank(&b_l);

    grammar::MustBeSampled<_PTS> pred(pts_, _storing_factor);
    std::vector<bool> tmp_b_f(nodes.size(), 0);
    f.resize(nodes.size(), 0);

    auto build_inner_data = [&add_set, &tmp_b_f, this](const _SLP &_slp,
                          std::size_t _curr_var,
                          const auto &_nodes,
                          std::size_t _new_node,
                          const auto &_left_ranges,
                          const auto &_right_ranges) {
      add_set(_slp, _curr_var, _nodes, _new_node, _left_ranges, _right_ranges);

      tmp_b_f.emplace_back(0);
      tmp_b_f[_left_ranges[0].first] = 1;

      f.emplace_back(0);
      f[_left_ranges[0].first] = _new_node;
    };

    ComputeSampledSLPNodes(_slp, _block_size, nodes, pred, build_inner_data);

    f.erase(std::remove(f.begin(), f.end(), 0), f.end());
    f.shrink_to_fit();

    std::cout << " Nodes = ";
    for (int i = 0; i < nodes.size(); ++i) {
      std::cout << nodes[i] << " ";
    }
    std::cout << std::endl;

    std::cout << " Bf = ";
    for (int i = 0; i < tmp_b_f.size(); ++i) {
      std::cout << tmp_b_f[i] << " ";
    }
    std::cout << std::endl;

    std::cout << " F = ";
    for (int i = 0; i < f.size(); ++i) {
      std::cout << f[i] << " ";
    }
    std::cout << std::endl;
  }

  std::size_t map(std::size_t _pos) {
    return b_l_rank(_pos + 1);
  }

  std::size_t parent(std::size_t _pos) {
    return b_l_rank(_pos + 1);
  }

 private:
  _BVLeafNodesMarks b_l;
  _BVLeafNodesMarksRank b_l_rank;
  std::vector<uint32_t> f;

  _PTS pts_;
};

}

#endif //GRAMMAR_SAMPLED_SLP_H
