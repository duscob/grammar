//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 5/20/21.
//

#include <iostream>

#include <gflags/gflags.h>

#include <benchmark/benchmark.h>

#include <sdsl/config.hpp>
#include <sdsl/util.hpp>
#include <sdsl/io.hpp>
#include <sdsl/bit_vectors.hpp>

#include "grammar/re_pair.h"
#include "grammar/slp_compact.h"
#include "grammar/sampled_slp.h"
#include "grammar/utility.h"
#include "definitions.h"

DEFINE_string(data, "", "Data file. (MANDATORY)");
DEFINE_bool(rebuild, false, "Rebuild all the items.");

void SetupCommonCounters(benchmark::State &t_state) {
  t_state.counters["sigma"] = 0;
  t_state.counters["rules"] = 0;
  t_state.counters["c_seq"] = 0;
  t_state.counters["tree"] = 0;
  t_state.counters["leaves"] = 0;
}

auto BM_EncodeSLPRules = [](benchmark::State &t_state, auto *t_config) {
  int sigma;
  std::vector<int> rules;

  auto report_sigma = [&sigma](const auto &tt_sigma) { sigma = tt_sigma; };
  auto report_rule = [&rules](auto tt_left, auto tt_right) {
    rules.emplace_back(tt_left);
    rules.emplace_back(tt_right);
  };

  grammar::ReadRePairRules(FLAGS_data + ".R", report_sigma, report_rule);

  sdsl::int_vector<> rules_iv;
  for (auto _ : t_state) {
    grammar::Construct(rules_iv, rules);
    sdsl::util::bit_compress(rules_iv);
  }

  sdsl::store_to_cache(sigma, grammar::KEY_SLP_SIGMA, *t_config);
  sdsl::store_to_cache(rules, grammar::KEY_SLP_RULES + "_v", *t_config);
  sdsl::store_to_cache(rules_iv, grammar::KEY_SLP_RULES + "_iv", *t_config);

  SetupCommonCounters(t_state);
  t_state.counters["sigma"] = sigma;
  t_state.counters["rules"] = rules.size();
};

auto BM_EncodeSLPCompressedSequence = [](benchmark::State &t_state, auto *t_config) {
  std::vector<int> c_seq;

  auto report_c_seq = [&c_seq](const auto &tt_symbol) {
    c_seq.push_back(tt_symbol);
  };

  grammar::ReadRePairCompactSequence(FLAGS_data + ".C", report_c_seq);
  sdsl::int_vector<> c_seq_iv;
  for (auto _ : t_state) {
    grammar::Construct(c_seq_iv, c_seq);
    sdsl::util::bit_compress(c_seq_iv);
  }

  sdsl::store_to_cache(c_seq, grammar::KEY_SLP_C_SEQ + "_v", *t_config);
  sdsl::store_to_cache(c_seq_iv, grammar::KEY_SLP_C_SEQ + "_iv", *t_config);

  SetupCommonCounters(t_state);
  t_state.counters["c_seq"] = c_seq.size();
};

auto BM_BuildCompactSLP = [](benchmark::State &t_state, auto *t_config) {
  int sigma;
  std::vector<int> rules;

  sdsl::load_from_cache(sigma, grammar::KEY_SLP_SIGMA, *t_config);
  sdsl::load_from_cache(rules, grammar::KEY_SLP_RULES + "_v", *t_config);

  auto get_children = [&rules, sigma](const auto &tt_var) {
    auto pos = (tt_var - sigma - 1) * 2;
    return std::make_pair(rules[pos], rules[pos + 1]);
  };

  std::vector<int> roots(sigma + rules.size() / 2);
  std::generate(roots.begin(), roots.end(), [n = sigma + rules.size() / 2]() mutable { return n--; });

  decltype(grammar::CreateCompactGrammarForest(roots, sigma, get_children)) compact_slp;

  for (auto _ : t_state) {
    compact_slp = grammar::CreateCompactGrammarForest(roots, sigma, get_children);
  }

  const auto &[tree, leaves, nt_map] = compact_slp;

  sdsl::store_to_cache(tree, grammar::KEY_SLP_COMPACT_TREE + "_vb", *t_config);
  {
    sdsl::bit_vector tree_bv;
    grammar::Construct(tree_bv, tree);
    sdsl::store_to_cache(tree_bv, grammar::KEY_SLP_COMPACT_TREE + "_bv", *t_config);
  }

  sdsl::store_to_cache(leaves, grammar::KEY_SLP_COMPACT_LEAVES + "_v", *t_config);
  {
    sdsl::int_vector<> leaves_iv;
    grammar::Construct(leaves_iv, leaves);
    sdsl::util::bit_compress(leaves_iv);
    sdsl::store_to_cache(leaves_iv, grammar::KEY_SLP_COMPACT_LEAVES + "_iv", *t_config);
  }

  {
    std::vector<std::size_t> nt_map_vec;
    for (const auto &item: nt_map) {
      nt_map_vec.emplace_back(item.first);
      nt_map_vec.emplace_back(item.second);
    }
    sdsl::store_to_cache(nt_map_vec, grammar::KEY_SLP_COMPACT_NO_TERMINAL_MAP + "_vec", *t_config);
  }

  {
    std::vector<int> c_seq;
    sdsl::load_from_cache(c_seq, grammar::KEY_SLP_C_SEQ + "_v", *t_config);

    for (auto &item : c_seq) {
      if (item > sigma)
        item = nt_map.at(item);
    }
    sdsl::store_to_cache(c_seq, grammar::KEY_SLP_COMPACT_C_SEQ + "_v", *t_config);

    sdsl::int_vector<> c_seq_iv;
    grammar::Construct(c_seq_iv, c_seq);
    sdsl::util::bit_compress(c_seq_iv);
    sdsl::store_to_cache(c_seq_iv, grammar::KEY_SLP_COMPACT_C_SEQ + "_iv", *t_config);
  }

  SetupCommonCounters(t_state);
  t_state.counters["sigma"] = sigma;
  t_state.counters["rules"] = rules.size();
  t_state.counters["tree"] = tree.size();
  t_state.counters["leaves"] = leaves.size();
};

auto BM_BuildCompactSLPWithBP = [](benchmark::State &t_state, auto *t_config) {
  int sigma;
  std::vector<int> rules;

  sdsl::load_from_cache(sigma, grammar::KEY_SLP_SIGMA, *t_config);
  sdsl::load_from_cache(rules, grammar::KEY_SLP_RULES + "_v", *t_config);

  auto get_children = [&rules, sigma](const auto &tt_var) {
    auto pos = (tt_var - sigma - 1) * 2;
    return std::make_pair(rules[pos], rules[pos + 1]);
  };

  std::vector<int> roots(sigma + rules.size() / 2);
  std::generate(roots.begin(), roots.end(), [n = sigma + rules.size() / 2]() mutable { return n--; });

  decltype(grammar::CreateCompactGrammarTreeWithBP(roots, sigma, get_children)) compact_slp;

  for (auto _ : t_state) {
    compact_slp = grammar::CreateCompactGrammarTreeWithBP(roots, sigma, get_children);
  }

  const auto &[tree, leaves, nt_map] = compact_slp;

  sdsl::store_to_cache(tree, grammar::KEY_SLP_COMPACT_WITH_BP_TREE + "_vb", *t_config);
  {
    sdsl::bit_vector tree_bv;
    grammar::Construct(tree_bv, tree);
    sdsl::store_to_cache(tree_bv, grammar::KEY_SLP_COMPACT_WITH_BP_TREE + "_bv", *t_config);
  }

  sdsl::store_to_cache(leaves, grammar::KEY_SLP_COMPACT_WITH_BP_LEAVES + "_v", *t_config);
  {
    sdsl::int_vector<> leaves_iv;
    grammar::Construct(leaves_iv, leaves);
    sdsl::util::bit_compress(leaves_iv);
    sdsl::store_to_cache(leaves_iv, grammar::KEY_SLP_COMPACT_WITH_BP_LEAVES + "_iv", *t_config);
  }

  {
    std::vector<std::size_t> nt_map_vec;
    for (const auto &item: nt_map) {
      nt_map_vec.emplace_back(item.first);
      nt_map_vec.emplace_back(item.second);
    }
    sdsl::store_to_cache(nt_map_vec, grammar::KEY_SLP_COMPACT_WITH_BP_NO_TERMINAL_MAP + "_vec", *t_config);
  }

  {
    std::vector<int> c_seq;
    sdsl::load_from_cache(c_seq, grammar::KEY_SLP_C_SEQ + "_v", *t_config);

    for (auto &item : c_seq) {
      if (item > sigma)
        item = nt_map.at(item);
    }
    sdsl::store_to_cache(c_seq, grammar::KEY_SLP_COMPACT_WITH_BP_C_SEQ + "_v", *t_config);

    sdsl::int_vector<> c_seq_iv;
    grammar::Construct(c_seq_iv, c_seq);
    sdsl::util::bit_compress(c_seq_iv);
    sdsl::store_to_cache(c_seq_iv, grammar::KEY_SLP_COMPACT_WITH_BP_C_SEQ + "_iv", *t_config);
  }

  SetupCommonCounters(t_state);
  t_state.counters["sigma"] = sigma;
  t_state.counters["rules"] = rules.size();
  t_state.counters["tree"] = tree.size();
  t_state.counters["leaves"] = leaves.size();
};

int main(int argc, char **argv) {
  gflags::SetUsageMessage("This program calculates the compact SLP items for the given text.");
  gflags::AllowCommandLineReparsing();
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  if (FLAGS_data.empty()) {
    std::cerr << "Command-line error!!!" << std::endl;
    return 1;
  }

  std::string data_path = FLAGS_data;

  sdsl::cache_config config(false, ".", sdsl::util::basename(FLAGS_data));

  if (!cache_file_exists(grammar::KEY_SLP_RULES + "_iv", config) || FLAGS_rebuild) {
    benchmark::RegisterBenchmark("EncodeSLPRules", BM_EncodeSLPRules, &config);
  }

  if (!cache_file_exists(grammar::KEY_SLP_C_SEQ + "_iv", config) || FLAGS_rebuild) {
    benchmark::RegisterBenchmark("EncodeSLPCSeq", BM_EncodeSLPCompressedSequence, &config);
  }

  if (!cache_file_exists(grammar::KEY_SLP_COMPACT_TREE + "_bv", config) || FLAGS_rebuild) {
    benchmark::RegisterBenchmark("BuildCompatSLP", BM_BuildCompactSLP, &config);
  }

  if (!cache_file_exists(grammar::KEY_SLP_COMPACT_WITH_BP_TREE + "_bv", config) || FLAGS_rebuild) {
    benchmark::RegisterBenchmark("BuildCompatSLPWithBP", BM_BuildCompactSLPWithBP, &config);
  }

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();

  return 0;
}
