//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/17/18.
//

#ifndef GRAMMAR_SLP_METADATA_H
#define GRAMMAR_SLP_METADATA_H

#include <vector>


namespace grammar {

/**
 * Precomputed Terminal Set
 */
class PTS {
 public:
  /**
   * Construct the set of terminals for each variable
   *
   * @tparam _SLP Straight-Line Program (Grammar)
   * @param slp
   */
  template<typename _SLP>
  PTS(const _SLP &slp) {
    sets_.reserve(slp.Variables() + 1);
    sets_.emplace_back(0);

    std::size_t i = 1;
    for (; i <= slp.Sigma(); ++i) {
      sets_.emplace_back(1, i);
    }

    for (; i <= slp.Variables(); ++i) {
      const auto &right_hand = slp[i];
      const auto &left = sets_[right_hand.first];
      const auto &right = sets_[right_hand.second];

      sets_.emplace_back(0);
      sets_.reserve(left.size() + right.size());
      set_union(begin(left), end(left), begin(right), end(right), back_inserter(sets_.back()));
      sets_.back().shrink_to_fit();
    }
  }

  /**
   * Get the set of terminal of variable i
   *
   * @param i variable
   * @return set of terminal
   */
  const std::vector<std::size_t> &operator[](std::size_t i) const {
    return sets_.at(i);
  }

 protected:
  std::vector<std::vector<std::size_t>> sets_;
};

}

#endif //GRAMMAR_SLP_METADATA_H
