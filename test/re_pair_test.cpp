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
  NonTerminalWrapper(std::vector<NonTerminal> &_non_terminals) : non_terminals{_non_terminals} {}

  void operator()(int left, int right, int length) {
    non_terminals.emplace_back(NonTerminal{left, right, length});
  }

 private:
  std::vector<NonTerminal> &non_terminals;
};

class CompactSequenceWrapper {
 public:
  CompactSequenceWrapper(std::vector<int> &_compact_seq) : compact_seq{_compact_seq} {}
  void operator()(int val) {
    compact_seq.push_back(val);
  }

 private:
  std::vector<int> &compact_seq;
};


class RePairEncoderTF : public ::testing::TestWithParam<std::tuple<std::vector<int>,
                                                                   std::vector<NonTerminal>,
                                                                   std::vector<int>>> {};

TEST_P(RePairEncoderTF, encode) {
  std::vector<NonTerminal> non_terminals;
  NonTerminalWrapper report_non_terminals(non_terminals);
  std::vector<int> compact_seq;
  CompactSequenceWrapper report_cseq(compact_seq);

  grammar::RePairEncoder<false> encoder;

  encoder.Encode(std::get<0>(GetParam()).begin(), std::get<0>(GetParam()).end(), report_non_terminals, report_cseq);
  EXPECT_EQ(non_terminals, std::get<1>(GetParam()));
  EXPECT_EQ(compact_seq, std::get<2>(GetParam()));

  non_terminals.clear();
  compact_seq.clear();
  encoder.Encode(std::get<0>(GetParam()).begin(), std::get<0>(GetParam()).end(), report_non_terminals, report_cseq);
  EXPECT_EQ(non_terminals, std::get<1>(GetParam()));
  EXPECT_EQ(compact_seq, std::get<2>(GetParam()));
}

INSTANTIATE_TEST_CASE_P(RePairEncoder,
                        RePairEncoderTF,
                        ::testing::Values(
                            std::make_tuple(std::vector<int>({4, 3, 2, 1, 3, 2, 1, 3, 3, 3, 2, 1, 2, 2, 1, 1}),
                                            std::vector<NonTerminal>({{2, 1, 2}, {3, 5, 3}}),
                                            std::vector<int>{4, 6, 6, 3, 3, 6, 2, 5, 1}),
                            std::make_tuple(std::vector<int>({1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8}),
                                            std::vector<NonTerminal>({{1, 2, 2}, {3, 4, 2}, {6, 7, 2}, {10, 5, 3},
                                                                      {11, 8, 3}, {9, 12, 5}, {14, 13, 8}}),
                                            std::vector<int>{15, 15}),
                            std::make_tuple(std::vector<int>({1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 9, 10, 11, 12, 13, 14}),
                                            std::vector<NonTerminal>({{1, 2, 2}}),
                                            std::vector<int>{15, 3, 4, 5, 6, 7, 8, 15, 9, 10, 11, 12, 13, 14})
                        ));


class RePairEncoderInChomskyNFTF : public ::testing::TestWithParam<std::tuple<std::vector<int>,
                                                                              std::vector<NonTerminal>>> {};

TEST_P(RePairEncoderInChomskyNFTF, encode) {
  std::vector<NonTerminal> non_terminals;
  NonTerminalWrapper report_non_terminals(non_terminals);

  grammar::RePairEncoder<true> encoder;
  encoder.Encode(std::get<0>(GetParam()).begin(), std::get<0>(GetParam()).end(), report_non_terminals);
  EXPECT_EQ(non_terminals, std::get<1>(GetParam()));

  non_terminals.clear();
  encoder.Encode(std::get<0>(GetParam()).begin(), std::get<0>(GetParam()).end(), report_non_terminals);
  EXPECT_EQ(non_terminals, std::get<1>(GetParam()));
}

INSTANTIATE_TEST_CASE_P(RePairEncoder,
                        RePairEncoderInChomskyNFTF,
                        ::testing::Values(
                            std::make_tuple(std::vector<int>({1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8}),
                                            std::vector<NonTerminal>({{1, 2, 2}, {3, 4, 2}, {6, 7, 2}, {10, 5, 3},
                                                                      {11, 8, 3}, {9, 12, 5}, {14, 13, 8},
                                                                      {15, 15, 16}})),
                            std::make_tuple(std::vector<int>({4, 3, 2, 1, 3, 2, 1, 3, 3, 3, 2, 1, 2, 2, 1, 1}),
                                            std::vector<NonTerminal>({{2, 1, 2}, {3, 5, 3}, {3, 3, 2}, {2, 5, 3},
                                                                      {4, 6, 4}, {8, 1, 4}, {6, 7, 5}, {11, 6, 8},
                                                                      {9, 12, 12}, {13, 10, 16}})),
                            std::make_tuple(std::vector<int>({1, 2, 3, 4, 5, 6, 7, 8}),
                                            std::vector<NonTerminal>({{1, 2, 2}, {3, 4, 2}, {5, 6, 2}, {7, 8, 2},
                                                                      {9, 10, 4}, {11, 12, 4}, {13, 14, 8}})),
                            std::make_tuple(std::vector<int>({1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 9, 10, 11, 12, 13, 14}),
                                            std::vector<NonTerminal>({{1, 2, 2}, {3, 4, 2}, {5, 6, 2}, {7, 8, 2},
                                                                      {9, 10, 2}, {11, 12, 2}, {13, 14, 2},
                                                                      {15, 16, 4}, {17, 18, 4}, {15, 19, 4},
                                                                      {20, 21, 4}, {22, 23, 8}, {24, 25, 8},
                                                                      {26, 27, 16}})),
                            std::make_tuple(std::vector<int>({8, 3, 6, 8, 3}),
                                            std::vector<NonTerminal>({{8, 3, 2}, {9, 6, 3}, {10, 9, 5}}))
                        )
);


template<typename T>
class RepairEncoderTypeTF : public ::testing::Test {};

typedef ::testing::Types<std::vector<int>, std::vector<char>> MyTypes;
TYPED_TEST_CASE(RepairEncoderTypeTF, MyTypes);

TYPED_TEST(RepairEncoderTypeTF, encode) {
  TypeParam data = {1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8};

  std::vector<NonTerminal> non_terminals;
  NonTerminalWrapper report_non_terminals(non_terminals);
  std::vector<int> compact_seq;
  CompactSequenceWrapper report_cseq(compact_seq);

  grammar::RePairEncoder<false> encoder;
  encoder.Encode(data.begin(), data.end(), report_non_terminals, report_cseq);
  EXPECT_EQ(non_terminals, std::vector<NonTerminal>({{1, 2, 2}, {3, 4, 2}, {6, 7, 2}, {10, 5, 3},
                                                     {11, 8, 3}, {9, 12, 5}, {14, 13, 8}}));
  EXPECT_EQ(compact_seq, std::vector<int>({15, 15}));
}

TYPED_TEST(RepairEncoderTypeTF, encodeInCNF) {
  TypeParam data = {1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8};

  std::vector<NonTerminal> non_terminals;
  NonTerminalWrapper report_non_terminals(non_terminals);

  grammar::RePairEncoder<true> encoder;
  encoder.Encode(data.begin(), data.end(), report_non_terminals);
  EXPECT_EQ(non_terminals,
            std::vector<NonTerminal>({{1, 2, 2}, {3, 4, 2}, {6, 7, 2}, {10, 5, 3}, {11, 8, 3}, {9, 12, 5}, {14, 13, 8},
                                      {15, 15, 16}}));
}
