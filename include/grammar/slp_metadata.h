//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/17/18.
//

#ifndef GRAMMAR_SLP_METADATA_H
#define GRAMMAR_SLP_METADATA_H

#include <vector>
#include <cstdint>


namespace grammar {

/**
 * Precomputed Terminal Set
 */
template<typename _Container = std::vector<uint32_t>>
class PTS {
 public:
  PTS() = default;

  /**
   * Construct the set of terminals for each variable
   *
   * @tparam _SLP Straight-Line Program (Grammar)
   * @param slp
   */
  template<typename _SLP>
  PTS(const _SLP &slp) {
    Compute(slp);
  }

  /**
   * Compute the set of terminals for each variable
   *
   * @tparam _SLP Straight-Line Program (Grammar)
   * @param slp
   */
  template<typename _SLP>
  void Compute(const _SLP &slp) {
    sets_.reserve(slp.Variables() + 1);

    std::vector<typename _Container::value_type> set(0);
    sets_.emplace_back(set);

    std::size_t i = 1;
    set.reserve(1);
    set.push_back(0);
    for (; i <= slp.Sigma(); ++i) {
      set[0] = i;
      sets_.emplace_back(set);
    }

    set.reserve(slp.Sigma());
    for (; i <= slp.Variables(); ++i) {
      const auto &right_hand = slp[i];
      const auto &left = sets_[right_hand.first];
      const auto &right = sets_[right_hand.second];

      set.clear();
      set_union(left.begin(), left.end(), right.begin(), right.end(), back_inserter(set));
      sets_.emplace_back(set);
    }
  }

  /**
   * Get the set of terminal of variable i
   *
   * @param i variable
   * @return set of terminal
   */
  const _Container &operator[](std::size_t i) const {
    return sets_.at(i);
  }

 protected:
  std::vector<_Container> sets_;
};


/**
 * Sampled Precomputed Terminal Set
 *
 * @tparam _ValueType
 */
template<typename _ValueType = uint32_t>
class SampledPTS {
 public:
  template<typename _Set>
  std::size_t AddSet(const _Set &_set) {
    return (*this)(begin(_set), end(_set));
  }


  template<typename _II>
  std::size_t AddSet(_II _begin, _II _end) {
    return (*this)(_begin, _end);
  }


  template<typename _II>
  std::size_t operator()(_II _begin, _II _end) {
    if (_begin == _end)
      return 0;

    b_d.push_back(d.size());
    for (auto it = _begin; it != _end; ++it) {
      d.push_back(*it);
    }

    return b_d.size();
  }


  auto operator[](std::size_t i) {
    return std::make_pair(d.cbegin() + b_d[i - 1], (i < b_d.size()) ? d.cbegin() + b_d[i] : d.cend());
  }

 private:
  std::vector<_ValueType> d;
  std::vector<std::size_t> b_d;
};

}

#endif //GRAMMAR_SLP_METADATA_H
