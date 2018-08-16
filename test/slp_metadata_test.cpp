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


TEST_P(Chunks_TF, InsertAndAccess) {
  const auto &sets = std::get<0>(GetParam());

  grammar::Chunks<> chunks;

  for (int i = 0; i < sets.size(); ++i) {
    EXPECT_EQ(chunks.Insert(sets[i].begin(), sets[i].end()), i + 1);
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


TEST_P(Chunks_TF, CopyConstructor) {
  const auto &sets = std::get<0>(GetParam());

  grammar::Chunks<> chunks;
  for (const auto &item : sets) {
    chunks.Insert(item.begin(), item.end());
  }
//  std::cout << "  |chunks| = " << sdsl::size_in_bytes(chunks.GetObjects()) << std::endl;

  grammar::Chunks<sdsl::int_vector<32>, sdsl::int_vector<>> copied_chunks(chunks);
//  std::cout << "  |copied_chunks| = " << sdsl::size_in_bytes(copied_chunks.GetObjects()) << std::endl;
  for (int i = 0; i < sets.size(); ++i) {
    auto s = copied_chunks[i + 1];
    ASSERT_EQ(s.second - s.first, sets[i].size());
    EXPECT_TRUE(std::equal(s.first, s.second, sets[i].begin()));
  }

  auto bit_compress = [](sdsl::int_vector<> &_v) { sdsl::util::bit_compress(_v); };
  grammar::Chunks<sdsl::int_vector<>, sdsl::int_vector<>> copied_compact_chunks(chunks, bit_compress, bit_compress);
//  std::cout << "  |copied_compact_chunks| = " << sdsl::size_in_bytes(copied_compact_chunks.GetObjects()) << std::endl;
  for (int i = 0; i < sets.size(); ++i) {
    auto s = copied_compact_chunks[i + 1];
    ASSERT_EQ(s.second - s.first, sets[i].size());
    EXPECT_TRUE(std::equal(s.first, s.second, sets[i].begin()));
  }
}


TEST_P(Chunks_TF, Serialization) {
  const auto &sets = std::get<0>(GetParam());

  grammar::Chunks<> chunks;

  for (const auto &item : sets) {
    chunks.Insert(item.begin(), item.end());
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


TEST_P(Chunks_TF, GrammarCompressedChunk) {
  const auto &sets = std::get<0>(GetParam());

  grammar::Chunks<> chunks;
  for (const auto &item : sets) {
    chunks.Insert(item.begin(), item.end());
  }

  grammar::RePairEncoder<false> encoder;
  grammar::GrammarCompressedChunks<grammar::SLP<>, false> compact_chunks(
      chunks.GetObjects().begin(), chunks.GetObjects().end(), chunks, encoder);

  const auto &e_sets = std::get<1>(GetParam());
  EXPECT_EQ(compact_chunks.size(), e_sets.size());

  for (int i = 0; i < sets.size(); ++i) {
    auto s = compact_chunks[i + 1];
    ASSERT_EQ(s.size(), e_sets[i].size()) << i;
    EXPECT_TRUE(std::equal(s.begin(), s.end(), e_sets[i].begin()));
  }
}


TEST_P(Chunks_TF, GrammarCompressedChunkExpand) {
  const auto &sets = std::get<0>(GetParam());

  grammar::Chunks<> chunks;
  for (const auto &item : sets) {
    chunks.Insert(item.begin(), item.end());
  }

  grammar::RePairEncoder<false> encoder;
  grammar::GrammarCompressedChunks<grammar::SLP<>, true> compact_chunks(
      chunks.GetObjects().begin(), chunks.GetObjects().end(), chunks, encoder);

//  const auto &e_sets = std::get<1>(GetParam());
  ASSERT_EQ(compact_chunks.size(), sets.size());

  for (int i = 0; i < sets.size(); ++i) {
    auto s = compact_chunks[i + 1];
    EXPECT_EQ(s, sets[i]) << i;
//    ASSERT_EQ(s.size(), sets[i].size()) << i;
//    EXPECT_TRUE(std::equal(s.begin(), s.end(), sets[i].begin()));
  }
}


TEST_P(Chunks_TF, GrammarCompressedChunkSerialization) {
  const auto &sets = std::get<0>(GetParam());

  grammar::Chunks<> chunks;
  for (const auto &item : sets) {
    chunks.Insert(item.begin(), item.end());
  }

  grammar::RePairEncoder<false> encoder;
  grammar::GrammarCompressedChunks<grammar::SLP<>> compact_chunks(
      chunks.GetObjects().begin(), chunks.GetObjects().end(), chunks, encoder);

  {
    std::ofstream out("tmp.slp_metadata", std::ios::binary);
    compact_chunks.serialize(out);
  }

  grammar::GrammarCompressedChunks<grammar::SLP<>> compact_chunks_loaded;
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
            Sets{{4, 6}, {6}, {3}, {6}, {5}}
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
            Sets{{1}, {2}, {3, 4}, {6}, {1, 3}, {2, 3}, {5}}
        ),
        std::make_tuple(
            Sets{{1}, {2}, {3, 4}, {1, 2, 3}, {3, 1}, {2, 3, 1}, {2}},
            Sets{{1}, {2}, {3, 4}, {6}, {1, 3}, {1, 2, 3}, {2}}
        ),
        std::make_tuple(
            Sets{{1, 2, 3, 4}, {1, 2, 3}, {3}, {1, 2}, {3, 1}, {2}},
            Sets{{4, 6}, {6}, {3}, {5}, {1, 3}, {2}}
        )
    )
);


class GCChunks_TF : public ::testing::TestWithParam<std::tuple<Sets, Sets, Sets>> {};


TEST_P(GCChunks_TF, ComputeAndAccess) {
  const auto &sets = std::get<0>(GetParam());

  grammar::Chunks<> chunks;
  for (const auto &item : sets) {
    chunks.Insert(item.begin(), item.end());
  }

  grammar::RePairEncoder<false> encoder;
  grammar::GCChunks<grammar::SLP<>> gc_chunks(chunks.GetObjects().begin(), chunks.GetObjects().end(), chunks, encoder);

  const auto &t_sets = std::get<1>(GetParam());
  const auto &v_sets = std::get<2>(GetParam());
  EXPECT_EQ(gc_chunks.size(), t_sets.size());

  for (int i = 0; i < sets.size(); ++i) {
    {
      const auto &ts = gc_chunks[i + 1];
      ASSERT_EQ(ts.size(), t_sets[i].size()) << i;
      EXPECT_TRUE(std::equal(ts.begin(), ts.end(), t_sets[i].begin()));
    }
    {
      const auto &vs = gc_chunks.GetVariableSet(i + 1);
      ASSERT_EQ(vs.size(), v_sets[i].size()) << i;
      EXPECT_TRUE(std::equal(vs.begin(), vs.end(), v_sets[i].begin()));
    }
  }
}


TEST_P(GCChunks_TF, Serialization) {
  const auto &sets = std::get<0>(GetParam());

  grammar::Chunks<> chunks;
  for (const auto &item : sets) {
    chunks.Insert(item.begin(), item.end());
  }

  grammar::RePairEncoder<false> encoder;
  grammar::GCChunks<grammar::SLP<>> gc_chunks(chunks.GetObjects().begin(), chunks.GetObjects().end(), chunks, encoder);

  {
    std::ofstream out("tmp.slp_metadata", std::ios::binary);
    gc_chunks.serialize(out);
  }

  grammar::GCChunks<grammar::SLP<>> gc_chunks_loaded;
  EXPECT_FALSE(gc_chunks == gc_chunks_loaded);

  {
    std::ifstream in("tmp.slp_metadata", std::ios::binary);
    gc_chunks_loaded.load(in);
  }
  EXPECT_TRUE(gc_chunks == gc_chunks_loaded);
}


INSTANTIATE_TEST_CASE_P(
    Chunks,
    GCChunks_TF,
    ::testing::Values(
        std::make_tuple(
            Sets{{1, 2, 3, 4}, {1, 2, 3}, {3}, {1, 2, 3}, {1, 2}},
            Sets{{4}, {}, {3}, {}, {}},
            Sets{{6}, {6}, {}, {6}, {5}}
        ),
        std::make_tuple(
            Sets{{1, 2, 3, 4}, {1, 2, 3}, {3}, {1, 2, 3}, {1, 2}, {1, 2, 4}},
            Sets{{4}, {}, {3}, {}, {}, {4}},
            Sets{{6}, {6}, {}, {6}, {5}, {5}}
        ),
        std::make_tuple(
            Sets{{1, 2, 3, 4}, {1, 2, 3}, {3}, {1, 2, 3}, {1, 2}, {1}, {2, 4}},
            Sets{{4}, {}, {3}, {}, {}, {1}, {2, 4}},
            Sets{{6}, {6}, {}, {6}, {5}, {}, {}}
        )
    )
);