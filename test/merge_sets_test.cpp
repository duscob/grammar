//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 8/5/18.
//

#include <gtest/gtest.h>

#include <vector>
#include <cstdint>

#include "grammar/algorithm.h"


using Set = std::vector<uint32_t>;
using Sets = std::vector<Set>;


class MergeSets_TF : public ::testing::TestWithParam<Sets> {
 protected:
  Set idx_sets;
  Set e_set;

  void SetUp() override {
    const auto &sets = GetParam();
    idx_sets.resize(sets.size());
    for (uint32_t i = 0; i < sets.size(); ++i) {
      idx_sets[i] = i;
    }

    for (const auto &item : sets) {
      std::copy(item.begin(), item.end(), std::back_inserter(e_set));
    }
    sort(e_set.begin(), e_set.end());
    e_set.erase(unique(e_set.begin(), e_set.end()), e_set.end());
  }
};


TEST_P(MergeSets_TF, SetUnion) {
  const auto &sets = GetParam();

  std::vector<uint32_t> set, tmp_set, r_set;
  for (const auto &item : sets) {
    tmp_set.resize(set.size() + item.size());
    tmp_set.swap(set);
    r_set.resize(set.size());

    auto last_it = std::set_union(tmp_set.begin(), tmp_set.end(), item.begin(), item.end(), set.begin());
    set.resize(last_it - set.begin());

    auto last_it1 = grammar::SetUnion(tmp_set.begin(), tmp_set.end(), item.begin(), item.end(), r_set.begin());
    r_set.resize(last_it1 - r_set.begin());

    EXPECT_EQ(r_set, set);
  }
}


TEST_P(MergeSets_TF, MergeSetsOneByOne) {
  const auto &sets = GetParam();

  std::vector<uint32_t> set;
  grammar::MergeSetsOneByOne(idx_sets.begin(), idx_sets.end(), sets, set);

  EXPECT_EQ(set, e_set);
}


TEST_P(MergeSets_TF, MergeSetsBinaryTree) {
  const auto &sets = GetParam();

  std::vector<uint32_t> set;
  grammar::MergeSetsBinaryTree(idx_sets.begin(), idx_sets.end(), sets, set);

  EXPECT_EQ(set, e_set);
}


TEST_P(MergeSets_TF, MergeSetsOneByOneWithSetUnion) {
  const auto &sets = GetParam();

  auto std_set_union = [](auto _first1, auto _last1, auto _first2, auto _last2, auto _result) -> auto {
    return std::set_union(_first1, _last1, _first2, _last2, _result);
  };

  std::vector<uint32_t> set;
  grammar::MergeSetsOneByOne(idx_sets.begin(), idx_sets.end(), sets, set, std_set_union);

  EXPECT_EQ(set, e_set);
}


INSTANTIATE_TEST_CASE_P(
    MergeSets,
    MergeSets_TF,
    ::testing::Values(
        Sets{{7}},
        Sets{{1, 4, 7, 8}, {1, 2}, {2, 7}},
        Sets{{7}, {1, 4, 7, 8}, {1, 2}, {2, 7}},
        Sets{{7}, {1, 4, 7, 8}, {1, 2}, {2, 7}, {1, 2}, {2, 7}},
        Sets{{7}, {1, 4, 7, 8}, {1, 2}, {2, 7}, {1, 2}, {2, 7}, {10}},
        Sets{}
    )
);