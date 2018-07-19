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

};

#endif //GRAMMAR_CONSTRUCT_SLP_H
