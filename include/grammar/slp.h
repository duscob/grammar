//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/15/18.
//

#ifndef GRAMMAR_SLP_H
#define GRAMMAR_SLP_H

#include <vector>


namespace grammar {

class SLP {
 public:
  SLP(std::size_t sigma) : sigma_(sigma) {}

  std::size_t SigmaSize() {
    return sigma_;
  }


  std::size_t AddRule(std::size_t left, std::size_t right) {
    assert(left <= sigma_ + rules_.size() && right <= sigma_ + rules_.size());
    rules_.emplace_back(left, right);

    return sigma_ + rules_.size();
  }


  std::size_t Start() {
    return sigma_ + rules_.size();
  }


  std::pair<std::size_t, std::size_t> operator[](std::size_t i) {
    if (i <= sigma_)
      return std::make_pair(i, std::numeric_limits<std::size_t>::max());
    else
      return rules_.at(i - sigma_ - 1);
  }

 protected:
  std::size_t sigma_;
  std::vector<std::pair<int, int>> rules_;
};

}

#endif //GRAMMAR_SLP_H
