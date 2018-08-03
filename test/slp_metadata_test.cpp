//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/17/18.
//

#include <gtest/gtest.h>

#include <sdsl/vectors.hpp>

#include "grammar/slp.h"
#include "grammar/slp_metadata.h"
#include "grammar/re_pair.h"


using RightHand = std::pair<uint32_t, uint32_t>;
using Rules = std::vector<RightHand>;


class SLPMD_TF : public ::testing::TestWithParam<std::tuple<uint32_t, Rules>> {
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


TEST_P(SLPMD_TF, PTSCompute) {
  grammar::PTS<> pts;
  pts.Compute(&slp_);

  for (auto i = 1u; i <= slp_.Variables(); ++i) {
    auto span = slp_.Span(i);
    sort(span.begin(), span.end());
    span.erase(unique(span.begin(), span.end()), span.end());

    const auto &result = pts[i];
    ASSERT_EQ(result.size(), span.size());
    EXPECT_TRUE(equal(result.begin(), result.end(), span.begin()));
  }
}


TEST_P(SLPMD_TF, PTSConstructor) {
  grammar::PTS<> pts(&slp_);

  for (auto i = 1u; i <= slp_.Variables(); ++i) {
    auto span = slp_.Span(i);
    sort(span.begin(), span.end());
    span.erase(unique(span.begin(), span.end()), span.end());

    const auto &result = pts[i];
    ASSERT_EQ(result.size(), span.size());
    EXPECT_TRUE(equal(result.begin(), result.end(), span.begin()));
  }
}


TEST_P(SLPMD_TF, SampledPTSConstructor) {
  grammar::SampledPTS<grammar::SLP<>> pts(&slp_, 4, 2);

  for (auto i = 1u; i <= slp_.Variables(); ++i) {
    auto span = slp_.Span(i);
    sort(span.begin(), span.end());
    span.erase(unique(span.begin(), span.end()), span.end());

    const auto &result = pts[i];
    ASSERT_EQ(result.size(), span.size()) << i;
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
  grammar::SLP<> slp_{0};

// Sets up the test fixture.
  void SetUp() override {
    auto sigma = 4u;
    Rules rules = {{2, 1}, {3, 5}, {3, 3}, {2, 5}, {4, 6}, {8, 1}, {6, 7}, {11, 6}, {9, 12}, {13, 10}};

    slp_ = grammar::SLP<>(sigma);

    for (auto &&rule : rules) {
      slp_.AddRule(rule.first, rule.second), rule.first;
    }
  }
};


using MyTypes = ::testing::Types<grammar::PTS<>,
                                 grammar::PTS<std::vector<uint64_t>>,
                                 grammar::PTS<sdsl::enc_vector<>>,
                                 grammar::PTS<sdsl::vlc_vector<>>,
                                 grammar::PTS<sdsl::dac_vector<>>,
                                 grammar::SampledPTS<grammar::SLP<>>>;
TYPED_TEST_CASE(SLPMDGeneric_TF, MyTypes);


TYPED_TEST(SLPMDGeneric_TF, PTSConstructor) {
  TypeParam pts(&this->slp_);

  for (auto i = 1u; i <= this->slp_.Variables(); ++i) {
    auto span = this->slp_.Span(i);
    sort(span.begin(), span.end());
    span.erase(unique(span.begin(), span.end()), span.end());

    const auto &result = pts[i];
    ASSERT_EQ(result.size(), span.size());
    EXPECT_TRUE(equal(result.begin(), result.end(), span.begin()));
  }
}


TYPED_TEST(SLPMDGeneric_TF, Serialization) {
  TypeParam pts(&this->slp_);
  {
    std::ofstream out("tmp.slp_metadata", std::ios::binary);
    pts.serialize(out);
  }

  TypeParam pts_loaded;
  EXPECT_FALSE(pts == pts_loaded);

  {
    std::ifstream in("tmp.slp_metadata", std::ios::binary);
    pts_loaded.load(in);
  }
  EXPECT_TRUE(pts == pts_loaded);
}


using Sets = std::vector<std::vector<uint32_t>>;


class Chunks_TF : public ::testing::TestWithParam<std::tuple<Sets, Sets>> {};


TEST_P(Chunks_TF, AddData) {
  const auto &sets = std::get<0>(GetParam());

  grammar::Chunks<> chunks;

  for (int i = 0; i < sets.size(); ++i) {
    EXPECT_EQ(chunks.AddData(sets[i]), i + 1);
    auto s = chunks[i + 1];
    ASSERT_EQ(s.second - s.first, sets[i].size());
    EXPECT_TRUE(std::equal(s.first, s.second, sets[i].begin()));
  }

  for (int i = 0; i < sets.size(); ++i) {
    auto s = chunks[i + 1];
    ASSERT_EQ(s.second - s.first, sets[i].size());
    EXPECT_TRUE(std::equal(s.first, s.second, sets[i].begin()));
  }

  {
    std::ofstream out("tmp.slp_metadata", std::ios::binary);
    chunks.serialize(out);
  }

  grammar::Chunks<> chunks_loaded;
  EXPECT_FALSE(chunks == chunks_loaded);

  {
    std::ifstream in("tmp.slp_metadata", std::ios::binary);
    chunks_loaded.load(in);
  }
  EXPECT_TRUE(chunks == chunks_loaded);
}


TEST_P(Chunks_TF, CompactChunk) {
  const auto &sets = std::get<0>(GetParam());

  grammar::Chunks<> chunks;
  for (const auto &item : sets) {
    chunks.AddData(item);
  }

  grammar::RePairEncoder<false> encoder;
  grammar::CompactChunks<grammar::SLP<>> compact_chunks(chunks.gbegin(), chunks.gend(), chunks, encoder);

  const auto &e_sets = std::get<1>(GetParam());
  EXPECT_EQ(compact_chunks.size(), e_sets.size());

  for (int i = 0; i < sets.size(); ++i) {
    auto s = compact_chunks[i + 1];
    ASSERT_EQ(s.size(), e_sets[i].size()) << i;
    EXPECT_TRUE(std::equal(s.first, s.second, e_sets[i].begin()));
  }
}


TEST_P(Chunks_TF, Serialization) {
  const auto &sets = std::get<0>(GetParam());

  grammar::Chunks<> chunks;
  for (const auto &item : sets) {
    chunks.AddData(item);
  }

  grammar::RePairEncoder<false> encoder;
  grammar::CompactChunks<grammar::SLP<>> compact_chunks(chunks.gbegin(), chunks.gend(), chunks, encoder);

  {
    std::ofstream out("tmp.slp_metadata", std::ios::binary);
    compact_chunks.serialize(out);
  }

  grammar::CompactChunks<grammar::SLP<>> compact_chunks_loaded;
  EXPECT_FALSE(compact_chunks == compact_chunks_loaded);

  {
    std::ifstream in("tmp.slp_metadata", std::ios::binary);
    compact_chunks_loaded.load(in);
  }
  EXPECT_TRUE(compact_chunks == compact_chunks_loaded);
}


INSTANTIATE_TEST_CASE_P(
    SLPMetadata,
    Chunks_TF,
    ::testing::Values(
        std::make_tuple(
            Sets{{1, 2, 3, 4}, {1, 2, 3}, {3}, {1, 2, 3}, {1, 2}},
            Sets{{6, 4}, {6}, {3}, {6}, {5}}
        ),
        std::make_tuple(
            Sets{{1, 2}, {3, 4}, {1, 2, 3}, {3}, {1, 2, 3}, {1, 2}},
            Sets{{5}, {3, 4}, {6}, {3}, {6}, {5}}
        ),
        std::make_tuple(
            Sets{{1}, {2}, {3, 4}, {1, 2, 3}, {3}, {1, 2, 3}, {1, 2}},
            Sets{{1}, {2}, {3, 4}, {6}, {3}, {6}, {5}}
        ),
        std::make_tuple(
            Sets{{1}, {2}, {3, 4}, {1, 2, 3}, {3, 1}, {2, 3}, {1, 2}},
            Sets{{1}, {2}, {3, 4}, {6}, {3, 1}, {2, 3}, {5}}
        ),
        std::make_tuple(
            Sets{{1}, {2}, {3, 4}, {1, 2, 3}, {3, 1}, {2, 3, 1}, {2}},
            Sets{{1}, {2}, {3, 4}, {6}, {3, 1}, {2, 3, 1}, {2}}
        )
    )
);