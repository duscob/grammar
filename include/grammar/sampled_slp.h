//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/26/18.
//

#ifndef GRAMMAR_SAMPLED_SLP_H
#define GRAMMAR_SAMPLED_SLP_H

#include <cstdint>
#include <vector>
#include <iterator>


namespace grammar {

template<typename _SLP, typename _OutputIterator>
void ComputeSampledSLPLeaves(const _SLP &_slp, uint64_t _block_size, _OutputIterator _out, std::size_t curr_var = 0) {
  if (curr_var == 0)
    curr_var = _slp.Start();

  auto length = _slp.SpanLength(curr_var);
  if (length <= _block_size)
    return;

  const auto &children = _slp[curr_var];

  auto left_length = _slp.SpanLength(children.first);
  if (left_length <= _block_size) {
    _out = children.first;
    ++_out;
  } else {
    ComputeSampledSLPLeaves(_slp, _block_size, _out, children.first);
  }

  auto right_length = _slp.SpanLength(children.second);
  if (right_length <= _block_size) {
    _out = children.second;
    ++_out;
  } else {
    ComputeSampledSLPLeaves(_slp, _block_size, _out, children.second);
  }
}

}

#endif //GRAMMAR_SAMPLED_SLP_H
