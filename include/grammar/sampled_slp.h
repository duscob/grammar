//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/26/18.
//

#ifndef GRAMMAR_SAMPLED_SLP_H
#define GRAMMAR_SAMPLED_SLP_H

#include <cstdint>
#include <vector>
#include <iterator>


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
  bool operator()(const _SLP &_slp, std::size_t _curr_var) {
    auto set = _slp.Span(_curr_var);
    sort(set.begin(), set.end());
    set.erase(unique(set.begin(), set.end()), set.end());

    pts_.AddSet(set);
  }

  template<typename _SLP, typename _Nodes, typename _RangeContainer>
  bool operator()(const _SLP &_slp,
                  std::size_t _curr_var,
                  const _Nodes &nodes,
                  const _RangeContainer &left_ranges,
                  const _RangeContainer &right_ranges) {
    auto set = _slp.Span(_curr_var);
    sort(set.begin(), set.end());
    set.erase(unique(set.begin(), set.end()), set.end());

    pts_.AddSet(set);
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
                             std::size_t curr_var,
                             _Action _action = NoAction()) {
  if (curr_var == 0)
    curr_var = _slp.Start();

  auto length = _slp.SpanLength(curr_var);
  if (length <= _block_size) {
    _out = curr_var;
    ++_out;

    _action(_slp, curr_var);

    return;
  }

  const auto &children = _slp[curr_var];

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
void ComputeSampledSLP(const _SLP &_slp,
                       uint32_t _block_size,
                       std::vector<_SLPValueType> &_nodes,
                       const _Predicate &_pred,
                       _Action _action = NoAction()) {
  std::size_t curr_leaf = 0;
  ComputeSampledSLP(_slp, _slp.Start(), _block_size, _nodes, curr_leaf, _pred, _action);
}


template<typename _SLP, typename _SLPValueType = uint32_t, typename _Predicate, typename _Action = NoAction>
std::vector<std::pair<std::size_t, std::size_t>> ComputeSampledSLP(const _SLP &_slp,
                                                                   std::size_t _curr_var,
                                                                   uint32_t _block_size,
                                                                   std::vector<_SLPValueType> &_nodes,
                                                                   std::size_t &_curr_leaf,
                                                                   const _Predicate &_pred,
                                                                   _Action _action = NoAction()) {
  auto length = _slp.SpanLength(_curr_var);
  if (length <= _block_size)
    return {{_curr_leaf++, 1}};

  const auto &children = _slp[_curr_var];

  auto left_children = ComputeSampledSLP(_slp, children.first, _block_size, _nodes, _curr_leaf, _pred, _action);
  auto right_children = ComputeSampledSLP(_slp, children.second, _block_size, _nodes, _curr_leaf, _pred, _action);

  if (_pred(_slp, _curr_var, left_children, right_children) || _curr_var == _slp.Start()) {
    _nodes.emplace_back(_curr_var);
    _action(_slp, _curr_var, _nodes, left_children, right_children);
    return {{_nodes.size() - 1, 1}};
  } else {
    left_children.insert(left_children.end(), right_children.begin(), right_children.end());
    return left_children;
  }
}

}

#endif //GRAMMAR_SAMPLED_SLP_H
