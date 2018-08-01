//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/18/18.
//

#ifndef GRAMMAR_SLP_INTERFACE_H
#define GRAMMAR_SLP_INTERFACE_H

#include <vector>
#include <utility>


namespace grammar {

/**
 * Straight-Line Program Interface
 *
 * This data structure represent, roughly, an interface to Context-Free Grammar in Chomsky Normal Form, where all
 * non-terminals appears at left hand of a unique rule.
 */
class SLPInterface {
 public:
  /**
   * Get sigma == size of alphabet == last symbol of alphabet
   *
   * @return sigma
   */
  virtual std::size_t Sigma() const = 0;

  /**
   * Add a new rule to SLP. left & right must be appear before (<= sigma + |rules|)
   *
   * @param left
   * @param right
   * @param span_length
   *
   * @return id/index of the new rule
   */
  virtual std::size_t AddRule(std::size_t left, std::size_t right, std::size_t span_length = 0) = 0;

  /**
   * Get number of variables == sigma + number of rules
   *
   * @return number of variables
   */
  virtual std::size_t Variables() const = 0;

  /**
   * Get start/initial rule
   *
   * @return start rule
   */
  virtual std::size_t Start() const = 0;

  /**
   * Get left-hand of rule i
   *
   * @param i must be greater than sigma
   *
   * @return left-hand of the rule [left, right]
   */
  virtual std::pair<std::size_t, std::size_t> operator[](std::size_t i) const = 0;

  /**
   * Is i a terminal symbol?
   *
   * @param i
   *
   * @return true if i is terminal symbol
   */
  virtual bool IsTerminal(std::size_t i) const = 0;

  /**
   * Get span of rule i
   *
   * @param i
   *
   * @return sequence equal to span of rule i
   */
  virtual std::vector<std::size_t> Span(std::size_t i) const = 0;

  /**
   * Get span length of rule i in terminal symbols.
   *
   * @param i
   *
   * @return span length
   */
  virtual std::size_t SpanLength(std::size_t i) const = 0;

  /**
   * Reset
   *
   * @param sigma Size of alphabet == last symbol of alphabet
   */
  virtual void Reset(std::size_t sigma) = 0;
};


/**
 * Straight-Line Program Template Interface
 *
 * This data structure represent, roughly, an interface to Context-Free Grammar in Chomsky Normal Form, where all
 * non-terminals appears at left hand of a unique rule.
 */
template<typename _SLP>
class SLPTInterface : public SLPInterface, public _SLP {
  //todo move definitions to .cpp
 public:
  template<typename _Data>
  SLPTInterface(_Data data): _SLP(data) {}

  /**
   * Get sigma == size of alphabet == last symbol of alphabet
   *
   * @return sigma
   */
  virtual std::size_t Sigma() const {
    return _SLP::Sigma();
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
  virtual std::size_t AddRule(std::size_t left, std::size_t right, std::size_t span_length = 0) {
    return _SLP::AddRule(left, right, span_length);
  }

  /**
   * Get number of variables == sigma + number of rules
   *
   * @return number of variables
   */
  virtual std::size_t Variables() const {
    return _SLP::Variables();
  }

  /**
   * Get start/initial rule
   *
   * @return start rule
   */
  virtual std::size_t Start() const {
    return _SLP::Start();
  }

  /**
   * Get left-hand of rule i
   *
   * @param i must be greater than sigma
   *
   * @return left-hand of the rule [left, right]
   */
  virtual std::pair<std::size_t, std::size_t> operator[](std::size_t i) const {
    return _SLP::operator[](i);
  };

  /**
   * Is i a terminal symbol?
   *
   * @param i
   *
   * @return true if i is terminal symbol
   */
  virtual bool IsTerminal(std::size_t i) const {
    return _SLP::IsTerminal(i);
  }

  /**
   * Get span of rule i
   *
   * @param i
   *
   * @return sequence equal to span of rule i
   */
  virtual std::vector<std::size_t> Span(std::size_t i) const {
    auto span = _SLP::Span(i);
    return std::vector<std::size_t>(span.begin(), span.end());
  }

  /**
   * Get span length of rule i in terminal symbols.
   *
   * @param i
   *
   * @return span length
   */
  virtual std::size_t SpanLength(std::size_t i) const {
    return _SLP::SpanLength(i);
  }

  /**
   * Reset
   *
   * @param sigma Size of alphabet == last symbol of alphabet
   */
  virtual void Reset(std::size_t sigma) {
    _SLP::Reset(sigma);
  }
};

}

#endif //GRAMMAR_SLP_INTERFACE_H
