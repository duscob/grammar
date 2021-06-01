//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 5/23/21.
//

#include <iostream>
#include <functional>

#include <gflags/gflags.h>

#include <benchmark/benchmark.h>

#include <sdsl/config.hpp>
#include <sdsl/io.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/bit_vectors.hpp>

#include "grammar/slp_helper.h"
#include "grammar/slp_compact.h"
#include "definitions.h"

DEFINE_string(data_dir, "./", "Data directory.");
DEFINE_string(data_name, "data", "Data file basename.");
DEFINE_bool(print_result, false, "Execute benchmark that print results per index.");

void SetupDefaultCounters(benchmark::State &t_state) {
  t_state.counters["Collection_Size(bytes)"] = 0;
  t_state.counters["Size(bytes)"] = 0;
  t_state.counters["Bits_x_Symbol"] = 0;
  t_state.counters["Time_x_Symbol"] = 0;
}

// Benchmark Warm-up
static void BM_WarmUp(benchmark::State &_state) {
  for (auto _ : _state) {
    std::vector<int> empty_vector(1000000, 0);
  }

  SetupDefaultCounters(_state);
}
BENCHMARK(BM_WarmUp);

auto BM_QueryExpand = [](benchmark::State &t_state, const auto &t_expand, const auto &t_vars, auto t_slp_size) {
  std::size_t total_expansion = 0;

  for (auto _ : t_state) {
    total_expansion = 0;
    for (const auto &var: t_vars) {
      std::vector<size_t> expansion;
      auto report = [&expansion](auto tt_symbol) { expansion.emplace_back(tt_symbol); };

      t_expand(var, std::numeric_limits<size_t>::max(), report);
      total_expansion += expansion.size();
    }
  }

  SetupDefaultCounters(t_state);
  t_state.counters["Collection_Size(bytes)"] = total_expansion;
  t_state.counters["Size(bytes)"] = t_slp_size;
  t_state.counters["Bits_x_Symbol"] = t_slp_size * 8.0 / total_expansion;
  t_state.counters["Time_x_Symbol"] = benchmark::Counter(
      total_expansion, benchmark::Counter::kIsIterationInvariantRate | benchmark::Counter::kInvert);
};

auto BM_PrintQueryExpand =
    [](benchmark::State &t_state, const auto &t_prefix, const auto &t_expand, const auto &t_vars, auto t_slp_size) {
      std::size_t total_expansion = 0;

      sdsl::cache_config config(false, ".", FLAGS_data_name);

      for (auto _ : t_state) {
        total_expansion = 0;
        std::vector<int> expansion;
        for (const auto &var: t_vars) {
          auto report = [&expansion](auto tt_symbol) { expansion.emplace_back(tt_symbol); };

          t_expand(var, std::numeric_limits<size_t>::max(), report);
        }

        total_expansion = expansion.size();
        sdsl::store_to_cache(expansion, t_prefix, config);
      }

      SetupDefaultCounters(t_state);
      t_state.counters["Collection_Size(bytes)"] = total_expansion;
      t_state.counters["Size(bytes)"] = t_slp_size;
      t_state.counters["Bits_x_Symbol"] = t_slp_size * 8.0 / total_expansion;
      t_state.counters["Time_x_Symbol"] = benchmark::Counter(
          total_expansion, benchmark::Counter::kIsIterationInvariantRate | benchmark::Counter::kInvert);
    };

int main(int argc, char **argv) {
  gflags::SetUsageMessage("This program calculates the compact SLP items for the given text.");
  gflags::AllowCommandLineReparsing();
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  if (FLAGS_data_name.empty() || FLAGS_data_dir.empty()) {
    std::cerr << "Command-line error!!!" << std::endl;
    return 1;
  }

  sdsl::cache_config config(true, FLAGS_data_dir, FLAGS_data_name);

  int sigma;
  sdsl::load_from_cache(sigma, grammar::KEY_SLP_SIGMA, config);

  //********************
  // SLP Forward
  //********************
  sdsl::int_vector<> rules_iv;
  sdsl::load_from_cache(rules_iv, grammar::KEY_SLP_RULES + "_iv", config);

  auto size_expand_slp = sdsl::size_in_bytes(rules_iv);

  std::vector<int> vars(sigma + rules_iv.size() / 2);
  std::generate(vars.begin(), vars.end(), [n = sigma + rules_iv.size() / 2]() mutable { return n--; });

  auto expand_slp = [&rules_iv, &sigma](auto tt_var, auto tt_len, auto &tt_report) {
    grammar::ExpandSLPForward(rules_iv, sigma, tt_var, tt_len, tt_report);
  };

  std::vector<int> c_seq;
  sdsl::load_from_cache(c_seq, grammar::KEY_SLP_C_SEQ + "_v", config);
  benchmark::RegisterBenchmark("slp-forward", BM_QueryExpand, expand_slp, c_seq, size_expand_slp);
  if (FLAGS_print_result)
    benchmark::RegisterBenchmark("print-slp-forward", BM_PrintQueryExpand, "f", expand_slp, c_seq, size_expand_slp);

  //********************
  // SLP Backward
  //********************
  auto expand_slp_backward = [&rules_iv, &sigma](auto tt_var, auto tt_len, auto &tt_report) {
    grammar::ExpandSLPBackward(rules_iv, sigma, tt_var, tt_len, tt_report);
  };

  benchmark::RegisterBenchmark("slp-backward", BM_QueryExpand, expand_slp_backward, c_seq, size_expand_slp);
  if (FLAGS_print_result)
    benchmark::RegisterBenchmark(
        "print-slp-backward", BM_PrintQueryExpand, "b", expand_slp_backward, c_seq, size_expand_slp);

  //********************
  // Compact SLP Forward
  //********************
  sdsl::bit_vector tree_bv;
  sdsl::load_from_cache(tree_bv, grammar::KEY_SLP_COMPACT_TREE + "_bv", config);

  auto leaf_rank = sdsl::bit_vector::rank_0_type(&tree_bv);

  sdsl::int_vector<> leaves_iv;
  sdsl::load_from_cache(leaves_iv, grammar::KEY_SLP_COMPACT_LEAVES + "_iv", config);

  auto size_expand_slp_c =
      sdsl::size_in_bytes(tree_bv) + sdsl::size_in_bytes(leaf_rank) + sdsl::size_in_bytes(leaves_iv);

  std::vector<int> c_seq_slp_c;
  sdsl::load_from_cache(c_seq_slp_c, grammar::KEY_SLP_COMPACT_C_SEQ + "_v", config);

  auto expand_slp_c = [&tree_bv, &sigma, &leaves_iv, &leaf_rank](auto tt_var, auto tt_len, auto &tt_report) {
    grammar::ExpandCompactSLPForward(tree_bv, sigma, leaves_iv, leaf_rank, tt_var, tt_len, tt_report);
  };

  benchmark::RegisterBenchmark("slp-c-forward", BM_QueryExpand, expand_slp_c, c_seq_slp_c, size_expand_slp_c);
  if (FLAGS_print_result)
    benchmark::RegisterBenchmark(
        "print-slp-c-forward", BM_PrintQueryExpand, "f-c", expand_slp_c, c_seq_slp_c, size_expand_slp_c);

  //********************
  // Compact SLP Backward
  //********************
  auto expand_slp_c_backward = [&tree_bv, &sigma, &leaves_iv, &leaf_rank](auto tt_var, auto tt_len, auto &tt_report) {
    grammar::ExpandCompactSLPBackward(tree_bv, sigma, leaves_iv, leaf_rank, tt_var, tt_len, tt_report);
  };

  benchmark::RegisterBenchmark("slp-c-backward", BM_QueryExpand, expand_slp_c_backward, c_seq_slp_c, size_expand_slp_c);
  if (FLAGS_print_result)
    benchmark::RegisterBenchmark(
        "print-slp-c-backward", BM_PrintQueryExpand, "b-c", expand_slp_c_backward, c_seq_slp_c, size_expand_slp_c);

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();

  return 0;
}
