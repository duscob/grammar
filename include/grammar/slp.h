//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/15/18.
//

#ifndef GRAMMAR_SLP_H
#define GRAMMAR_SLP_H

#include <vector>
#include <utility>


namespace grammar {

class SLP {
 public:
  SLP(std::size_t sigma);

  std::size_t SigmaSize();

  std::size_t AddRule(std::size_t left, std::size_t right, std::size_t span_length = 0);

  std::size_t Start();

  std::pair<std::size_t, std::size_t> operator[](std::size_t i);

  bool IsTerminal(std::size_t i);

  std::vector<std::size_t> Span(std::size_t i);

  std::size_t SpanLength(std::size_t i);

 protected:
  std::size_t sigma_;
  std::vector<std::pair<std::pair<std::size_t, std::size_t>, std::size_t>> rules_;
};

}

#endif //GRAMMAR_SLP_H
