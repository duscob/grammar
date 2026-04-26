//
// Smoke tests for grammar::CompactBPSLP.
//
// Builds a small CombinedSLP, then derives a CompactBPSLP from it and checks:
//   - Sigma / IsTerminal
//   - operator[] for every non-terminal in the original grammar (round-trips
//     to terminals / nested non-terminals correctly under the BP encoding)
//   - Map(leaf) / Cover(leaf) match the expected sampled-leaf variable ids
//     after the nt_id remap
//   - serialize/load round-trip preserves equality and behaviour
//

#include <sstream>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "grammar/compact_bp_slp.h"
#include "grammar/sampled_slp.h"
#include "grammar/slp_metadata.h"

namespace {

// Build a CombinedSLP<> from a sigma + rules pair, then Compute against a
// MustBeSampled predicate.
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

// Recursively walk the BP-encoded grammar starting from `var` and collect the
// terminal sequence in left-to-right order. Used to cross-check operator[]
// against the original CombinedSLP's expansion of the same variable.
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

class CompactBPSLP_TF : public ::testing::TestWithParam<
    std::tuple<std::size_t,                                          // Sigma
               std::vector<std::pair<std::size_t, std::size_t>>,     // Rules
               uint32_t,                                              // Block size
               float                                                  // Storing factor
              >> {};


// Compute against a CombinedSLP and check that every non-terminal expands to
// the same terminal sequence in CompactBPSLP as in the source CombinedSLP.
TEST_P(CompactBPSLP_TF, OperatorBracket_RoundTripsToOriginalExpansion) {
  const auto &sigma = std::get<0>(GetParam());
  const auto &rules = std::get<1>(GetParam());
  const auto &block_size = std::get<2>(GetParam());
  const auto &storing_factor = std::get<3>(GetParam());

  auto cslp = BuildCombinedSLP(sigma, rules, block_size, storing_factor);

  grammar::CompactBPSLP<> bp;
  bp.Compute(cslp);

  EXPECT_EQ(bp.Sigma(), sigma);
  for (std::size_t v = 1; v <= sigma; ++v) EXPECT_TRUE(bp.IsTerminal(v));
  EXPECT_FALSE(bp.IsTerminal(sigma + 1));

  // For every original non-terminal, expand via the CombinedSLP's rules
  // (the source of truth) and via CompactBPSLP. The BP class uses compact
  // ids internally, so we drive the comparison from the CombinedSLP side and
  // expand each leaf of the CombinedSLP via CompactBPSLP using Map.
  std::vector<std::size_t> expected;
  Expand(cslp, cslp.Start(), expected);

  // Reconstruct the same sequence from the CompactBPSLP by iterating its
  // sampled-tree leaves and expanding each Map'd compact variable.
  std::vector<std::size_t> got;
  std::size_t total_leaves = bp.SampledLeaves().size();
  for (std::size_t leaf = 1; leaf <= total_leaves; ++leaf) {
    Expand(bp, bp.Map(leaf), got);
  }
  EXPECT_EQ(got, expected);
}


// Cover(leaf) returns a length-1 array whose sole element is Map(leaf).
TEST_P(CompactBPSLP_TF, CoverIsUnit) {
  const auto &sigma = std::get<0>(GetParam());
  const auto &rules = std::get<1>(GetParam());
  const auto &block_size = std::get<2>(GetParam());
  const auto &storing_factor = std::get<3>(GetParam());

  auto cslp = BuildCombinedSLP(sigma, rules, block_size, storing_factor);
  grammar::CompactBPSLP<> bp;
  bp.Compute(cslp);

  std::size_t total_leaves = bp.SampledLeaves().size();
  for (std::size_t leaf = 1; leaf <= total_leaves; ++leaf) {
    auto cover = bp.Cover(leaf);
    ASSERT_EQ(cover.size(), 1u);
    EXPECT_EQ(cover[0], bp.Map(leaf));
  }
}


// serialize/load round-trip preserves equality and behaviour.
TEST_P(CompactBPSLP_TF, SerializeLoadRoundTrip) {
  const auto &sigma = std::get<0>(GetParam());
  const auto &rules = std::get<1>(GetParam());
  const auto &block_size = std::get<2>(GetParam());
  const auto &storing_factor = std::get<3>(GetParam());

  auto cslp = BuildCombinedSLP(sigma, rules, block_size, storing_factor);
  grammar::CompactBPSLP<> bp;
  bp.Compute(cslp);

  std::stringstream buf;
  bp.serialize(buf);

  grammar::CompactBPSLP<> loaded;
  EXPECT_NE(bp, loaded);

  loaded.load(buf);
  EXPECT_EQ(bp, loaded);

  // Behaviour parity: same expansion sequence.
  std::vector<std::size_t> got_a;
  std::vector<std::size_t> got_b;
  std::size_t total_leaves = bp.SampledLeaves().size();
  for (std::size_t leaf = 1; leaf <= total_leaves; ++leaf) {
    Expand(bp, bp.Map(leaf), got_a);
    Expand(loaded, loaded.Map(leaf), got_b);
  }
  EXPECT_EQ(got_a, got_b);
}


// Sampled-tree queries (inherited from SampledSLP) work after Compute.
TEST_P(CompactBPSLP_TF, InheritedSampledQueries) {
  const auto &sigma = std::get<0>(GetParam());
  const auto &rules = std::get<1>(GetParam());
  const auto &block_size = std::get<2>(GetParam());
  const auto &storing_factor = std::get<3>(GetParam());

  auto cslp = BuildCombinedSLP(sigma, rules, block_size, storing_factor);
  grammar::CompactBPSLP<> bp;
  bp.Compute(cslp);

  // Round-trip Leaf(Position(leaf)) for every leaf.
  std::size_t total_leaves = bp.SampledLeaves().size();
  for (std::size_t leaf = 1; leaf <= total_leaves; ++leaf) {
    EXPECT_EQ(bp.Leaf(bp.Position(leaf)), leaf);
  }
}


INSTANTIATE_TEST_SUITE_P(
    CompactBPSLP,
    CompactBPSLP_TF,
    ::testing::Values(
        // 4 terminals, 10 rules. Same dataset as SampledSLPParent_TF.
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
