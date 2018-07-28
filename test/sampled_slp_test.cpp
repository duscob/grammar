//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/26/18.
//

#include <gtest/gtest.h>

#include "grammar/sampled_slp.h"
#include "grammar/slp.h"
#include "grammar/slp_metadata.h"


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

  // Check leaves
  const auto &e_leaves = std::get<3>(GetParam());;
  EXPECT_EQ(leaves, e_leaves);

  // Check that leaves cover full sequence
  std::size_t size = 0;
  for (const auto &item : leaves) {
    size += slp_.SpanLength(item);
  }
  EXPECT_EQ(size, slp_.SpanLength(slp_.Start()));
}


TEST_P(SampledSLPLeaves_TF, ComputeSampledSLPLeavesAndItsPTS) {
  Nodes leaves;
  auto &block_size = std::get<2>(GetParam());

  grammar::SampledPTS<> spts;
  grammar::AddSet<grammar::SampledPTS<>> add_set(spts);
  grammar::ComputeSampledSLPLeaves(slp_, block_size, back_inserter(leaves), add_set);

  // Check leaves
  const auto &e_leaves = std::get<3>(GetParam());;
  EXPECT_EQ(leaves, e_leaves);

  // Check that leaves cover full sequence
  std::size_t size = 0;
  for (const auto &item : leaves) {
    size += slp_.SpanLength(item);
  }
  EXPECT_EQ(size, slp_.SpanLength(slp_.Start()));

  // Check that we get a set for each leaf
  EXPECT_EQ(leaves.size(), spts.size());
  for (int i = 0; i < leaves.size(); ++i) {
    auto e_set = this->slp_.Span(leaves[i]);
    sort(e_set.begin(), e_set.end());
    e_set.erase(unique(e_set.begin(), e_set.end()), e_set.end());

    // check the set for each leaf
    auto set = spts[i + 1];
    EXPECT_EQ(set.second - set.first, e_set.size());
    EXPECT_TRUE(std::equal(set.first, set.second, e_set.begin()));
  }
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


class SampledSLPNodes_TF : public ::testing::TestWithParam<std::tuple<std::size_t, Rules, uint, float, Nodes>> {
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


TEST_P(SampledSLPNodes_TF, ComputeSampledSLPNodes) {
  Nodes nodes;
  auto block_size = std::get<2>(GetParam());

  grammar::SampledPTS<> spts;
  grammar::AddSet<grammar::SampledPTS<>> add_set(spts);
  grammar::ComputeSampledSLPLeaves(slp_, block_size, back_inserter(nodes), add_set);

  auto storing_factor = std::get<3>(GetParam());
  grammar::MustBeSampled<grammar::SampledPTS<>> pred(spts, storing_factor);
  grammar::ComputeSampledSLP(slp_, block_size, nodes, pred, add_set);

  const auto &e_nodes = std::get<4>(GetParam());;
  EXPECT_EQ(nodes, e_nodes);
}


INSTANTIATE_TEST_CASE_P(
    SampledSLP,
    SampledSLPNodes_TF,
    ::testing::Values(
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            4u,
            1,
            Nodes{9, 6, 7, 6, 10, 11, 12, 13, 14}
        ),
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            4u,
            2,
            Nodes{9, 6, 7, 6, 10, 12, 14}
        ),
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            4u,
            3,
            Nodes{9, 6, 7, 6, 10, 14}
        ),
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            4u,
            2.5,
            Nodes{9, 6, 7, 6, 10, 13, 14}
        )
    )
);
