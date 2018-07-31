//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/17/18.
//

#ifndef GRAMMAR_SLP_METADATA_H
#define GRAMMAR_SLP_METADATA_H

#include <cstdint>
#include <vector>
#include <map>
#include <algorithm>
#include <cassert>

#include "slp_helper.h"


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
  PTS(const _SLP *slp) {
    Compute(slp);
  }

  /**
   * Compute the set of terminals for each variable
   *
   * @tparam _SLP Straight-Line Program (Grammar)
   * @param slp
   */
  template<typename _SLP>
  void Compute(const _SLP *slp) {
    sets_.reserve(slp->Variables() + 1);

    std::vector<typename _Container::value_type> set(0);
    sets_.emplace_back(set);

    std::size_t i = 1;
    set.reserve(1);
    set.push_back(0);
    for (; i <= slp->Sigma(); ++i) {
      set[0] = i;
      sets_.emplace_back(set);
    }

    set.reserve(slp->Sigma());
    for (; i <= slp->Variables(); ++i) {
      const auto &right_hand = (*slp)[i];
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


template<typename _ValueType = uint32_t>
class Chunks {
 public:
  typedef std::pair<typename std::vector<_ValueType>::const_iterator, typename std::vector<_ValueType>::const_iterator>
      Chunk;

  class FakeContainer : public Chunk {
   public:
    FakeContainer(Chunk &&_chunk) : Chunk(_chunk) {}

    auto begin() const {
      return Chunks<_ValueType>::Chunk::first;
    }

    auto end() const {
      return Chunks<_ValueType>::Chunk::second;
    }

    auto size() const {
      return end() - begin();
    }
  };

  template<typename _Data>
  std::size_t AddData(const _Data &_data) {
    return (*this)(begin(_data), end(_data));
  }

  template<typename _II>
  std::size_t AddData(_II _begin, _II _end) {
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

  FakeContainer operator[](std::size_t i) const {
    assert(i <= b_d.size());

    return std::make_pair(d.cbegin() + b_d[i - 1], (i < b_d.size()) ? d.cbegin() + b_d[i] : d.cend());
  }

  auto size() const {
    return b_d.size();
  }

 private:
  std::vector<_ValueType> d;
  std::vector<std::size_t> b_d;
};


/**
 *  Sampled Precomputed Terminal Set
 *
 * @tparam _SLP
 * @tparam _ValueType
 * @tparam __block_size
 */
template<typename _SLP, typename _ValueType = uint32_t, uint32_t __block_size = 256>
class SampledPTS {
 public:
  SampledPTS() = default;

  template<typename _SLPValueType = uint32_t>
  SampledPTS(const _SLP *_slp, uint32_t _block_size = __block_size, float _storing_factor = 16) {
    Compute<_SLPValueType>(_slp, _block_size, _storing_factor);
  }

  template<typename _SLPValueType = uint32_t>
  void Compute(const _SLP *_slp, uint32_t _block_size = __block_size, float _storing_factor = 16) {
    slp_ = _slp;
    std::vector<_SLPValueType> nodes;

    auto add_set = [this](const auto &_slp, std::size_t _curr_var) {
      auto set = _slp.Span(_curr_var);
      sort(set.begin(), set.end());
      set.erase(unique(set.begin(), set.end()), set.end());

      pts_.AddData(set);
      if (vars.count(_curr_var) == 0) {
        vars[_curr_var] = pts_.size();
      }
    };
    ComputeSampledSLPLeaves(*slp_, _block_size, back_inserter(nodes), add_set);

    grammar::MustBeSampled<decltype(pts_)> pred(pts_, _storing_factor);

    auto build_inner_data = [&add_set, this](
        const _SLP &_slp,
        std::size_t _curr_var,
        const auto &_nodes,
        std::size_t _new_node,
        const auto &_left_ranges,
        const auto &_right_ranges) {
      add_set(_slp, _curr_var);
    };

    ComputeSampledSLPNodes(*slp_, _block_size, nodes, pred, build_inner_data);
  }

  std::vector<_ValueType> operator[](std::size_t i) const {
    std::vector<bool> _visited(i, false);
    return GetSet(i, _visited);
  }

 private:
  const _SLP *slp_ = nullptr;

  Chunks<_ValueType> pts_;
  std::map<_ValueType, std::size_t> vars;

  template<typename _BitVector>
  std::vector<_ValueType> GetSet(std::size_t i, _BitVector &_visited) const {
    std::vector<_ValueType> set;
    auto it = vars.find(i);
    if (it != vars.end()) {
      auto s = pts_[it->second];
      copy(s.first, s.second, back_inserter(set));
    } else {
      if (i <= slp_->Sigma()) {
        return {_ValueType(i)};
      }

      auto children = (*slp_)[i];
      auto left = !_visited[children.first] ? GetSet(children.first, _visited) : std::vector<_ValueType>(0);
      auto right = !_visited[children.second] ? GetSet(children.second, _visited) : std::vector<_ValueType>(0);

      set_union(left.begin(), left.end(), right.begin(), right.end(), back_inserter(set));
    }

    _visited[i] = 1;

    return set;
  }
};

}

#endif //GRAMMAR_SLP_METADATA_H
