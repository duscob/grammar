//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 25-06-18.
//


#include <gtest/gtest.h>

#include "grammar/complete_tree.h"


class SimpleWrapper {
 public:
  SimpleWrapper(std::vector<std::pair<int, int>> &_result) : result{_result} {}

  void operator()(int id, int id_left_child, int id_right_child, int height) {
    std::cout << id << std::endl;
    result.emplace_back(std::make_pair(id_left_child, id_right_child));
  }

 private:
  std::vector<std::pair<int, int>> &result;
};


class GetSameValue {
 public:
  template<typename Value>
  Value operator()(Value _v) {
    return _v;
  }
};


class TreeHeight {
 public:
  template<typename Value>
  Value operator()(Value _v1, Value _v2) {
    return std::max(_v1, _v2) + 1;
  }
};


TEST(CompleteTreeTest, construct) {
  std::vector<int> leaves = {1, 2, 0, 3, 1, 1, 0, 0, 3};

  std::vector<std::pair<int, int>> result;
  SimpleWrapper out(result);

  grammar::BalanceTreeByWeight(leaves.begin(), leaves.end(), out, GetSameValue(), TreeHeight());

  std::vector<std::pair<int, int>> eresult = {{6, 7}, {4, 5}, {1, 2}, {10, 9}, {0, 11}, {3, 12}, {14, 8}, {13, 15}};
  EXPECT_EQ(result, eresult);
}