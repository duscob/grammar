//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 5/11/21.
//

#include <gtest/gtest.h>

#include <sdsl/bit_vectors.hpp>

#include "grammar/slp_compact.h"

class CreateCompactSLP_TF : public ::testing::TestWithParam<
    std::tuple<size_t, // Sigma
               std::vector<std::pair<size_t, size_t >>, // Rules
               std::vector<size_t>, // Roots
               std::vector<bool>, // Tree
               std::vector<size_t>, // Leaves
               std::map<size_t, size_t> // Non-terminal Ids
    >
> {
};

TEST_P(CreateCompactSLP_TF, CreateCompactGrammarForest) {
  const auto &sigma = std::get<0>(GetParam());
  const auto &rules = std::get<1>(GetParam());
  auto get_children = [&rules, sigma](const auto &tt_var) {
    return rules[tt_var - sigma - 1];
  };

  const auto &roots = std::get<2>(GetParam());

  auto[tree, leaves, nt_id] = grammar::CreateCompactGrammarForest(roots, sigma, get_children);

  const auto &e_tree = std::get<3>(GetParam());
  ASSERT_EQ(e_tree, tree);

  const auto &e_leaves = std::get<4>(GetParam());
  ASSERT_EQ(e_leaves, leaves);

  const auto &e_nt_id = std::get<5>(GetParam());
  std::map<size_t, size_t> r_nt_id;
  std::copy(nt_id.begin(), nt_id.end(), std::inserter(r_nt_id, r_nt_id.end()));
  ASSERT_EQ(e_nt_id, r_nt_id);
}

INSTANTIATE_TEST_SUITE_P(
    CompactSLP,
    CreateCompactSLP_TF,
    ::testing::Values(
        std::make_tuple(
            3,
            std::vector<std::pair<size_t, size_t>>{{1, 2}, {4, 3}, {5, 4}},
            std::vector<size_t>{6},
            std::vector<bool>{1, 1, 1, 0, 0, 0, 0},
            std::vector<size_t>{1, 2, 3, 6},
            std::map<size_t, size_t>{{4, 6}, {5, 5}, {6, 4}}),
        std::make_tuple(
            3,
            std::vector<std::pair<size_t, size_t>>{{1, 2}, {4, 3}, {5, 4}},
            std::vector<size_t>{5, 6},
            std::vector<bool>{1, 1, 0, 0, 0, 1, 0, 0},
            std::vector<size_t>{1, 2, 3, 4, 5},
            std::map<size_t, size_t>{{4, 5}, {5, 4}, {6, 9}}),
        std::make_tuple(
            3,
            std::vector<std::pair<size_t, size_t>>{{2, 3}, {4, 1}, {5, 3}},
            std::vector<size_t>{6},
            std::vector<bool>{1, 1, 1, 0, 0, 0, 0},
            std::vector<size_t>{2, 3, 1, 3},
            std::map<size_t, size_t>{{4, 6}, {5, 5}, {6, 4}}),
        std::make_tuple(
            3,
            std::vector<std::pair<size_t, size_t>>{{2, 3}, {4, 1}, {5, 3}},
            std::vector<size_t>{5},
            std::vector<bool>{1, 1, 0, 0, 0},
            std::vector<size_t>{2, 3, 1},
            std::map<size_t, size_t>{{4, 5}, {5, 4}})
    )
);

class CreateCompactSLPWithBP_TF : public CreateCompactSLP_TF {};

TEST_P(CreateCompactSLPWithBP_TF, CreateCompactGrammarTreeWithBP) {
  const auto &sigma = std::get<0>(GetParam());
  const auto &rules = std::get<1>(GetParam());
  auto get_children = [&rules, sigma](const auto &tt_var) {
    return rules[tt_var - sigma - 1];
  };

  const auto &roots = std::get<2>(GetParam());

  auto[tree, leaves, nt_id] = grammar::CreateCompactGrammarTreeWithBP(roots, sigma, get_children);

  const auto &e_tree = std::get<3>(GetParam());
  ASSERT_EQ(e_tree, tree);

  const auto &e_leaves = std::get<4>(GetParam());
  ASSERT_EQ(e_leaves, leaves);

  const auto &e_nt_id = std::get<5>(GetParam());
  std::map<size_t, size_t> r_nt_id;
  std::copy(nt_id.begin(), nt_id.end(), std::inserter(r_nt_id, r_nt_id.end()));
  ASSERT_EQ(e_nt_id, r_nt_id);
}

INSTANTIATE_TEST_SUITE_P(
    CompactSLP,
    CreateCompactSLPWithBP_TF,
    ::testing::Values(
        std::make_tuple(
            3,
            std::vector<std::pair<size_t, size_t>>{{1, 2}, {4, 3}, {5, 4}},
            std::vector<size_t>{6},
            std::vector<bool>{1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0},
            std::vector<size_t>{1, 2, 3, 7},
            std::map<size_t, size_t>{{4, 7}, {5, 6}, {6, 5}}),
        std::make_tuple(
            3,
            std::vector<std::pair<size_t, size_t>>{{1, 2}, {4, 3}, {5, 4}},
            std::vector<size_t>{5, 6},
            std::vector<bool>{1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0},
            std::vector<size_t>{1, 2, 3, 5, 6},
            std::map<size_t, size_t>{{4, 6}, {5, 5}, {6, 15}}),
        std::make_tuple(
            3,
            std::vector<std::pair<size_t, size_t>>{{2, 3}, {4, 1}, {5, 3}},
            std::vector<size_t>{6},
            std::vector<bool>{1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0},
            std::vector<size_t>{2, 3, 1, 3},
            std::map<size_t, size_t>{{4, 7}, {5, 6}, {6, 5}}),
        std::make_tuple(
            3,
            std::vector<std::pair<size_t, size_t>>{{2, 3}, {4, 1}, {5, 3}},
            std::vector<size_t>{5},
            std::vector<bool>{1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0},
            std::vector<size_t>{2, 3, 1},
            std::map<size_t, size_t>{{4, 6}, {5, 5}})
    )
);

class ExpandCompactSLP_TF : public ::testing::TestWithParam<
    std::tuple<size_t, // Sigma
               sdsl::bit_vector, // Tree
               std::vector<size_t>, // Leaves
               size_t, // Variable
               std::vector<size_t> // Expansion
    >
> {
};

TEST_P(ExpandCompactSLP_TF, ExpandCompactSLP) {
  const auto &sigma = std::get<0>(GetParam());
  const auto &tree = std::get<1>(GetParam());
  auto rank_leaf = sdsl::bit_vector::rank_0_type(&tree);
  const auto &leaves = std::get<2>(GetParam());
  const auto &var = std::get<3>(GetParam());

  std::vector<size_t> expansion;
  auto report = [&expansion](auto tt_term) {
    expansion.emplace_back(tt_term);
  };

  grammar::ExpandCompactSLP(var, sigma, tree, rank_leaf, leaves, report);

  const auto &e_expansion = std::get<4>(GetParam());
  ASSERT_EQ(e_expansion, expansion);
}

INSTANTIATE_TEST_SUITE_P(
    CompactSLP,
    ExpandCompactSLP_TF,
    ::testing::Values(
        std::make_tuple(
            3,
            sdsl::bit_vector{1, 1, 1, 0, 0, 0, 0},
            std::vector<size_t>{1, 2, 3, 6},
            4,
            std::vector<size_t>{1, 2, 3, 1, 2}),
        std::make_tuple(
            3,
            sdsl::bit_vector{1, 1, 1, 0, 0, 0, 0},
            std::vector<size_t>{1, 2, 3, 6},
            6,
            std::vector<size_t>{1, 2}),
        std::make_tuple(
            3,
            sdsl::bit_vector{1, 1, 0, 0, 0, 1, 0, 0},
            std::vector<size_t>{1, 2, 3, 4, 5},
            9,
            std::vector<size_t>{1, 2, 3, 1, 2}),
        std::make_tuple(
            3,
            sdsl::bit_vector{1, 1, 1, 0, 0, 0, 0},
            std::vector<size_t>{2, 3, 1, 3},
            4,
            std::vector<size_t>{2, 3, 1, 3}),
        std::make_tuple(
            3,
            sdsl::bit_vector{1, 1, 1, 0, 0, 0, 0},
            std::vector<size_t>{2, 3, 1, 3},
            3,
            std::vector<size_t>{3}),
        std::make_tuple(
            3,
            sdsl::bit_vector{1, 1, 1, 0, 0, 0, 0},
            std::vector<size_t>{2, 3, 1, 3},
            7,
            std::vector<size_t>{2})
    )
);

class ExpandPartialCompactSLP_TF : public ::testing::TestWithParam<
    std::tuple<size_t, // Sigma
               sdsl::bit_vector, // Tree
               std::vector<size_t>, // Leaves
               size_t, // Variable
               size_t, // Length
               std::vector<size_t> // Expansion
    >
> {
};

TEST_P(ExpandPartialCompactSLP_TF, ExpandCompactSLPFromFront) {
  const auto &sigma = std::get<0>(GetParam());
  const auto &tree = std::get<1>(GetParam());
  auto rank_leaf = sdsl::bit_vector::rank_0_type(&tree);
  const auto &leaves = std::get<2>(GetParam());
  const auto &var = std::get<3>(GetParam());
  const auto &len = std::get<4>(GetParam());

  std::vector<size_t> expansion;
  auto report = [&expansion](auto tt_term) {
    expansion.emplace_back(tt_term);
  };

  grammar::ExpandCompactSLPFromFront(var, len, sigma, tree, rank_leaf, leaves, report);

  const auto &e_expansion = std::get<5>(GetParam());
  ASSERT_EQ(e_expansion, expansion);
}

INSTANTIATE_TEST_SUITE_P(
    CompactSLP,
    ExpandPartialCompactSLP_TF,
    ::testing::Values(
        std::make_tuple(
            3,
            sdsl::bit_vector{1, 1, 1, 0, 0, 0, 0},
            std::vector<size_t>{1, 2, 3, 6},
            4,
            5,
            std::vector<size_t>{1, 2, 3, 1, 2}),
        std::make_tuple(
            3,
            sdsl::bit_vector{1, 1, 1, 0, 0, 0, 0},
            std::vector<size_t>{1, 2, 3, 6},
            4,
            10,
            std::vector<size_t>{1, 2, 3, 1, 2}),
        std::make_tuple(
            3,
            sdsl::bit_vector{1, 1, 1, 0, 0, 0, 0},
            std::vector<size_t>{1, 2, 3, 6},
            4,
            2,
            std::vector<size_t>{1, 2}),
        std::make_tuple(
            3,
            sdsl::bit_vector{1, 1, 1, 0, 0, 0, 0},
            std::vector<size_t>{1, 2, 3, 6},
            4,
            0,
            std::vector<size_t>{}),
        std::make_tuple(
            3,
            sdsl::bit_vector{1, 1, 1, 0, 0, 0, 0},
            std::vector<size_t>{2, 3, 1, 3},
            3,
            1,
            std::vector<size_t>{3}),
        std::make_tuple(
            3,
            sdsl::bit_vector{1, 1, 1, 0, 0, 0, 0},
            std::vector<size_t>{2, 3, 1, 3},
            7,
            5,
            std::vector<size_t>{2})
    )
);

class ExpandPartialCompactSLPFromBack_TF : public ExpandPartialCompactSLP_TF {};

TEST_P(ExpandPartialCompactSLPFromBack_TF, ExpandCompactSLPFromBack) {
  const auto &sigma = std::get<0>(GetParam());
  const auto &tree = std::get<1>(GetParam());
  auto rank_leaf = sdsl::bit_vector::rank_0_type(&tree);
  const auto &leaves = std::get<2>(GetParam());
  const auto &var = std::get<3>(GetParam());
  const auto &len = std::get<4>(GetParam());

  std::vector<size_t> expansion;
  auto report = [&expansion](auto tt_term) {
    expansion.emplace_back(tt_term);
  };

  grammar::ExpandCompactSLPFromBack(var, len, sigma, tree, rank_leaf, leaves, report);

  const auto &e_expansion = std::get<5>(GetParam());
  ASSERT_EQ(e_expansion, expansion);
}

INSTANTIATE_TEST_SUITE_P(
    CompactSLP,
    ExpandPartialCompactSLPFromBack_TF,
    ::testing::Values(
        std::make_tuple(
            3,
            sdsl::bit_vector{1, 1, 1, 0, 0, 0, 0},
            std::vector<size_t>{1, 2, 3, 6},
            4,
            5,
            std::vector<size_t>{2, 1, 3, 2, 1}),
        std::make_tuple(
            3,
            sdsl::bit_vector{1, 1, 1, 0, 0, 0, 0},
            std::vector<size_t>{1, 2, 3, 6},
            4,
            10,
            std::vector<size_t>{2, 1, 3, 2, 1}),
        std::make_tuple(
            3,
            sdsl::bit_vector{1, 1, 1, 0, 0, 0, 0},
            std::vector<size_t>{1, 2, 3, 6},
            4,
            2,
            std::vector<size_t>{2, 1}),
        std::make_tuple(
            3,
            sdsl::bit_vector{1, 1, 1, 0, 0, 0, 0},
            std::vector<size_t>{1, 2, 3, 6},
            4,
            0,
            std::vector<size_t>{}),
        std::make_tuple(
            3,
            sdsl::bit_vector{1, 1, 1, 0, 0, 0, 0},
            std::vector<size_t>{2, 3, 1, 3},
            3,
            1,
            std::vector<size_t>{3}),
        std::make_tuple(
            3,
            sdsl::bit_vector{1, 1, 1, 0, 0, 0, 0},
            std::vector<size_t>{2, 3, 1, 3},
            7,
            5,
            std::vector<size_t>{2})
    )
);