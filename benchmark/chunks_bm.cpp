//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 8/15/18.
//

#include <iostream>
#include <random>
#include <algorithm>

#include <boost/filesystem.hpp>

#include <benchmark/benchmark.h>

#include <gflags/gflags.h>

#include <sdsl/io.hpp>
#include <sdsl/vectors.hpp>

#include "grammar/slp_metadata.h"
#include "grammar/slp.h"
#include "grammar/re_pair.h"


DEFINE_string(data, "", "Data file.");


static void BM_Empty(benchmark::State &state) {
  for (auto _ : state)
    std::string empty_string;

  state.counters["Items"] = 0;
}
// Register the function as a benchmark
BENCHMARK(BM_Empty);


template<typename _GCChunks>
class VChunksWrapper {
 public:
  VChunksWrapper(const _GCChunks &_gcchunks): gcchunks_(_gcchunks){}

  auto operator[](std::size_t i) const {
    return gcchunks_.GetVariableSet(i);
  }

 protected:
  const _GCChunks &gcchunks_;
};

template<typename _GCChunks>
auto BuildVChunksWrapper(const _GCChunks &_gcchunks) {
  return VChunksWrapper<_GCChunks>(_gcchunks);
}


template<typename _SLP>
class VSpanWrapper {
 public:
  VSpanWrapper(const _SLP &_slp): slp_(_slp){}

  auto operator[](std::size_t i) const {
    return slp_.Span(i);
  }

 protected:
  const _SLP &slp_;
};

template<typename _SLP>
auto BuildVSpanWrapper(const _SLP &_slp) {
  return VSpanWrapper<_SLP>(_slp);
}


auto BM_merge_chunks = [](benchmark::State &state, const auto &chunks, const auto &merge, const auto &set_union) {
  std::vector<uint32_t> result;

  std::vector<uint32_t> items;

  std::default_random_engine gen(state.range(0));
  std::uniform_int_distribution<> uniform_dist(1, chunks.size());
  for (int i = 0; i < state.range(0); ++i) {
    items.push_back(uniform_dist(gen));
  }

  for (auto _ : state) {
    result.clear();

    merge(items.begin(), items.end(), chunks, result, set_union);
//    grammar::MergeSetsOneByOne(items.begin(), items.end(), chunks, result, set_union);

//    std::vector<uint32_t> tmp_set;
//
//    for (const auto &item : items) {
////    for (int i = 0; i < st.range(0); ++i) {
//      const auto &set = chunks[item];
//
//      tmp_set.resize(result.size() + set.size());
//      tmp_set.swap(result);
//
//      auto last_it = std::set_union(tmp_set.begin(), tmp_set.end(), set.begin(), set.end(), result.begin());
//      result.resize(last_it - result.begin());
//    }
  }

  state.counters["Items"] = result.size();
};

auto BM_merge_gcchunks = [](benchmark::State &state, const auto &chunks, const auto &merge, const auto &set_union) {
  std::vector<uint32_t> result;
  std::vector<uint32_t> tresult;
  std::vector<uint32_t> vresult;
  std::vector<uint32_t> sresult;

  std::vector<uint32_t> items;

  std::default_random_engine gen(state.range(0));
  std::uniform_int_distribution<> uniform_dist(1, chunks.size());
  for (int i = 0; i < state.range(0); ++i) {
    items.push_back(uniform_dist(gen));
  }

  for (auto _ : state) {
    tresult.clear();
    vresult.clear();
    sresult.clear();
    result.clear();

    merge(items.begin(), items.end(), chunks, tresult, set_union);
    merge(items.begin(), items.end(), BuildVChunksWrapper(chunks), vresult, set_union);
    merge(vresult.begin(), vresult.end(), BuildVSpanWrapper(chunks.GetSLP()), sresult, set_union);

    result.reserve(tresult.size() + sresult.size());
    std::set_union(tresult.begin(), tresult.end(), sresult.begin(), sresult.end(), back_inserter(result));
  }

  state.counters["Items"] = result.size();
};


int main(int argc, char *argv[]) {
  gflags::AllowCommandLineReparsing();
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  if (FLAGS_data.empty()) {
    std::cerr << "Command-line error!!!" << std::endl;
    return 1;
  }

  auto set_union_default = [](auto _first1, auto _last1, auto _first2, auto _last2, auto _result) -> auto {
    return std::set_union(_first1, _last1, _first2, _last2, _result);
  };

  auto set_union_custom = [](auto _first1, auto _last1, auto _first2, auto _last2, auto _result) -> auto {
    return grammar::SetUnion(_first1, _last1, _first2, _last2, _result);
  };

  grammar::MergeSetsOneByOneFunctor merge_one_by_one;
//  auto merge_one_by_one = [](auto _first, auto _last, const auto &_sets, auto &_result, const auto &_set_union) {
//    grammar::MergeSetsOneByOne(_first, _last, _sets, _result, _set_union);
//  };

  grammar::Chunks<> chunks;
  std::ifstream in(FLAGS_data, std::ios::binary | std::ios::in);
  sdsl::load(chunks, in);

  std::cout << "  *** |Chunks| = " << sdsl::size_in_bytes(chunks) << std::endl;
  benchmark::RegisterBenchmark("Chunks",
                               BM_merge_chunks,
                               chunks,
                               merge_one_by_one,
                               set_union_default)->Arg(8);
  benchmark::RegisterBenchmark("Chunks_set_union_custom",
                               BM_merge_chunks,
                               chunks,
                               merge_one_by_one,
                               set_union_custom)->Arg(8);

  auto bit_compress = [](auto &_v) { sdsl::util::bit_compress(_v); };

  grammar::Chunks<sdsl::int_vector<>, sdsl::int_vector<>> chunks_bc(chunks, bit_compress, bit_compress);
  std::cout << "  *** |Chunks<BC>| = " << sdsl::size_in_bytes(chunks_bc) << std::endl;
  benchmark::RegisterBenchmark("Chunks<BC>",
                               BM_merge_chunks,
                               chunks_bc,
                               merge_one_by_one,
                               set_union_default)->Arg(8);
  benchmark::RegisterBenchmark("Chunks<BC>_set_union_custom",
                               BM_merge_chunks,
                               chunks_bc,
                               merge_one_by_one,
                               set_union_custom)->Arg(8);

  grammar::Chunks<sdsl::enc_vector<>, sdsl::enc_vector<>> chunks_encv(chunks);
  std::cout << "  *** |Chunks<EncV>| = " << sdsl::size_in_bytes(chunks_encv) << std::endl;
  benchmark::RegisterBenchmark("Chunks<EncV>",
                               BM_merge_chunks,
                               chunks_encv,
                               merge_one_by_one,
                               set_union_default)->Arg(8);
  benchmark::RegisterBenchmark("Chunks<EncV>_set_union_custom",
                               BM_merge_chunks,
                               chunks_encv,
                               merge_one_by_one,
                               set_union_custom)->Arg(8);

  grammar::Chunks<sdsl::vlc_vector<>, sdsl::vlc_vector<>> chunks_vlcv(chunks);
  std::cout << "  *** |Chunks<VlcV>| = " << sdsl::size_in_bytes(chunks_vlcv) << std::endl;
  benchmark::RegisterBenchmark("Chunks<VlcV>",
                               BM_merge_chunks,
                               chunks_vlcv,
                               merge_one_by_one,
                               set_union_default)->Arg(8);
  benchmark::RegisterBenchmark("Chunks<VlcV>_set_union_custom",
                               BM_merge_chunks,
                               chunks_vlcv,
                               merge_one_by_one,
                               set_union_custom)->Arg(8);

  grammar::RePairEncoder<false> encoder;
//  grammar::GrammarCompressedChunks<grammar::SLP<>> chunks_gc(
//      chunks.GetObjects().begin(), chunks.GetObjects().end(), chunks, encoder);
//  std::cout << "  *** |Chunks_GC| = " << sdsl::size_in_bytes(chunks_gc) << std::endl;
//  benchmark::RegisterBenchmark("Chunks_GC",
//                               BM_merge_chunks,
//                               chunks_gc,
//                               merge_one_by_one,
//                               set_union_default)->Arg(8);
//  benchmark::RegisterBenchmark("Chunks_GC",
//                               BM_merge_chunks,
//                               chunks_gc,
//                               merge_one_by_one,
//                               set_union_custom)->Arg(8);
//
//  grammar::GrammarCompressedChunks<grammar::SLP<>, false> chunks_gc_nexp(
//      chunks.GetObjects().begin(), chunks.GetObjects().end(), chunks, encoder);
//  std::cout << "  *** |Chunks_GC_NoExp| = " << sdsl::size_in_bytes(chunks_gc_nexp) << std::endl;
//  benchmark::RegisterBenchmark("Chunks_GC_NoExp",
//                               BM_merge_chunks,
//                               chunks_gc_nexp,
//                               merge_one_by_one,
//                               set_union_default)->Arg(8);
//  benchmark::RegisterBenchmark("Chunks_GC_NoExp",
//                               BM_merge_chunks,
//                               chunks_gc_nexp,
//                               merge_one_by_one,
//                               set_union_custom)->Arg(8);
//
  grammar::GCChunks<grammar::SLP<>> gcchunks(chunks.GetObjects().begin(), chunks.GetObjects().end(), chunks, encoder);
  std::cout << "  *** |GCChunks| = " << sdsl::size_in_bytes(gcchunks) << std::endl;
  benchmark::RegisterBenchmark("GCChunks", BM_merge_chunks, gcchunks, merge_one_by_one, set_union_default)->Arg(8);
  benchmark::RegisterBenchmark("GCChunks_All", BM_merge_gcchunks, gcchunks, merge_one_by_one, set_union_default)->Arg(8);
  benchmark::RegisterBenchmark("GCChunks", BM_merge_chunks, gcchunks, merge_one_by_one, set_union_custom)->Arg(8);
  benchmark::RegisterBenchmark("GCChunks_All", BM_merge_gcchunks, gcchunks, merge_one_by_one, set_union_custom)->Arg(8);

  grammar::GCChunks<grammar::SLP<sdsl::int_vector<>, sdsl::int_vector<>>,
                    grammar::Chunks<sdsl::int_vector<>, sdsl::int_vector<>>,
                    grammar::Chunks<sdsl::int_vector<>, sdsl::int_vector<>>> gcchunks_bc(
      gcchunks.GetSLP(),
      gcchunks.GetTerminalChunks(),
      gcchunks.GetVariableChunks(),
      bit_compress,
      bit_compress,
      bit_compress,
      bit_compress,
      bit_compress,
      bit_compress);
  std::cout << "  *** |GCChunks<bc>| = " << sdsl::size_in_bytes(gcchunks_bc) << std::endl;
  benchmark::RegisterBenchmark("GCChunks<bc>",
                               BM_merge_chunks,
                               gcchunks_bc,
                               merge_one_by_one,
                               set_union_default)->Arg(8);
  benchmark::RegisterBenchmark("GCChunks<bc>_All",
                               BM_merge_gcchunks,
                               gcchunks_bc,
                               merge_one_by_one,
                               set_union_default)->Arg(8);
  benchmark::RegisterBenchmark("GCChunks<bc>", BM_merge_chunks, gcchunks_bc, merge_one_by_one, set_union_custom)->Arg(8);
  benchmark::RegisterBenchmark("GCChunks<bc>_All", BM_merge_gcchunks, gcchunks_bc, merge_one_by_one, set_union_custom)->Arg(8);

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();

  return 0;
}