//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/26/18.
//

#include <gtest/gtest.h>

#include "grammar/sampled_slp.h"
#include "grammar/slp.h"


using RightHand = std::pair<std::size_t, std::size_t>;
using Rules = std::vector<RightHand>;
using Nodes = std::vector<std::size_t>;


class SampledSLPLeaves_TF : public ::testing::TestWithParam<std::tuple<std::size_t, Rules, std::size_t, Nodes>> {
 protected:
  grammar::SLP slp_{0};

// Sets up the test fixture.
  void SetUp() override {
    auto &sigma = std::get<0>(GetParam());
    auto &rules = std::get<1>(GetParam());

    slp_ = grammar::SLP(sigma);

    for (auto &&rule : rules) {
      slp_.AddRule(rule.first, rule.second), rule.first;
    }
  }
};


TEST_P(SampledSLPLeaves_TF, ComputeSampledSLPLeaf) {
  Nodes leaves;
  auto &block_size = std::get<2>(GetParam());

  grammar::ComputeSampledSLPLeaves(slp_, block_size, back_inserter(leaves));

  const auto &e_leaves = std::get<3>(GetParam());;
  EXPECT_EQ(leaves, e_leaves);

  std::size_t size = 0;
  for (const auto &item : leaves) {
    size += slp_.SpanLength(item);
  }
  EXPECT_EQ(size, slp_.SpanLength(slp_.Start()));
}


INSTANTIATE_TEST_CASE_P(
    SampledSLP,
    SampledSLPLeaves_TF,
    ::testing::Values(
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            4ul,
            Nodes{9, 6, 7, 6, 10}
        ),
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            3ul,
            Nodes{4, 6, 6, 7, 6, 8, 1}
        ),
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            5ul,
            Nodes{9, 11, 6, 10}
        ),
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            8ul,
            Nodes{9, 12, 10}
        ),
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            12ul,
            Nodes{13, 10}
        ),
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            13ul,
            Nodes{13, 10}
        )
    )
);
