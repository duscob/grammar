//
// Smoke tests for grammar::CompactLOUDSSLP.
//
// Mirrors compact_bp_slp_test.cpp's structure to make cross-class behaviour
// easy to compare, but specific to the LOUDS encoding.
//

#include <sstream>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "grammar/compact_louds_slp.h"
#include "grammar/sampled_slp.h"
#include "grammar/slp_metadata.h"

namespace {

grammar::CombinedSLP<> BuildCombinedSLP(
    std::size_t sigma,
    const std::vector<std::pair<std::size_t, std::size_t>> &rules,
    uint32_t block_size,
    float storing_factor) {
  grammar::CombinedSLP<> cslp;
  cslp.Reset(sigma);
  for (const auto &r : rules) cslp.AddRule(r.first, r.second);

  grammar::Chunks<> pts;
  grammar::AddSet<grammar::Chunks<>> add_set(pts);
  cslp.Compute(block_size,
               add_set,
               add_set,
               grammar::MustBeSampled<grammar::Chunks<>>(
                   grammar::AreChildrenTooBig<grammar::Chunks<>>(pts, storing_factor)));
  return cslp;
}

template<typename SLP>
void Expand(const SLP &slp, std::size_t var, std::vector<std::size_t> &out) {
  if (slp.IsTerminal(var)) {
    out.push_back(var);
    return;
  }
  auto[l, r] = slp[var];
  Expand(slp, l, out);
  Expand(slp, r, out);
}

}  // namespace

class CompactLOUDSSLP_TF : public ::testing::TestWithParam<
    std::tuple<std::size_t,
               std::vector<std::pair<std::size_t, std::size_t>>,
               uint32_t,
               float>> {};


TEST_P(CompactLOUDSSLP_TF, OperatorBracket_RoundTripsToOriginalExpansion) {
  const auto &sigma = std::get<0>(GetParam());
  const auto &rules = std::get<1>(GetParam());
  const auto &block_size = std::get<2>(GetParam());
  const auto &storing_factor = std::get<3>(GetParam());

  auto cslp = BuildCombinedSLP(sigma, rules, block_size, storing_factor);

  grammar::CompactLOUDSSLP<> louds;
  louds.Compute(cslp);

  EXPECT_EQ(louds.Sigma(), sigma);
  for (std::size_t v = 1; v <= sigma; ++v) EXPECT_TRUE(louds.IsTerminal(v));
  EXPECT_FALSE(louds.IsTerminal(sigma + 1));

  std::vector<std::size_t> expected;
  Expand(cslp, cslp.Start(), expected);

  std::vector<std::size_t> got;
  std::size_t total_leaves = louds.SampledLeaves().size();
  for (std::size_t leaf = 1; leaf <= total_leaves; ++leaf) {
    Expand(louds, louds.Map(leaf), got);
  }
  EXPECT_EQ(got, expected);
}


TEST_P(CompactLOUDSSLP_TF, CoverIsUnit) {
  const auto &sigma = std::get<0>(GetParam());
  const auto &rules = std::get<1>(GetParam());
  const auto &block_size = std::get<2>(GetParam());
  const auto &storing_factor = std::get<3>(GetParam());

  auto cslp = BuildCombinedSLP(sigma, rules, block_size, storing_factor);
  grammar::CompactLOUDSSLP<> louds;
  louds.Compute(cslp);

  std::size_t total_leaves = louds.SampledLeaves().size();
  for (std::size_t leaf = 1; leaf <= total_leaves; ++leaf) {
    auto cover = louds.Cover(leaf);
    ASSERT_EQ(cover.size(), 1u);
    EXPECT_EQ(cover[0], louds.Map(leaf));
  }
}


TEST_P(CompactLOUDSSLP_TF, SerializeLoadRoundTrip) {
  const auto &sigma = std::get<0>(GetParam());
  const auto &rules = std::get<1>(GetParam());
  const auto &block_size = std::get<2>(GetParam());
  const auto &storing_factor = std::get<3>(GetParam());

  auto cslp = BuildCombinedSLP(sigma, rules, block_size, storing_factor);
  grammar::CompactLOUDSSLP<> louds;
  louds.Compute(cslp);

  std::stringstream buf;
  louds.serialize(buf);

  grammar::CompactLOUDSSLP<> loaded;
  EXPECT_NE(louds, loaded);

  loaded.load(buf);
  EXPECT_EQ(louds, loaded);

  std::vector<std::size_t> got_a;
  std::vector<std::size_t> got_b;
  std::size_t total_leaves = louds.SampledLeaves().size();
  for (std::size_t leaf = 1; leaf <= total_leaves; ++leaf) {
    Expand(louds, louds.Map(leaf), got_a);
    Expand(loaded, loaded.Map(leaf), got_b);
  }
  EXPECT_EQ(got_a, got_b);
}


TEST_P(CompactLOUDSSLP_TF, InheritedSampledQueries) {
  const auto &sigma = std::get<0>(GetParam());
  const auto &rules = std::get<1>(GetParam());
  const auto &block_size = std::get<2>(GetParam());
  const auto &storing_factor = std::get<3>(GetParam());

  auto cslp = BuildCombinedSLP(sigma, rules, block_size, storing_factor);
  grammar::CompactLOUDSSLP<> louds;
  louds.Compute(cslp);

  std::size_t total_leaves = louds.SampledLeaves().size();
  for (std::size_t leaf = 1; leaf <= total_leaves; ++leaf) {
    EXPECT_EQ(louds.Leaf(louds.Position(leaf)), leaf);
  }
}


// LOUDS tree should be roughly half the size of the BP tree on the same
// grammar (LOUDS is ~2n bits, BP is ~4n bits). We don't pull in
// CompactBPSLP here to keep the test self-contained — instead we just
// verify the BvTree is non-empty and well-formed.
TEST_P(CompactLOUDSSLP_TF, BvTreeIsCompact) {
  const auto &sigma = std::get<0>(GetParam());
  const auto &rules = std::get<1>(GetParam());
  const auto &block_size = std::get<2>(GetParam());
  const auto &storing_factor = std::get<3>(GetParam());

  auto cslp = BuildCombinedSLP(sigma, rules, block_size, storing_factor);
  grammar::CompactLOUDSSLP<> louds;
  louds.Compute(cslp);

  // Tree must be non-empty.
  EXPECT_GT(louds.BvTree().size(), 0u);
  // CompactLeaves count must equal the number of `0` bits in the tree.
  std::size_t zeros = 0;
  for (std::size_t i = 0; i < louds.BvTree().size(); ++i) {
    if (louds.BvTree()[i] == 0) ++zeros;
  }
  EXPECT_EQ(zeros, louds.CompactLeaves().size());
}


INSTANTIATE_TEST_SUITE_P(
    CompactLOUDSSLP,
    CompactLOUDSSLP_TF,
    ::testing::Values(
        std::make_tuple(
            std::size_t{4},
            std::vector<std::pair<std::size_t, std::size_t>>{
                {2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6},
                {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            uint32_t{4}, 1.0f),
        std::make_tuple(
            std::size_t{4},
            std::vector<std::pair<std::size_t, std::size_t>>{
                {2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6},
                {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            uint32_t{4}, 2.0f),
        std::make_tuple(
            std::size_t{4},
            std::vector<std::pair<std::size_t, std::size_t>>{
                {2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6},
                {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}},
            uint32_t{4}, 3.0f)));
