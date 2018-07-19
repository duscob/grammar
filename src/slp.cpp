//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/16/18.
//

#include "grammar/slp.h"

#include <limits>
#include <cassert>


grammar::SLP::SLP(std::size_t sigma) : sigma_(sigma) {}


std::size_t grammar::SLP::Sigma() const {
  return sigma_;
}


std::size_t grammar::SLP::AddRule(std::size_t left, std::size_t right, std::size_t span_length) {
  assert(left <= sigma_ + rules_.size() && right <= sigma_ + rules_.size());

  if (span_length == 0)
    span_length = SpanLength(left) + SpanLength(right);

  rules_.emplace_back(std::make_pair(std::make_pair(left, right), span_length));

  return sigma_ + rules_.size();
}


std::size_t grammar::SLP::Variables() const {
  return sigma_ + rules_.size();
}


std::size_t grammar::SLP::Start() const {
  return sigma_ + rules_.size();
}


const std::pair<std::size_t, std::size_t> &grammar::SLP::operator[](std::size_t i) const {
  return rules_.at(i - sigma_ - 1).first;
}


bool grammar::SLP::IsTerminal(std::size_t i) const {
  return i <= sigma_;
}


std::vector<std::size_t> grammar::SLP::Span(std::size_t i) const {
  if (IsTerminal(i))
    return {i};

  auto children = (*this)[i];
  auto left = Span(children.first);
  auto right = Span(children.second);
  left.insert(left.end(), right.begin(), right.end());
  return left;
}


std::size_t grammar::SLP::SpanLength(std::size_t i) const {
  if (i <= sigma_)
    return 1;
  else
    return rules_.at(i - sigma_ - 1).second;
}


void grammar::SLP::Reset(std::size_t sigma) {
  sigma_ = sigma;
  rules_.clear();
}

