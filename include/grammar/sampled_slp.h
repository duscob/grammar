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

}

#endif //GRAMMAR_SAMPLED_SLP_H
