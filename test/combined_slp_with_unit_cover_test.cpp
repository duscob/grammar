//
// Smoke tests for grammar::CombinedSLPWithUnitCover.
//

#include <cstdio>
#include <fstream>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "grammar/combined_slp_with_unit_cover.h"
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

}  // namespace

class CombinedSLPWithUnitCover_TF : public ::testing::TestWithParam<
    std::tuple<std::size_t,
               std::vector<std::pair<std::size_t, std::size_t>>,
               uint32_t,
               float>> {};


// IsTerminal honours sigma; Cover(leaf) is a length-1 array containing Map(leaf).
TEST_P(CombinedSLPWithUnitCover_TF, IsTerminalAndCover) {
  const auto &sigma = std::get<0>(GetParam());
  const auto &rules = std::get<1>(GetParam());
  const auto &block_size = std::get<2>(GetParam());
  const auto &storing_factor = std::get<3>(GetParam());

  // Build a CombinedSLP, then slice-assign into the wrapper. The wrapper has
  // no data members of its own, so the assignment is a clean upcast from the
  // CombinedSLP base.
  auto cslp = BuildCombinedSLP(sigma, rules, block_size, storing_factor);
  grammar::CombinedSLPWithUnitCover<> wrapped;
  static_cast<grammar::CombinedSLP<> &>(wrapped) = cslp;

  for (std::size_t v = 1; v <= sigma; ++v) EXPECT_TRUE(wrapped.IsTerminal(v));
  EXPECT_FALSE(wrapped.IsTerminal(sigma + 1));

  std::size_t total_leaves = wrapped.GetLeaves().size();
  for (std::size_t leaf = 1; leaf <= total_leaves; ++leaf) {
    auto cover = wrapped.Cover(leaf);
    ASSERT_EQ(cover.size(), 1u);
    EXPECT_EQ(cover[0], wrapped.Map(leaf));
  }
}


// Serialize/load round-trip preserves equality and behaviour. The wrapper has
// no extra fields, so its serialized form is byte-identical to the underlying
// CombinedSLP — cache files are kept distinct only via SDSL's type hash.
TEST_P(CombinedSLPWithUnitCover_TF, SerializeLoadRoundTrip) {
  const auto &sigma = std::get<0>(GetParam());
  const auto &rules = std::get<1>(GetParam());
  const auto &block_size = std::get<2>(GetParam());
  const auto &storing_factor = std::get<3>(GetParam());

  auto cslp = BuildCombinedSLP(sigma, rules, block_size, storing_factor);
  grammar::CombinedSLPWithUnitCover<> wrapped;
  static_cast<grammar::CombinedSLP<> &>(wrapped) = cslp;

  {
    std::ofstream out("tmp.combined_slp_unit_cover", std::ios::binary);
    wrapped.serialize(out);
  }

  grammar::CombinedSLPWithUnitCover<> loaded;
  EXPECT_NE(wrapped, loaded);

  {
    std::ifstream in("tmp.combined_slp_unit_cover", std::ios::binary);
    loaded.load(in);
  }
  EXPECT_EQ(wrapped, loaded);

  // Behaviour parity.
  std::size_t total_leaves = wrapped.GetLeaves().size();
  for (std::size_t leaf = 1; leaf <= total_leaves; ++leaf) {
    EXPECT_EQ(loaded.Cover(leaf), wrapped.Cover(leaf));
    EXPECT_EQ(loaded.Map(leaf), wrapped.Map(leaf));
  }

  std::remove("tmp.combined_slp_unit_cover");
}


INSTANTIATE_TEST_SUITE_P(
    CombinedSLPWithUnitCover,
    CombinedSLPWithUnitCover_TF,
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
            uint32_t{4}, 2.0f)));
