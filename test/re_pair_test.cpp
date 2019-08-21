//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 23-06-18.
//

#include <gtest/gtest.h>

#include <gflags/gflags.h>

#include "grammar/re_pair.h"
#include "grammar/slp.h"
#include "grammar/slp_helper.h"


DEFINE_string(datapath, "", "Path of data input.");


struct NonTerminal {
  int left;
  int right;
  int length;
};


void PrintTo(const NonTerminal &nt, ::std::ostream *os) {
  *os << "{" << nt.left << "; " << nt.right << "}:" << nt.length;
}


bool operator==(const NonTerminal &_nt1, const NonTerminal &_nt2) {
  return _nt1.left == _nt2.left && _nt1.right == _nt2.right && _nt1.length == _nt2.length;
}


class NonTerminalWrapper {
 public:
  NonTerminalWrapper(int &_sigma, std::vector<NonTerminal> &_non_terminals) : sigma{_sigma},
                                                                              non_terminals{_non_terminals} {}

  void operator()(int _sigma) {
    sigma = _sigma;
  }

  void operator()(int left, int right, int length) {
    non_terminals.emplace_back(NonTerminal{left, right, length});
  }

 private:
  int &sigma;
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
                                                                   std::vector<int>>> {
};


TEST_P(RePairEncoderTF, encode) {
  int sigma;
  std::vector<NonTerminal> non_terminals;
  NonTerminalWrapper report_non_terminals(sigma, non_terminals);
  std::vector<int> compact_seq;
  CompactSequenceWrapper report_cseq(compact_seq);

  const auto &sequence = std::get<0>(GetParam());

  grammar::RePairEncoder<false> encoder;

  encoder.Encode(sequence.begin(), sequence.end(), report_non_terminals, report_cseq);
  EXPECT_EQ(sigma, *std::max_element(sequence.begin(), sequence.end()));
  EXPECT_EQ(non_terminals, std::get<1>(GetParam()));
  EXPECT_EQ(compact_seq, std::get<2>(GetParam()));

  non_terminals.clear();
  compact_seq.clear();
  encoder.Encode(sequence.begin(), sequence.end(), report_non_terminals, report_cseq);
  EXPECT_EQ(sigma, *std::max_element(sequence.begin(), sequence.end()));
  EXPECT_EQ(non_terminals, std::get<1>(GetParam()));
  EXPECT_EQ(compact_seq, std::get<2>(GetParam()));
}


INSTANTIATE_TEST_CASE_P(
    RePairEncoder,
    RePairEncoderTF,
    ::testing::Values(
        std::make_tuple(std::vector<int>({4, 3, 2, 1, 3, 2, 1, 3, 3, 3, 2, 1, 2, 2, 1, 1}),
                        std::vector<NonTerminal>({{2, 1, 2}, {3, 5, 3}}),
                        std::vector<int>{4, 6, 6, 3, 3, 6, 2, 5, 1}),
        std::make_tuple(std::vector<int>({1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8}),
                        std::vector<NonTerminal>({{1, 2, 2}, {3, 4, 2}, {6, 7, 2}, {10, 5, 3}, {11, 8, 3}, {9, 12, 5},
                                                  {14, 13, 8}}),
                        std::vector<int>{15, 15}),
        std::make_tuple(std::vector<int>({1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 9, 10, 11, 12, 13, 14}),
                        std::vector<NonTerminal>({{1, 2, 2}}),
                        std::vector<int>{15, 3, 4, 5, 6, 7, 8, 15, 9, 10, 11, 12, 13, 14})/*,
        std::make_tuple(std::vector<int>({1, 2, 3, 1, 2, 2, 1, 1, 1, 1, 3, 1, 2, 3}),
                        std::vector<NonTerminal>({{1, 1, 2}, {1, 2, 2}, {5, 3, 3}, {5, 2, 3}, {4, 4, 4}, {6, 7, 6},
                                                  {9, 8, 10}}),
                        std::vector<int>{15, 15})*/
    )
);


class RePairEncoderInChomskyNFTF : public ::testing::TestWithParam<std::tuple<std::vector<int>,
                                                                              std::vector<NonTerminal>>> {
};


TEST_P(RePairEncoderInChomskyNFTF, encode) {
  int sigma;
  std::vector<NonTerminal> non_terminals;
  NonTerminalWrapper report_non_terminals(sigma, non_terminals);

  const auto &sequence = std::get<0>(GetParam());

  grammar::RePairEncoder<true> encoder;
  encoder.Encode(sequence.begin(), sequence.end(), report_non_terminals);
  EXPECT_EQ(sigma, *std::max_element(sequence.begin(), sequence.end()));
  EXPECT_EQ(non_terminals, std::get<1>(GetParam()));

  non_terminals.clear();
  encoder.Encode(sequence.begin(), sequence.end(), report_non_terminals);
  EXPECT_EQ(sigma, *std::max_element(sequence.begin(), sequence.end()));
  EXPECT_EQ(non_terminals, std::get<1>(GetParam()));
}


TEST_P(RePairEncoderInChomskyNFTF, encode_slp) {
  grammar::SLP<> slp(0);
  auto report_rules = grammar::BuildSLPWrapper(slp);

  const auto &sequence = std::get<0>(GetParam());

  grammar::RePairEncoder<true> encoder;
  encoder.Encode(sequence.begin(), sequence.end(), report_rules);
  EXPECT_EQ(slp.Sigma(), *std::max_element(sequence.begin(), sequence.end()));

  auto &rules = std::get<1>(GetParam());
  for (int i = 0; i < rules.size(); ++i) {
    EXPECT_EQ(slp[slp.Sigma() + i + 1].first, rules[i].left);
    EXPECT_EQ(slp[slp.Sigma() + i + 1].second, rules[i].right);
    EXPECT_EQ(slp.SpanLength(slp.Sigma() + i + 1), rules[i].length);
  }
}


INSTANTIATE_TEST_CASE_P(
    RePairEncoder,
    RePairEncoderInChomskyNFTF,
    ::testing::Values(
        std::make_tuple(std::vector<int>({1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8}),
                        std::vector<NonTerminal>({{1, 2, 2}, {3, 4, 2}, {6, 7, 2}, {10, 5, 3}, {11, 8, 3}, {9, 12, 5},
                                                  {14, 13, 8}, {15, 15, 16}})),
        std::make_tuple(std::vector<int>({4, 3, 2, 1, 3, 2, 1, 3, 3, 3, 2, 1, 2, 2, 1, 1}),
                        std::vector<NonTerminal>({{2, 1, 2}, {3, 5, 3}, {3, 3, 2}, {2, 5, 3}, {4, 6, 4}, {8, 1, 4},
                                                  {6, 7, 5}, {11, 6, 8}, {9, 12, 12}, {13, 10, 16}})),
        std::make_tuple(std::vector<int>({1, 2, 3, 4, 5, 6, 7, 8}),
                        std::vector<NonTerminal>({{1, 2, 2}, {3, 4, 2}, {5, 6, 2}, {7, 8, 2}, {9, 10, 4}, {11, 12, 4},
                                                  {13, 14, 8}})),
        std::make_tuple(std::vector<int>({1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 9, 10, 11, 12, 13, 14}),
                        std::vector<NonTerminal>({{1, 2, 2}, {3, 4, 2}, {5, 6, 2}, {7, 8, 2}, {9, 10, 2}, {11, 12, 2},
                                                  {13, 14, 2}, {15, 16, 4}, {17, 18, 4}, {15, 19, 4}, {20, 21, 4},
                                                  {22, 23, 8}, {24, 25, 8}, {26, 27, 16}})),
        std::make_tuple(std::vector<int>({8, 3, 6, 8, 3}),
                        std::vector<NonTerminal>({{8, 3, 2}, {9, 6, 3}, {10, 9, 5}})),
        std::make_tuple(std::vector<int>({1, 2, 3, 1, 2, 2, 1, 1, 1, 1}),
                        std::vector<NonTerminal>({{1, 1, 2}, {1, 2, 2}, {5, 3, 3}, {5, 2, 3}, {4, 4, 4}, {6, 7, 6},
                                                  {9, 8, 10}}))/*,
        std::make_tuple(std::vector<int>({1, 2, 3, 1, 2, 2, 1, 1, 1, 1, 3, 1, 2, 3}),
                        std::vector<NonTerminal>({{1, 1, 2}, {1, 2, 2}, {5, 3, 3}, {5, 2, 3}, {4, 4, 4}, {6, 7, 6},
                                                  {9, 8, 10}}))*/
    )
);


template<typename T>
class RepairEncoderTypeTF : public ::testing::Test {};


typedef ::testing::Types<std::vector<int>, std::vector<char>> MyTypes;
TYPED_TEST_CASE(RepairEncoderTypeTF, MyTypes);


TYPED_TEST(RepairEncoderTypeTF, encode) {
  TypeParam data = {1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8};

  int sigma;
  std::vector<NonTerminal> non_terminals;
  NonTerminalWrapper report_non_terminals(sigma, non_terminals);
  std::vector<int> compact_seq;
  CompactSequenceWrapper report_cseq(compact_seq);

  grammar::RePairEncoder<false> encoder;
  encoder.Encode(data.begin(), data.end(), report_non_terminals, report_cseq);
  EXPECT_EQ(sigma, 8);
  EXPECT_EQ(non_terminals, std::vector<NonTerminal>({{1, 2, 2}, {3, 4, 2}, {6, 7, 2}, {10, 5, 3},
                                                     {11, 8, 3}, {9, 12, 5}, {14, 13, 8}}));
  EXPECT_EQ(compact_seq, std::vector<int>({15, 15}));
}


TYPED_TEST(RepairEncoderTypeTF, encodeInCNF) {
  TypeParam data = {1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8};

  int sigma;
  std::vector<NonTerminal> non_terminals;
  NonTerminalWrapper report_non_terminals(sigma, non_terminals);

  grammar::RePairEncoder<true> encoder;
  encoder.Encode(data.begin(), data.end(), report_non_terminals);
  EXPECT_EQ(sigma, 8);
  EXPECT_EQ(non_terminals,
            std::vector<NonTerminal>({{1, 2, 2}, {3, 4, 2}, {6, 7, 2}, {10, 5, 3}, {11, 8, 3}, {9, 12, 5}, {14, 13, 8},
                                      {15, 15, 16}}));
}


class RePairReaderTF : public ::testing::TestWithParam<std::tuple<std::string,
                                                                  int,
                                                                  std::vector<NonTerminal>,
                                                                  std::vector<int>>> {
};


TEST_P(RePairReaderTF, read) {
  int sigma;
  std::vector<NonTerminal> non_terminals;
  NonTerminalWrapper report_non_terminals(sigma, non_terminals);
  std::vector<int> compact_seq;
  CompactSequenceWrapper report_cseq(compact_seq);

  grammar::RePairReader<false> reader;

  auto datafile = FLAGS_datapath + "/" + std::get<0>(GetParam());

  ASSERT_NO_THROW(reader.Read(datafile, report_non_terminals, report_cseq));
  EXPECT_EQ(sigma, std::get<1>(GetParam()));
  EXPECT_EQ(non_terminals, std::get<2>(GetParam()));
  EXPECT_EQ(compact_seq, std::get<3>(GetParam()));
}


INSTANTIATE_TEST_CASE_P(
    RePairReader,
    RePairReaderTF,
    ::testing::Values(
        std::make_tuple(std::string("4_3_2_1_3_2_1_3_3_3_2_1_2_2_1_1.int.bin"),
                        4,
                        std::vector<NonTerminal>({{2, 1, 2}, {3, 5, 3}}),
                        std::vector<int>{4, 6, 6, 3, 3, 6, 2, 5, 1}),
        std::make_tuple(std::string("1_2_3_4_5_6_7_8_1_2_3_4_5_6_7_8.int.bin"),
                        8,
                        std::vector<NonTerminal>({{1, 2, 2}, {3, 4, 2}, {6, 7, 2}, {10, 5, 3}, {11, 8, 3}, {9, 12, 5},
                                                  {14, 13, 8}}),
                        std::vector<int>{15, 15}),
        std::make_tuple(std::string("1_2_3_4_5_6_7_8_1_2_9_10_11_12_13_14.int.bin"),
                        14,
                        std::vector<NonTerminal>({{1, 2, 2}}),
                        std::vector<int>{15, 3, 4, 5, 6, 7, 8, 15, 9, 10, 11, 12, 13, 14})
    )
);


class RePairReaderInChomskyNFTF : public ::testing::TestWithParam<std::tuple<std::string,
                                                                             int,
                                                                             std::vector<NonTerminal>>> {
};


TEST_P(RePairReaderInChomskyNFTF, read) {
  int sigma;
  std::vector<NonTerminal> non_terminals;
  NonTerminalWrapper report_non_terminals(sigma, non_terminals);
  std::vector<int> compact_seq;

  grammar::RePairReader<true> reader;

  auto datafile = FLAGS_datapath + "/" + std::get<0>(GetParam());

  ASSERT_NO_THROW(reader.Read(datafile, report_non_terminals));
  EXPECT_EQ(sigma, std::get<1>(GetParam()));
  EXPECT_EQ(non_terminals, std::get<2>(GetParam()));
}


INSTANTIATE_TEST_CASE_P(
    RePairReader,
    RePairReaderInChomskyNFTF,
    ::testing::Values(
        std::make_tuple(std::string("4_3_2_1_3_2_1_3_3_3_2_1_2_2_1_1.int.bin"),
                        4,
                        std::vector<NonTerminal>({{2, 1, 2}, {3, 5, 3}, {3, 3, 2}, {2, 5, 3}, {4, 6, 4}, {8, 1, 4},
                                                  {6, 7, 5}, {11, 6, 8}, {9, 12, 12}, {13, 10, 16}})),
        std::make_tuple(std::string("1_2_3_4_5_6_7_8_1_2_3_4_5_6_7_8.int.bin"),
                        8,
                        std::vector<NonTerminal>({{1, 2, 2}, {3, 4, 2}, {6, 7, 2}, {10, 5, 3}, {11, 8, 3}, {9, 12, 5},
                                                  {14, 13, 8}, {15, 15, 16}})),
        std::make_tuple(std::string("1_2_3_4_5_6_7_8_1_2_9_10_11_12_13_14.int.bin"),
                        14,
                        std::vector<NonTerminal>({{1, 2, 2}, {3, 4, 2}, {5, 6, 2}, {7, 8, 2}, {9, 10, 2}, {11, 12, 2},
                                                  {13, 14, 2}, {15, 16, 4}, {17, 18, 4}, {15, 19, 4}, {20, 21, 4},
                                                  {22, 23, 8}, {24, 25, 8}, {26, 27, 16}}))
    )
);


//TEST(RePair, repair) {
//  std::string fname = "/home/dcobas/Workspace/PhD/Research/Document_Retrieval/Codes/repair/bal/docs.out";
//
//  FILE *Tf, *Rf, *Cf;
//
//  struct stat s;
//  if (stat(fname.c_str(), &s) != 0) {
//    fprintf(stderr, "Error: cannot stat file %s\n", fname.c_str());
//    exit(1);
//  }
//
//  int i, len;
//  len = s.st_size / sizeof(int);
//  Tf = fopen(fname.c_str(), "r");
//  if (Tf == NULL) {
//    fprintf(stderr, "Error: cannot open file %s for reading\n", fname.c_str());
//    exit(1);
//  }
//
//  int *C = (int *) malloc(len * sizeof(int));
//  if (fread(C, sizeof(int), len, Tf) != len) {
//    fprintf(stderr, "Error: cannot read file %s\n", fname.c_str());
//    exit(1);
//  }
//  fclose(Tf);
//
//  grammar::RePairBasicEncoder encoder;
//  encoder.prepare(C, len);
//  encoder.repair(C, len, [](auto, auto, auto){});
//}


int main(int argc, char **argv) {
  gflags::AllowCommandLineReparsing();
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}