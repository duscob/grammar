//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/17/18.
//

#include <gtest/gtest.h>

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


TEST_P(SLPMD_TF, PTS) {
  grammar::PTS pts(slp_);

  for (auto i = 1ul; i <= slp_.Variables(); ++i) {
    auto span = slp_.Span(i);
//    std::cout << "+" << testing::PrintToString(span) << std::endl;
    sort(span.begin(), span.end());
    span.erase(unique(span.begin(), span.end()), span.end());
//    std::cout << "-" << testing::PrintToString(span) << std::endl;
//    std::cout << "*" << testing::PrintToString(pts[i]) << std::endl;

    EXPECT_EQ(pts[i], span);
  }
}


INSTANTIATE_TEST_CASE_P(SLPMetadata,
                        SLPMD_TF,
                        ::testing::Values(
                            std::make_tuple(4ul, Rules{{1, 2}, {3, 4}, {5, 6}, {7, 7}}),
                            std::make_tuple(3ul, Rules{{1, 2}, {4, 3}, {5, 4}}),
                            std::make_tuple(3ul, Rules{{1, 1}, {1, 2}, {5, 3}, {5, 2}, {4, 4}, {6, 7}, {9, 8}})
                        )
);