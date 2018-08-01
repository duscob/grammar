//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/15/18.
//

#ifndef GRAMMAR_SLP_H
#define GRAMMAR_SLP_H

#include <vector>
#include <utility>

#include "io.h"


namespace grammar {

/**
 * Straight-Line Program
 *
 * This data structure represent, roughly, a Context-Free Grammar in Chomsky Normal Form, where all non-terminals
 * appears at left hand of a unique rule.
 */
class SLP {
 public:
  /**
   * Constructor
   *
   * @param sigma Size of alphabet == last symbol of alphabet
   */
  SLP(std::size_t sigma = 0);

  /**
   * Get sigma == size of alphabet == last symbol of alphabet
   *
   * @return sigma
   */
  std::size_t Sigma() const;

  /**
   * Add a new rule to SLP. left & right must be appear before (<= sigma + |rules|)
   *
   * @param left
   * @param right
   * @param span_length
   *
   * @return id/index of the new rule
   */
  std::size_t AddRule(std::size_t left, std::size_t right, std::size_t span_length = 0);

  /**
   * Get number of variables == sigma + number of rules
   *
   * @return number of variables
   */
  std::size_t Variables() const;

  /**
   * Get start/initial rule
   *
   * @return start rule
   */
  std::size_t Start() const;

  /**
   * Get left-hand of rule i
   *
   * @param i must be greater than sigma
   *
   * @return left-hand of the rule [left, right]
   */
  const std::pair<std::size_t, std::size_t> &operator[](std::size_t i) const;

  /**
   * Is i a terminal symbol?
   *
   * @param i
   *
   * @return true if i is terminal symbol
   */
  bool IsTerminal(std::size_t i) const;

  /**
   * Get span of rule i
   *
   * @param i
   *
   * @return sequence equal to span of rule i
   */
  std::vector<std::size_t> Span(std::size_t i) const;

  /**
   * Get span length of rule i in terminal symbols.
   *
   * @param i
   *
   * @return span length
   */
  std::size_t SpanLength(std::size_t i) const;

  /**
   * Reset
   *
   * @param sigma Size of alphabet == last symbol of alphabet
   */
  void Reset(std::size_t sigma);

  bool operator==(const SLP &_slp) const {
    return sigma_ == _slp.sigma_ && rules_ == _slp.rules_;
  }

  bool operator!=(const SLP &_slp) const {
    return !(*this == _slp);
  }

  std::size_t serialize(std::ostream &out) const {
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
  std::size_t sigma_;
  std::vector<std::pair<std::pair<std::size_t, std::size_t>, std::size_t>> rules_;
};


/**
 * Straight-Line Program With Metadata
 *
 * This data structure represent, roughly, a Context-Free Grammar in Chomsky Normal Form, where all non-terminals
 * appears at left hand of a unique rule. Represent also some metadata associated to the variables of SLP.
 *
 * @tparam _Metadata
 */
template<typename _Metadata>
class SLPWithMetadata : public SLP {
 public:
  template<typename ...Args>
  SLPWithMetadata(Args ...args): SLP(args...) {}

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
