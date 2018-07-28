//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/19/18.
//

#ifndef GRAMMAR_CONSTRUCT_SLP_H
#define GRAMMAR_CONSTRUCT_SLP_H

#include "slp.h"


namespace grammar {

template<typename _II, typename _SLP, typename _Encoder>
void ConstructSLP(_II begin, _II end, _Encoder encoder, _SLP &slp) {
  auto wrapper = grammar::BuildSLPWrapper(slp);
  encoder.Encode(begin, end, wrapper);
}


template<typename _II, typename _SLP, typename _Encoder>
void ComputeSLP(_II begin, _II end, _Encoder encoder, _SLP &slp) {
  ConstructSLP(begin, end, encoder, slp);
  slp.ComputeMetadata();
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

};

#endif //GRAMMAR_CONSTRUCT_SLP_H
