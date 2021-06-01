//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 5/24/21.
//


#include <gtest/gtest.h>

#include "grammar/slp_helper.h"

class ExpandPartialCompactSLP_TF : public ::testing::TestWithParam<
    std::tuple<size_t, // Sigma
               std::vector<size_t>, // Rules
               size_t, // Variable
               size_t, // Length
               std::vector<size_t> // Expansion
    >
> {
};

TEST_P(ExpandPartialCompactSLP_TF, ExpandSLPForwardWithObject) {
  const auto &[sigma, rules, var, len, e_expansion] = GetParam();

  std::vector<size_t> expansion;
  auto report = [&expansion](auto tt_term) {
    expansion.emplace_back(tt_term);
  };

  grammar::ExpandSLPForward(rules, sigma, var, len, report);

  ASSERT_EQ(e_expansion, expansion);
}

INSTANTIATE_TEST_SUITE_P(
    SLP,
    ExpandPartialCompactSLP_TF,
    ::testing::Values(
        std::make_tuple(
            3,
            std::vector<size_t>{1, 2, 4, 3, 5, 4},
            6,
            5,
            std::vector<size_t>{1, 2, 3, 1, 2}),
        std::make_tuple(
            3,
            std::vector<size_t>{1, 2, 4, 3, 5, 4},
            6,
            10,
            std::vector<size_t>{1, 2, 3, 1, 2}),
        std::make_tuple(
            3,
            std::vector<size_t>{1, 2, 4, 3, 5, 4},
            6,
            2,
            std::vector<size_t>{1, 2}),
        std::make_tuple(
            3,
            std::vector<size_t>{1, 2, 4, 3, 5, 4},
            6,
            0,
            std::vector<size_t>{}),
        std::make_tuple(
            3,
            std::vector<size_t>{2, 3, 4, 1, 5, 3},
            3,
            1,
            std::vector<size_t>{3})
    )
);

class ExpandSLPBackward_TF : public ExpandPartialCompactSLP_TF {};

TEST_P(ExpandSLPBackward_TF, ExpandSLPBackwardWithObject) {
  const auto &[sigma, rules, var, len, e_expansion] = GetParam();

  std::vector<size_t> expansion;
  auto report = [&expansion](auto tt_term) {
    expansion.emplace_back(tt_term);
  };

  grammar::ExpandSLPBackward(rules, sigma, var, len, report);

  ASSERT_EQ(e_expansion, expansion);
}

INSTANTIATE_TEST_SUITE_P(
    SLP,
    ExpandSLPBackward_TF,
    ::testing::Values(
        std::make_tuple(
            3,
            std::vector<size_t>{1, 2, 4, 3, 5, 4},
            6,
            5,
            std::vector<size_t>{2, 1, 3, 2, 1}),
        std::make_tuple(
            3,
            std::vector<size_t>{1, 2, 4, 3, 5, 4},
            6,
            10,
            std::vector<size_t>{2, 1, 3, 2, 1}),
        std::make_tuple(
            3,
            std::vector<size_t>{1, 2, 4, 3, 5, 4},
            6,
            2,
            std::vector<size_t>{2, 1}),
        std::make_tuple(
            3,
            std::vector<size_t>{1, 2, 4, 3, 5, 4},
            6,
            0,
            std::vector<size_t>{}),
        std::make_tuple(
            3,
            std::vector<size_t>{2, 3, 4, 1, 5, 3},
            3,
            1,
            std::vector<size_t>{3})
    )
);