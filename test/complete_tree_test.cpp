//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 25-06-18.
//


#include <gtest/gtest.h>

#include "grammar/complete_tree.h"


class SimpleWrapper {
 public:
  SimpleWrapper(std::vector<std::pair<int, int>> &_result) : result{_result} {}

  void operator()(int id, int id_left_child, int id_right_child, int height) {
    result.emplace_back(std::make_pair(id_left_child, id_right_child));
  }

 private:
  std::vector<std::pair<int, int>> &result;
};

class GetSameValue {
 public:
  template<typename Value>
  Value operator()(Value _v) const {
    return _v;
  }
};


class CompleteTreeTF : public ::testing::TestWithParam<std::tuple<std::vector<int>, std::vector<std::pair<int, int>>>> {
};

TEST_P(CompleteTreeTF, construct) {
  const auto &leaves = std::get<0>(GetParam());

  std::vector<std::pair<int, int>> result;
  SimpleWrapper out(result);

  grammar::BalanceTreeByWeight(leaves.begin(), leaves.end(), out, GetSameValue());

  EXPECT_EQ(result, std::get<1>(GetParam()));
}

INSTANTIATE_TEST_CASE_P(CompleteTreeByHeight,
                        CompleteTreeTF,
                        ::testing::Values(
                            std::make_tuple(std::vector<int>{0},
                                            std::vector<std::pair<int, int>>{}),
                            std::make_tuple(std::vector<int>{10, 4},
                                            std::vector<std::pair<int, int>>{{0, 1}}),
                            std::make_tuple(std::vector<int>{10, 4, 5},
                                            std::vector<std::pair<int, int>>{{1, 2}, {0, 3}}),
                            std::make_tuple(std::vector<int>{0, 0, 0, 0, 0, 0, 0, 0},
                                            std::vector<std::pair<int, int>>{{0, 1}, {2, 3}, {4, 5}, {6, 7}, {8, 9},
                                                                             {10, 11}, {12, 13}}),
                            std::make_tuple(std::vector<int>{1, 2, 0, 3, 1, 1, 0, 0, 3},
                                            std::vector<std::pair<int, int>>{{6, 7}, {4, 5}, {1, 2}, {10, 9}, {0, 11},
                                                                             {3, 12}, {14, 8}, {13, 15}})));