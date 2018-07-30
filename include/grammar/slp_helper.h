//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/19/18.
//

#ifndef GRAMMAR_CONSTRUCT_SLP_H
#define GRAMMAR_CONSTRUCT_SLP_H

#include <cstdint>
#include <utility>
#include <vector>
#include <unordered_map>
#include <algorithm>


namespace grammar {

template<typename _SLP>
class SLPWrapper {
 public:
  SLPWrapper(_SLP &slp) : slp_{slp} {}

  void operator()(int sigma) {
    slp_.Reset(sigma);
  }

  void operator()(int left, int right, int length) {
    slp_.AddRule(left, right, length);
  }

 private:
  _SLP &slp_;
};


template<typename _SLP>
SLPWrapper<_SLP> BuildSLPWrapper(_SLP &slp) {
  return SLPWrapper<_SLP>(slp);
}


template<typename _II, typename _SLP, typename _Encoder>
void ConstructSLP(_II begin, _II end, _Encoder encoder, _SLP &slp) {
  auto wrapper = BuildSLPWrapper(slp);
  encoder.Encode(begin, end, wrapper);
}


template<typename _II, typename _SLP, typename _Encoder, typename ...Args>
void ComputeSLP(_II begin, _II end, _Encoder encoder, _SLP &slp, Args ...args) {
  ConstructSLP(begin, end, encoder, slp);
  slp.ComputeMetadata(args...);
}


template<typename _SLP, typename _OutputIterator>
void ComputeSpanCover(const _SLP &slp,
                      std::size_t begin,
                      std::size_t end,
                      _OutputIterator oit,
                      std::size_t curr_var = 0) {
  if (begin >= end)
    return;

  if (curr_var == 0)
    curr_var = slp.Start();

  if (0 == begin && slp.SpanLength(curr_var) <= end) {
    oit = curr_var;
    ++oit;
    return;
  }

  const auto &children = slp[curr_var];
  auto left_length = slp.SpanLength(children.first);

  if (end <= left_length) {
    ComputeSpanCover(slp, begin, end, oit, children.first);
    return;
  }

  if (left_length <= begin) {
    ComputeSpanCover(slp, begin - left_length, end - left_length, oit, children.second);
    return;
  }

  ComputeSpanCoverBeginning(slp, begin, oit, children.first);
  ComputeSpanCoverEnding(slp, end - left_length, oit, children.second);
}


template<typename _SLP, typename _OutputIterator>
void ComputeSpanCoverBeginning(const _SLP &slp, std::size_t begin, _OutputIterator oit, std::size_t curr_var) {
  if (begin == 0) {
    oit = curr_var;
    ++oit;
    return;
  }

  const auto &children = slp[curr_var];
  auto left_length = slp.SpanLength(children.first);

  if (left_length <= begin) {
    ComputeSpanCoverBeginning(slp, begin - left_length, oit, children.second);
    return;
  }

  ComputeSpanCoverBeginning(slp, begin, oit, children.first);
  oit = children.second;
  ++oit;
}


template<typename _SLP, typename _OutputIterator>
void ComputeSpanCoverEnding(const _SLP &slp, std::size_t end, _OutputIterator oit, std::size_t curr_var) {
  if (slp.SpanLength(curr_var) <= end) {
    oit = curr_var;
    ++oit;
    return;
  }

  const auto &children = slp[curr_var];
  auto left_length = slp.SpanLength(children.first);

  if (end <= left_length) {
    ComputeSpanCoverEnding(slp, end, oit, children.first);
    return;
  }

  oit = children.first;
  ++oit;
  ComputeSpanCoverEnding(slp, end - left_length, oit, children.second);
}


class NoAction {
 public:
  template<typename ...Args>
  void operator()(Args... args) {}
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

    auto it = cache.find(_curr_var);
    if (it != cache.end()) {
      return it->second;
    }

    auto Count = [this](const auto &ranges) -> auto {
      std::size_t length = 0;
      for (const auto &range : ranges) {
        for (int i = 0; i < range.second; ++i) {
          // Added sets ranges start in 1, not in 0.
          length += pts_[range.first + i + 1].size();
        }
      }

      return length;
    };

    auto set = _slp.Span(_curr_var);
    sort(set.begin(), set.end());
    set.erase(unique(set.begin(), set.end()), set.end());

    auto expanded_length = Count(left_ranges) + Count(right_ranges);

    auto must_be_sampled = set.size() * storing_factor_ < expanded_length;
    cache[_curr_var] = must_be_sampled;
    return must_be_sampled;
  }

 private:
  const _PTS &pts_;
  float storing_factor_;

  mutable std::unordered_map<std::size_t, bool> cache;
};

};

#endif //GRAMMAR_CONSTRUCT_SLP_H
