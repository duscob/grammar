//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/15/18.
//

#ifndef GRAMMAR_SLP_H
#define GRAMMAR_SLP_H

#include <vector>
#include <utility>
#include <cassert>

#include "utility.h"
#include "io.h"


namespace grammar {

/**
 * Straight-Line Program
 *
 * This data structure represent, roughly, a Context-Free Grammar in Chomsky Normal Form, where all non-terminals
 * appears at left hand of a unique rule.
 */
template<typename _VarsContainer = std::vector<uint32_t>, typename _LengthsContainer = std::vector<uint32_t>>
class SLP {
 public:
  typedef std::size_t size_type;
  typedef typename _VarsContainer::value_type VariableType;
  typedef typename _LengthsContainer::value_type LengthType;

  /**
   * Constructor
   *
   * @param sigma Size of alphabet == last symbol of alphabet
   */
  SLP(VariableType sigma = 0) : sigma_(sigma) {}

  template<typename __VarsContainer, typename __LengthsContainer, typename _ActionVars = NoAction, typename _ActionLengths = NoAction>
  SLP(const SLP<__VarsContainer, __LengthsContainer> &_slp,
      _ActionVars &&_action_rules = NoAction(),
      _ActionLengths &&_action_lengths = NoAction()) {
    sigma_ = _slp.Sigma();
    Construct(rules_, _slp.GetRules());
    _action_rules(rules_);
    Construct(lengths_, _slp.GetRulesLengths());
    _action_lengths(lengths_);
  }

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
  template<typename __VariableType1, typename __VariableType2, typename __LengthType = LengthType>
  auto AddRule(__VariableType1 left, __VariableType2 right, __LengthType span_length = 0)
  -> typename std::enable_if<
      std::integral_constant<
          bool,
          has_push_back<_VarsContainer, void(VariableType)>::value
              && std::is_convertible<__VariableType1, VariableType>::value
              && std::is_convertible<__VariableType2, VariableType>::value
              && has_push_back<_LengthsContainer, void(LengthType)>::value
              && std::is_convertible<__LengthType, LengthType>::value
      >::value,
      VariableType
  >::type {
    if (span_length == 0)
      span_length = SpanLength(left) + SpanLength(right);

    rules_.push_back(left);
    rules_.push_back(right);
    lengths_.push_back(span_length);

    return sigma_ + rules_.size() / 2;
  }

  /**
   * Get number of variables == sigma + number of rules
   *
   * @return number of variables
   */
  VariableType Variables() const {
    return sigma_ + rules_.size() / 2;
  }

  /**
   * Get start/initial rule
   *
   * @return start rule
   */
  VariableType Start() const {
    return sigma_ + rules_.size() / 2;
  }

  /**
   * Get left-hand of rule i
   *
   * @param i must be greater than sigma
   *
   * @return left-hand of the rule [left, right]
   */
  std::pair<VariableType, VariableType> operator[](VariableType i) const {
    auto pos = (i - sigma_ - 1) * 2;
    return {rules_[pos], rules_[pos + 1]};
  }

  /**
   * Is i a terminal symbol?
   *
   * @param i
   *
   * @return true if i is terminal symbol
   */
  bool IsTerminal(VariableType i) const {
    return i <= sigma_;
  }

  /**
   * Get span of rule i
   *
   * @param i
   *
   * @return sequence equal to span of rule i
   */
  std::vector<VariableType> Span(VariableType i) const {
    if (IsTerminal(i))
      return {i};

    std::vector<VariableType> span = {};
    span.reserve(SpanLength(i));

    Span(i, back_inserter(span));
    return span;
  }

  template<typename _OI>
  void Span(VariableType i, _OI &&_out) const {
    if (IsTerminal(i)) {
      _out = i;
      ++_out;
      return;
    }

    auto children = (*this)[i];
    Span(children.first, _out);
    Span(children.second, _out);
  }

  /**
   * Get span length of rule i in terminal symbols.
   *
   * @param i
   *
   * @return span length
   */
  LengthType SpanLength(VariableType i) const {
    if (i <= sigma_)
      return 1;

    return lengths_[i - sigma_ - 1];
  }

  /**
   * Reset
   *
   * @param sigma Size of alphabet == last symbol of alphabet
   */
  void Reset(VariableType sigma) {
    sigma_ = sigma;
    rules_.clear();
    lengths_.clear();
  }

  const _VarsContainer &GetRules() const {
    return rules_;
  }

  const _LengthsContainer &GetRulesLengths() const {
    return lengths_;
  }

  template<typename __VarsContainer, typename __LengthsContainer>
  bool operator==(const SLP<__VarsContainer, __LengthsContainer> &_slp) const {
    return sigma_ == _slp.sigma_
        && rules_.size() == _slp.rules_.size()
        && std::equal(rules_.begin(), rules_.end(), _slp.rules_.begin())
        && lengths_.size() == _slp.lengths_.size()
        && std::equal(lengths_.begin(), lengths_.end(), _slp.lengths_.begin());
  }

  template<typename __VarsContainer, typename __LengthsContainer>
  bool operator!=(const SLP<__VarsContainer, __LengthsContainer> &_slp) const {
    return !(*this == _slp);
  }

  std::size_t serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
    std::size_t written_bytes = 0;
    written_bytes += sdsl::serialize(sigma_, out);
    written_bytes += sdsl::serialize(rules_, out);
    written_bytes += sdsl::serialize(lengths_, out);

    return written_bytes;
  }

  void load(std::istream &in) {
    sdsl::load(sigma_, in);
    sdsl::load(rules_, in);
    sdsl::load(lengths_, in);
  }

 protected:
  VariableType sigma_ = 0;
  _VarsContainer rules_;
  _LengthsContainer lengths_;
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
