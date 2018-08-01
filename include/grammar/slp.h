//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/15/18.
//

#ifndef GRAMMAR_SLP_H
#define GRAMMAR_SLP_H

#include <vector>
#include <utility>
#include <cassert>

#include "io.h"


namespace grammar {

/**
 * Straight-Line Program
 *
 * This data structure represent, roughly, a Context-Free Grammar in Chomsky Normal Form, where all non-terminals
 * appears at left hand of a unique rule.
 */
template <typename _VariableType = uint32_t, typename _LengthType = uint32_t>
class SLP {
 public:
  typedef std::size_t size_type;

  /**
   * Constructor
   *
   * @param sigma Size of alphabet == last symbol of alphabet
   */
  SLP(_VariableType sigma = 0) : sigma_(sigma) {}

  /**
   * Get sigma == size of alphabet == last symbol of alphabet
   *
   * @return sigma
   */
  auto Sigma() const {
    return sigma_;
  }


  /**
   * Add a new rule to SLP. left & right must be appear before (<= sigma + |rules|)
   *
   * @param left
   * @param right
   * @param span_length
   *
   * @return id/index of the new rule
   */
  _VariableType AddRule(_VariableType left, _VariableType right, _LengthType span_length = 0) {
    assert(left <= sigma_ + rules_.size() && right <= sigma_ + rules_.size());

    if (span_length == 0)
      span_length = SpanLength(left) + SpanLength(right);

    rules_.emplace_back(std::make_pair(std::make_pair(left, right), span_length));

    return sigma_ + rules_.size();
  }

  /**
   * Get number of variables == sigma + number of rules
   *
   * @return number of variables
   */
  _VariableType Variables() const {
    return sigma_ + rules_.size();
  }

  /**
   * Get start/initial rule
   *
   * @return start rule
   */
  _VariableType Start() const {
    return sigma_ + rules_.size();
  }

  /**
   * Get left-hand of rule i
   *
   * @param i must be greater than sigma
   *
   * @return left-hand of the rule [left, right]
   */
  const auto &operator[](_VariableType i) const {
    return rules_.at(i - sigma_ - 1).first;
  }

  /**
   * Is i a terminal symbol?
   *
   * @param i
   *
   * @return true if i is terminal symbol
   */
  bool IsTerminal(_VariableType i) const {
    return i <= sigma_;
  }

  /**
   * Get span of rule i
   *
   * @param i
   *
   * @return sequence equal to span of rule i
   */
  std::vector<_VariableType> Span(_VariableType i) const {
    if (IsTerminal(i))
      return {i};

    auto children = (*this)[i];
    auto left = Span(children.first);
    auto right = Span(children.second);
    left.insert(left.end(), right.begin(), right.end());
    return left;
  }

  /**
   * Get span length of rule i in terminal symbols.
   *
   * @param i
   *
   * @return span length
   */
  _LengthType SpanLength(_VariableType i) const {
    if (i <= sigma_)
      return 1;
    else
      return rules_.at(i - sigma_ - 1).second;
  }

  /**
   * Reset
   *
   * @param sigma Size of alphabet == last symbol of alphabet
   */
  void Reset(_VariableType sigma) {
    sigma_ = sigma;
    rules_.clear();
  }

  bool operator==(const SLP &_slp) const {
    return sigma_ == _slp.sigma_ && rules_ == _slp.rules_;
  }

  bool operator!=(const SLP &_slp) const {
    return !(*this == _slp);
  }

  std::size_t serialize(std::ostream &out, sdsl::structure_tree_node* v=nullptr, std::string name="") const {
    std::size_t written_bytes = 0;
    written_bytes += sdsl::serialize(sigma_, out);
    written_bytes += sdsl::serialize(rules_, out);

    return written_bytes;
  }

  void load(std::istream &in) {
    sdsl::load(sigma_, in);
    sdsl::load(rules_, in);
  }

 protected:
  _VariableType sigma_;
  std::vector<std::pair<std::pair<_VariableType, _VariableType>, _LengthType>> rules_;
};


/**
 * Straight-Line Program With Metadata
 *
 * This data structure represent, roughly, a Context-Free Grammar in Chomsky Normal Form, where all non-terminals
 * appears at left hand of a unique rule. Represent also some metadata associated to the variables of SLP.
 *
 * @tparam _Metadata
 */
template<typename _Metadata, typename _SLP = SLP<>>
class SLPWithMetadata : public _SLP {
 public:
  template<typename ...Args>
  SLPWithMetadata(Args ...args): _SLP(args...) {}

  /**
   * Compute the metadata. This methods must be called before GetData.
   */
  template<typename ...Args>
  void ComputeMetadata(Args ...args) {
    metadata_.Compute(this, args...);
  }

  /**
   * Get metadata associated to variable i
   *
   * @param i
   *
   * @return metadata
   */
  auto GetData(std::size_t i) {
    // todo verify ComputeMetadata was called before
    return metadata_[i];
  }

 private:
  _Metadata metadata_;
};

}

#endif //GRAMMAR_SLP_H
