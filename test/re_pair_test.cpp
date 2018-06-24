//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 23-06-18.
//

#include <gtest/gtest.h>

#include "grammar/re_pair.h"


struct NonTerminal {
  int left;
  int right;
  int length;
};


bool operator==(const NonTerminal &_nt1, const NonTerminal &_nt2) {
  return _nt1.left == _nt2.left && _nt1.right == _nt2.right && _nt1.length == _nt2.length;
}


class NonTerminalWrapper {
 public:
  NonTerminalWrapper(std::vector<NonTerminal> &_non_terminals) : non_terminals(_non_terminals) {}

  void operator()(int left, int right, int length) {
    non_terminals.emplace_back(NonTerminal{left, right, length});
  }

 private:
  std::vector<NonTerminal> &non_terminals;
};


class RePairEncoderTF : public ::testing::TestWithParam<std::tuple<std::vector<int>, std::vector<NonTerminal>>> {
};


TEST_P(RePairEncoderTF, encode) {
  std::vector<NonTerminal> non_terminals;
  NonTerminalWrapper out(non_terminals);

  grammar::RePairEncoder encoder;
  encoder.encode(std::get<0>(GetParam()).begin(), std::get<0>(GetParam()).end(), out);

  EXPECT_EQ(non_terminals, std::get<1>(GetParam()));
}


INSTANTIATE_TEST_CASE_P(RePairEncoderWithVectorInt,
                        RePairEncoderTF,
                        ::testing::Values(
                            std::make_tuple(std::vector<int>({4, 3, 2, 1, 3, 2, 1, 3, 3, 3, 2, 1, 2, 2, 1, 1}),
                                            std::vector<NonTerminal>({{2, 1, 2}, {3, 5, 3}})),
                            std::make_tuple(std::vector<int>({1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8}),
                                            std::vector<NonTerminal>({{1, 2, 2}, {3, 4, 2}, {6, 7, 2}, {10, 5, 3},
                                                                      {11, 8, 3}, {9, 12, 5}, {14, 13, 8}}))
                        ));


template<typename T>
class RepairEncoderTypeTF : public ::testing::Test {};


typedef ::testing::Types<std::vector<int>, std::vector<char>> MyTypes;
TYPED_TEST_CASE(RepairEncoderTypeTF, MyTypes);


TYPED_TEST(RepairEncoderTypeTF, encode) {
  TypeParam data = {1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8};

  std::vector<NonTerminal> non_terminals;
  NonTerminalWrapper out(non_terminals);

  grammar::RePairEncoder encoder;
  encoder.encode(data.begin(), data.end(), out);

  EXPECT_EQ(non_terminals, std::vector<NonTerminal>({{1, 2, 2}, {3, 4, 2}, {6, 7, 2}, {10, 5, 3},
                                                     {11, 8, 3}, {9, 12, 5}, {14, 13, 8}}));
}