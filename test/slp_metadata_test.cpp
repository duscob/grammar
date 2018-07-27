//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/17/18.
//

#include <gtest/gtest.h>

#include <sdsl/vectors.hpp>

#include "grammar/slp.h"
#include "grammar/slp_metadata.h"


using RightHand = std::pair<std::size_t, std::size_t>;
using Rules = std::vector<RightHand>;


class SLPMD_TF : public ::testing::TestWithParam<std::tuple<std::size_t, Rules>> {
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


TEST_P(SLPMD_TF, PTSCompute) {
  grammar::PTS<> pts;
  pts.Compute(slp_);

  for (auto i = 1ul; i <= slp_.Variables(); ++i) {
    auto span = slp_.Span(i);
    sort(span.begin(), span.end());
    span.erase(unique(span.begin(), span.end()), span.end());

    const auto &result = pts[i];
    ASSERT_EQ(result.size(), span.size());
    EXPECT_TRUE(equal(result.begin(), result.end(), span.begin()));
  }
}


TEST_P(SLPMD_TF, PTSConstructor) {
  grammar::PTS<> pts(slp_);

  for (auto i = 1ul; i <= slp_.Variables(); ++i) {
    auto span = slp_.Span(i);
    sort(span.begin(), span.end());
    span.erase(unique(span.begin(), span.end()), span.end());

    const auto &result = pts[i];
    ASSERT_EQ(result.size(), span.size());
    EXPECT_TRUE(equal(result.begin(), result.end(), span.begin()));
  }
}


INSTANTIATE_TEST_CASE_P(
    SLPMetadata,
    SLPMD_TF,
    ::testing::Values(
        std::make_tuple(4ul, Rules{{1, 2}, {3, 4}, {5, 6}, {7, 7}}),
        std::make_tuple(3ul, Rules{{1, 2}, {4, 3}, {5, 4}}),
        std::make_tuple(3ul, Rules{{1, 1}, {1, 2}, {5, 3}, {5, 2}, {4, 4}, {6, 7}, {9, 8}}),
        std::make_tuple(4ul, Rules{{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}})
    )
);


template<typename T>
class SLPMDGeneric_TF : public ::testing::Test {
 protected:
  grammar::SLP slp_{0};

// Sets up the test fixture.
  void SetUp() override {
    auto sigma = 4ul;
    Rules rules = {{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}};

    slp_ = grammar::SLP(sigma);

    for (auto &&rule : rules) {
      slp_.AddRule(rule.first, rule.second), rule.first;
    }
  }
};


using MyTypes = ::testing::Types<grammar::PTS<>,
                                 grammar::PTS<std::vector<uint64_t>>,
                                 grammar::PTS<sdsl::enc_vector<>>,
                                 grammar::PTS<sdsl::vlc_vector<>>,
                                 grammar::PTS<sdsl::dac_vector<>>>;
TYPED_TEST_CASE(SLPMDGeneric_TF, MyTypes);


TYPED_TEST(SLPMDGeneric_TF, PTSConstructor) {
  TypeParam pts(this->slp_);

  for (auto i = 1ul; i <= this->slp_.Variables(); ++i) {
    auto span = this->slp_.Span(i);
    sort(span.begin(), span.end());
    span.erase(unique(span.begin(), span.end()), span.end());

    const auto &result = pts[i];
    ASSERT_EQ(result.size(), span.size());
    EXPECT_TRUE(equal(result.begin(), result.end(), span.begin()));
  }
}


TEST(SampledPTSBuilder, AddSet) {
  grammar::SampledPTS<> ppts;

  std::vector<std::vector<uint32_t>> set = {{1, 2, 3}, {2}, {1, 2, 3}};

  for (int i = 0; i < set.size(); ++i) {
    EXPECT_EQ(ppts.AddSet(set[i]), i + 1);
    auto s = ppts[i + 1];
    EXPECT_TRUE(std::equal(s.first, s.second, set[i].begin()));
  }

  for (int i = 0; i < set.size(); ++i) {
    auto s = ppts[i + 1];
    EXPECT_TRUE(std::equal(s.first, s.second, set[i].begin()));
  }
}
