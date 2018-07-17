//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/15/18.
//

#ifndef GRAMMAR_SLP_H
#define GRAMMAR_SLP_H

#include <vector>
#include <utility>


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
  SLP(std::size_t sigma);

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
  std::size_t Start();

  /**
   * Get left-hand of rule i
   *
   * @param i must be greater than sigma
   *
   * @return left-hand of the rule [left, right]
   */
  const std::pair<std::size_t, std::size_t> &operator[](std::size_t i);

  /**
   * Is i a terminal symbol?
   *
   * @param i
   *
   * @return true if i is terminal symbol
   */
  bool IsTerminal(std::size_t i);

  /**
   * Get span of rule i
   *
   * @param i
   *
   * @return sequence equal to span of rule i
   */
  std::vector<std::size_t> Span(std::size_t i);

  /**
   * Get span length of rule i in terminal symbols.
   *
   * @param i
   *
   * @return span length
   */
  std::size_t SpanLength(std::size_t i);

 protected:
  std::size_t sigma_;
  std::vector<std::pair<std::pair<std::size_t, std::size_t>, std::size_t>> rules_;
};


class SLPWrapper {
 public:
  SLPWrapper(SLP &slp);

  void operator()(int sigma);

  void operator()(int left, int right, int length);

 private:
  SLP &slp_;
};

}

#endif //GRAMMAR_SLP_H
