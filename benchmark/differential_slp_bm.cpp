//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 5/17/20.
//

#include <iostream>
#include <random>

#include <boost/filesystem.hpp>

#include <benchmark/benchmark.h>

#include <gflags/gflags.h>

#include <sdsl/config.hpp>

#include <grammar/slp.h>
#include <grammar/re_pair.h>
#include <grammar/slp_helper.h>
#include <grammar/differential_slp.h>

#include "../tool/definitions.h"

DEFINE_string(data, "", "Data file. (MANDATORY)");

// Benchmark Warm-up
static void BM_WarmUp(benchmark::State &state) {
  for (auto _ : state)
    std::string empty_string;
}
BENCHMARK(BM_WarmUp);

auto BM_Access = [](benchmark::State &_state, const auto &_seq, const auto &_positions) {
  std::vector<std::size_t> values;
  values.reserve(_positions.size());
  for (auto _ : _state) {
    values.clear();
    for (const auto &position : _positions) {
      auto v = _seq[position];
      values.emplace_back(v);
    }
  }

//  std::ofstream out("values-seq.txt");
//  for (const auto &v: values) {
//    out << v << std::endl;
//  }

  _state.counters["Size"] = sdsl::size_in_bytes(_seq);
};

auto BM_ExpandDSLP = [](benchmark::State &_state,
                        const auto &_seq_size,
                        const auto &_slp,
                        const auto &_roots,
                        const auto &_diff_base_seq,
                        const auto &_span_sums,
                        const auto &_diff_base_sums,
                        const auto &_positions,
                        auto _config) {
  std::size_t max_span_len = _state.range(0);

  using BitVector = sdsl::sd_vector<>;
  sdsl::int_vector<> samples;
  sdsl::int_vector<> sample_roots_pos;
  BitVector samples_pos;
  {
    auto get_span_sum = [&_slp, &_diff_base_seq, &_span_sums, &_diff_base_sums](auto _var) {
      return _slp.IsTerminal(_var) ? (_var - _diff_base_seq) : (_span_sums[_var - _slp.Sigma() - 1] - _diff_base_sums);
    };

    sdsl::bit_vector tmp_sample_pos(_seq_size, 0);
    std::vector<std::size_t> tmp_samples;
    tmp_samples.reserve(_roots.size());
    std::vector<std::size_t> tmp_sample_roots_pos;
    tmp_sample_roots_pos.reserve(_roots.size());
    auto report_sample =
        [&tmp_sample_pos, &tmp_samples, &tmp_sample_roots_pos](auto _pos_seq, auto _sum, auto _pos_root) {
          tmp_sample_pos[_pos_seq] = 1;
          tmp_samples.emplace_back(_sum);
          tmp_sample_roots_pos.emplace_back(_pos_root);
        };

    grammar::ComputeSamplesOnCompactSequence(_roots, _slp, get_span_sum, max_span_len, report_sample);

    grammar::Construct(samples, tmp_samples);
    sdsl::util::bit_compress(samples);
//    sdsl::store_to_cache(samples, KEY_GRM_SAMPLE_VALUES + ("_" + std::to_string(max_span_len)), _config);

    grammar::Construct(sample_roots_pos, tmp_sample_roots_pos);
    sdsl::util::bit_compress(sample_roots_pos);
//    sdsl::store_to_cache(sample_roots_pos,
//                         KEY_GRM_SAMPLE_ROOTS_POSITIONS + ("_" + std::to_string(max_span_len)),
//                         _config);

    grammar::Construct(samples_pos, tmp_sample_pos);
//    sdsl::store_to_cache(samples_pos, KEY_GRM_SAMPLE_POSITIONS + ("_" + std::to_string(max_span_len)), _config);
  }
  BitVector::rank_1_type samples_pos_rank(&samples_pos);
  BitVector::select_1_type samples_pos_select(&samples_pos);

  auto dslp = grammar::MakeDifferentialSLPWrapper(_seq_size,
                                                  _slp,
                                                  _roots,
                                                  _diff_base_seq,
                                                  _span_sums,
                                                  _diff_base_sums,
                                                  samples,
                                                  sample_roots_pos,
                                                  samples_pos,
                                                  samples_pos_rank,
                                                  samples_pos_select);

  std::size_t value;
  auto report_value = [&value](std::size_t _suffix) {
    value = _suffix;
  };

  auto get_value = [&dslp, &report_value, &value](std::size_t _i) {
    grammar::ExpandDifferentialSLP(dslp, _i, _i, report_value);
    return value;
  };

  std::vector<std::size_t> values;
  values.reserve(_positions.size());
  for (auto _ : _state) {
    values.clear();
    for (const auto &position : _positions) {
      auto v = get_value(position);
      values.emplace_back(v);
    }
  }

//  std::ofstream out("values-" + std::to_string(max_span_len) + ".txt");
//  for (const auto &v: values) {
//    out << v << std::endl;
//  }

  _state.counters["Size"] = sizeof(_seq_size) + sdsl::size_in_bytes(_slp) + sdsl::size_in_bytes(_roots)
      + sizeof(_diff_base_seq) + sdsl::size_in_bytes(_span_sums) + sizeof(_diff_base_sums)
      + sdsl::size_in_bytes(samples) + sdsl::size_in_bytes(sample_roots_pos) + sdsl::size_in_bytes(samples_pos)
      + sdsl::size_in_bytes(samples_pos_rank) + sdsl::size_in_bytes(samples_pos_select);
};


//**********
//* Main
//**********

int main(int argc, char *argv[]) {
  gflags::AllowCommandLineReparsing();
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  if (FLAGS_data.empty()) {
    std::cerr << "Command-line error!!!" << std::endl;
    return 1;
  }

  boost::filesystem::path datafile = FLAGS_data;
  auto base_name = datafile.stem();

  sdsl::cache_config config(false, ".", base_name.string());

  auto bit_compress = [](sdsl::int_vector<> &_v) { sdsl::util::bit_compress(_v); };

  grammar::SLP<sdsl::int_vector<>, sdsl::int_vector<>> slp;
  sdsl::int_vector<> compact_seq;
  {
    grammar::SLP<> tmp_slp;
    std::vector<std::size_t> tmp_compact_seq;
    grammar::RePairReader<false> re_pair_reader;
    auto slp_wrapper = grammar::BuildSLPWrapper(tmp_slp);

    auto report_compact_seq = [&tmp_compact_seq](const auto &_var) {
      tmp_compact_seq.emplace_back(_var);
    };

    re_pair_reader.Read(datafile.string(), slp_wrapper, report_compact_seq);

    slp = decltype(slp)(tmp_slp, bit_compress, bit_compress);

    grammar::Construct(compact_seq, tmp_compact_seq);
    bit_compress(compact_seq);
  }

  uint32_t diff_base_seq;
  std::size_t seq_size;
  {
    std::ifstream in(datafile.string() + ".info");
    in >> seq_size;

    int32_t minimal;
    in >> minimal;
    diff_base_seq = minimal < 0 ? std::abs(minimal) : 0;
  }

  sdsl::int_vector<> span_sums(slp.GetRules().size());
  sdsl::load_from_cache(span_sums, KEY_GRM_SPAN_SUMS, config);

  uint32_t diff_base_sums;
  {
    std::ifstream in(cache_file_name(KEY_GRM_SPAN_SUMS, config) + ".info");
    int32_t minimal;
    in >> minimal;
    diff_base_sums = minimal < 0 ? std::abs(minimal) : 0;
  }

  const std::size_t kNPositions = 100000;
  std::vector<std::size_t> positions;
  positions.reserve(kNPositions + 2);
  positions.emplace_back(0);
  positions.emplace_back(seq_size - 1);
  {
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 eng(rd()); // seed the generator
    std::uniform_int_distribution<> distr(0, seq_size); // define the range

    for (std::size_t i = 0; i < kNPositions; ++i)
      positions.emplace_back(distr(eng)); // generate numbers
  }

  benchmark::RegisterBenchmark("BM_ExpandDSLP",
                               BM_ExpandDSLP,
                               seq_size,
                               slp,
                               compact_seq,
                               diff_base_seq,
                               span_sums,
                               diff_base_sums,
                               positions,
                               config)
      ->RangeMultiplier(2)->Range(1, 1 << 12);

//  sdsl::int_vector<> sa;
//  sdsl::load_from_file(sa, (datafile.parent_path() / "sa_data.sdsl").string());
//  benchmark::RegisterBenchmark("BM_Access", BM_Access, sa, positions);

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();

  return 0;
}
