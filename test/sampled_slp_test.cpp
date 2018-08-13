//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/26/18.
//

#include <gtest/gtest.h>

#include <sdsl/sd_vector.hpp>

#include "grammar/sampled_slp.h"
#include "grammar/slp.h"
#include "grammar/slp_metadata.h"


using RightHand = std::pair<std::size_t, std::size_t>;
using Rules = std::vector<RightHand>;
using Nodes = std::vector<std::size_t>;


class SampledSLPLeaves_TF : public ::testing::TestWithParam<std::tuple<std::size_t, Rules, std::size_t, Nodes>> {
 protected:
  grammar::SLP<> slp_{0};

// Sets up the test fixture.
  void SetUp() override {
    auto &sigma = std::get<0>(GetParam());
    auto &rules = std::get<1>(GetParam());

    slp_ = grammar::SLP<>(sigma);

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

  grammar::Chunks<> spts;
  grammar::AddSet<grammar::Chunks<>> add_set(spts);
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
  grammar::SLP<> slp_{0};

// Sets up the test fixture.
  void SetUp() override {
    auto &sigma = std::get<0>(GetParam());
    auto &rules = std::get<1>(GetParam());

    slp_ = grammar::SLP<>(sigma);

    for (auto &&rule : rules) {
      slp_.AddRule(rule.first, rule.second), rule.first;
    }
  }
};


TEST_P(SampledSLPNodes_TF, ComputeSampledSLPNodes) {
  Nodes nodes;
  auto block_size = std::get<2>(GetParam());

  grammar::Chunks<> spts;
  grammar::AddSet<grammar::Chunks<>> add_set(spts);
  grammar::ComputeSampledSLPLeaves(slp_, block_size, back_inserter(nodes), add_set);

  auto storing_factor = std::get<3>(GetParam());
  grammar::MustBeSampled<grammar::Chunks<>> pred(spts, storing_factor);
  grammar::ComputeSampledSLPNodes(slp_, block_size, nodes, pred, add_set);

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

using Pair = std::pair<std::size_t, std::size_t>;


class SampledSLPMap_TF : public ::testing::TestWithParam<std::tuple<std::size_t, Rules, std::vector<Pair>>> {
 protected:
  grammar::SLP<> slp_{0};

// Sets up the test fixture.
  void SetUp() override {
    auto &sigma = std::get<0>(GetParam());
    auto &rules = std::get<1>(GetParam());

    slp_ = grammar::SLP<>(sigma);

    for (auto &&rule : rules) {
      slp_.AddRule(rule.first, rule.second), rule.first;
    }
  }
};


TEST_P(SampledSLPMap_TF, LeafAndPos) {
  grammar::Chunks<> pts;
  grammar::AddSet<grammar::Chunks<>> add_set(pts);
  grammar::SampledSLP<> sslp(slp_, 4, add_set, add_set, grammar::MustBeSampled<grammar::Chunks<>>(pts, 2.5));

  const auto &pos = std::get<2>(GetParam());

  for (int i = 0; i < pos.size(); ++i) {
    EXPECT_EQ(sslp.Leaf(pos[i].first), pos[i].second) << i;
  }

  std::set<std::size_t> leaves;
  for (int i = 0; i < pos.size(); ++i) {
    if (leaves.count(pos[i].second) == 0) {
      EXPECT_EQ(sslp.Position(pos[i].second), pos[i].first) << i;
      leaves.insert(pos[i].second);
    }
  }
}


TEST_P(SampledSLPMap_TF, CombinedSLPLeafAndPos) {
  grammar::CombinedSLP<> cslp;
  {
    auto &sigma = std::get<0>(GetParam());
    auto &rules = std::get<1>(GetParam());

    cslp.Reset(sigma);

    for (auto &&rule : rules) {
      cslp.AddRule(rule.first, rule.second), rule.first;
    }
  }

  grammar::Chunks<> pts;
  grammar::AddSet<grammar::Chunks<>> add_set(pts);
  cslp.Compute(4, add_set, add_set, grammar::MustBeSampled<grammar::Chunks<>>(pts, 2.5));

  const auto &pos = std::get<2>(GetParam());

  for (int i = 0; i < pos.size(); ++i) {
    EXPECT_EQ(cslp.Leaf(pos[i].first), pos[i].second) << i;
  }

  std::set<std::size_t> leaves;
  for (int i = 0; i < pos.size(); ++i) {
    if (leaves.count(pos[i].second) == 0) {
      EXPECT_EQ(cslp.Position(pos[i].second), pos[i].first) << i;
      leaves.insert(pos[i].second);
    }
  }
}


INSTANTIATE_TEST_CASE_P(
    SampledSLP,
    SampledSLPMap_TF,
    ::testing::Values(
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            std::vector<Pair>{{0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 2}, {5, 2}, {6, 2}, {7, 3}, {8, 3}, {9, 4}, {10, 4},
                              {11, 4}, {12, 5}, {13, 5}, {14, 5}, {15, 5}}
        )
    )
);


class SampledSLPParent_TF : public ::testing::TestWithParam<std::tuple<std::size_t,
                                                                       Rules,
                                                                       uint,
                                                                       float,
                                                                       std::vector<std::size_t>,
                                                                       std::vector<Pair>>> {
 protected:
  grammar::SLP<> slp_{0};

// Sets up the test fixture.
  void SetUp() override {
    auto &sigma = std::get<0>(GetParam());
    auto &rules = std::get<1>(GetParam());

    slp_ = grammar::SLP<>(sigma);

    for (auto &&rule : rules) {
      slp_.AddRule(rule.first, rule.second), rule.first;
    }
  }
};


TEST_P(SampledSLPParent_TF, FirstChildAndParent) {
  auto block_size = std::get<2>(GetParam());
  auto storing_factor = std::get<3>(GetParam());

  grammar::Chunks<> pts;
  grammar::AddSet<grammar::Chunks<>> add_set(pts);
  grammar::SampledSLP<>
      sslp(slp_, block_size, add_set, add_set, grammar::MustBeSampled<grammar::Chunks<>>(pts, storing_factor));

  const auto &pos = std::get<4>(GetParam());
  const auto &e_res = std::get<5>(GetParam());

  for (int i = 0; i < pos.size(); ++i) {
    EXPECT_TRUE(sslp.IsFirstChild(pos[i]));
    EXPECT_EQ(sslp.Parent(pos[i]), e_res[i]);
  }
}


TEST_P(SampledSLPParent_TF, CombinedSLPFirstChildAndParent) {
  grammar::CombinedSLP<> cslp;
  {
    auto &sigma = std::get<0>(GetParam());
    auto &rules = std::get<1>(GetParam());

    cslp.Reset(sigma);

    for (auto &&rule : rules) {
      cslp.AddRule(rule.first, rule.second), rule.first;
    }
  }

  auto block_size = std::get<2>(GetParam());
  auto storing_factor = std::get<3>(GetParam());

  {
    grammar::Chunks<> pts;
    grammar::AddSet<grammar::Chunks<>> add_set(pts);
    cslp.Compute(block_size, add_set, add_set, grammar::MustBeSampled<grammar::Chunks<>>(pts, storing_factor));
  }

  const auto &pos = std::get<4>(GetParam());
  const auto &e_res = std::get<5>(GetParam());

  for (int i = 0; i < pos.size(); ++i) {
    EXPECT_TRUE(cslp.IsFirstChild(pos[i]));
    EXPECT_EQ(cslp.Parent(pos[i]), e_res[i]);
  }
}


TEST_P(SampledSLPParent_TF, Serialization) {
  auto block_size = std::get<2>(GetParam());
  auto storing_factor = std::get<3>(GetParam());

  grammar::Chunks<> pts;
  grammar::AddSet<grammar::Chunks<>> add_set(pts);
  grammar::SampledSLP<>
      sslp(slp_, block_size, add_set, add_set, grammar::MustBeSampled<grammar::Chunks<>>(pts, storing_factor));
  {
    std::ofstream out("tmp.sampled_slp", std::ios::binary);
    sslp.serialize(out);
  }

  grammar::SampledSLP<> sslp_loaded;
  EXPECT_FALSE(sslp == sslp_loaded);

  {
    std::ifstream in("tmp.sampled_slp", std::ios::binary);
    sslp_loaded.load(in);
  }
  EXPECT_TRUE(sslp == sslp_loaded);
}


INSTANTIATE_TEST_CASE_P(
    SampledSLP,
    SampledSLPParent_TF,
    ::testing::Values(
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            4,
            1,
            std::vector<std::size_t>{1, 2, 6, 8},
            std::vector<Pair>{{8, 5}, {6, 4}, {7, 5}, {9, 6}}
        ),
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            4,
            2,
            std::vector<std::size_t>{1, 2},
            std::vector<Pair>{{7, 6}, {6, 5}}
        ),
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            4,
            2.5,
            std::vector<std::size_t>{1, 6},
            std::vector<Pair>{{6, 5}, {7, 6}}
        ),
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            4,
            3,
            std::vector<std::size_t>{1},
            std::vector<Pair>{{6, 6}}
        )
    )
);

using Span = std::pair<std::size_t, std::size_t>;
using SpanCover = std::pair<std::vector<std::size_t>, Span>;


class SLPSpanCoverFromBottom_TF : public ::testing::TestWithParam<std::tuple<std::size_t,
                                                                             Rules,
                                                                             uint,
                                                                             float,
                                                                             Span,
                                                                             SpanCover>> {
 protected:
  grammar::SLPWithMetadata<grammar::PTS<>> slp_{0};

// Sets up the test fixture.
  void SetUp() override {
    auto &sigma = std::get<0>(GetParam());
    auto &rules = std::get<1>(GetParam());

    slp_.Reset(sigma);

    for (auto &&rule : rules) {
      slp_.AddRule(rule.first, rule.second), rule.first;
    }

    slp_.ComputeMetadata();
  }
};


TEST_P(SLPSpanCoverFromBottom_TF, SpanCover) {
  auto block_size = std::get<2>(GetParam());
  auto storing_factor = std::get<3>(GetParam());

  grammar::Chunks<> pts;
  grammar::AddSet<grammar::Chunks<>> add_set(pts);
  grammar::SampledSLP<>
      sslp(slp_, block_size, add_set, add_set, grammar::MustBeSampled<grammar::Chunks<>>(pts, storing_factor));

  auto &span = std::get<4>(GetParam());

  SpanCover result;
  result.second = grammar::ComputeSpanCoverFromBottom(sslp, span.first, span.second, back_inserter(result.first));

  auto &eresult = std::get<5>(GetParam());
  EXPECT_EQ(result, eresult);
}


INSTANTIATE_TEST_CASE_P(
    SLP,
    SLPSpanCoverFromBottom_TF,
    ::testing::Values(
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            4,
            2,
            Span{0, 16},
            SpanCover{{7}, {0, 16}}
        ),
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            4,
            2,
            Span{1, 16},
            SpanCover{{6, 5}, {4, 16}}
        ),
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            4,
            2,
            Span{0, 15},
            SpanCover{{1, 6}, {0, 12}}
        ),
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            4,
            2,
            Span{4, 12},
            SpanCover{{6}, {4, 12}}
        ),
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            4,
            2,
            Span{1, 15},
            SpanCover{{6}, {4, 12}}
        ),
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            4,
            2,
            Span{0, 10},
            SpanCover{{1, 2, 3}, {0, 9}}
        ),
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            4,
            2,
            Span{2, 11},
            SpanCover{{2, 3}, {4, 9}}
        ),
        std::make_tuple(
            4ul,
            Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            4,
            2,
            Span{1, 3},
            SpanCover{{}, {4, 0}}
        )
    )
);