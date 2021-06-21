//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 6/18/21.
//

#include <iostream>

#include <gflags/gflags.h>

#include <benchmark/benchmark.h>

#include <sdsl/config.hpp>
#include <sdsl/util.hpp>
#include <sdsl/io.hpp>

#include "grammar/slp.h"
#include "grammar/re_pair.h"
#include "grammar/slp_metadata.h"
#include "grammar/sampled_slp.h"
#include "definitions.h"

DEFINE_string(data, "", "Data file. (MANDATORY)");
DEFINE_uint32(bs, 512, "Block size.");
DEFINE_uint32(sf, 4, "Storing factor.");
DEFINE_bool(rebuild, false, "Rebuild all the items.");

void SetupCommonCounters(benchmark::State &t_state) {
  t_state.counters["leaves"] = 0;
  t_state.counters["nodes"] = 0;
  t_state.counters["partition_pts"] = 0;
  t_state.counters["c_seq"] = 0;
  t_state.counters["partition_c_seq"] = 0;
  t_state.counters["rules"] = 0;
  t_state.counters["valid_rules"] = 0;
}

void SerializeChunks(const grammar::Chunks<> &t_chunks,
                     const std::string &t_prefix,
                     const std::string &t_suffix,
                     sdsl::cache_config *t_config) {
  {
    auto filepath = sdsl::cache_file_name(t_prefix + "_obj_v" + t_suffix, *t_config);
    sdsl::osfstream out(filepath, std::ios::binary | std::ios::trunc | std::ios::out);
    serialize_vector(t_chunks.GetObjects(), out);
  }

  {
    sdsl::int_vector<> pos;
    grammar::Construct(pos, t_chunks.GetObjects());
    sdsl::util::bit_compress(pos);

    sdsl::store_to_cache(pos, t_prefix + "_obj_iv" + t_suffix, *t_config);
  }

  {
    sdsl::int_vector<> pos;
    grammar::Construct(pos, t_chunks.GetChunksPositions());
    sdsl::util::bit_compress(pos);

    sdsl::store_to_cache(pos, t_prefix + "_pos_iv" + t_suffix, *t_config);
  }

  {
    sdsl::bit_vector pos(t_chunks.GetObjects().size(), 0);
    for (const auto &item: t_chunks.GetChunksPositions()) {
      pos[item] = true;
    }

    sdsl::store_to_cache(pos, t_prefix + "_pos_bv" + t_suffix, *t_config);

    sdsl::sd_vector pos_sd(pos);
    sdsl::store_to_cache(pos_sd, t_prefix + "_pos_sd" + t_suffix, *t_config);
  }
}

auto BM_BuildSLPPartition = [](benchmark::State &t_state, auto *t_config) {
  std::string suffix = "-" + std::to_string(FLAGS_bs) + "-" + std::to_string(FLAGS_sf);

  grammar::SLP<> slp;
  {
    auto slp_wrapper = grammar::BuildSLPWrapper(slp);
    grammar::RePairReader<true> reader;
    reader.Read(FLAGS_data, slp_wrapper);
  }

  grammar::SLPPartition<> partition;
  grammar::SLPPartitionTree<> partition_tree;
  sdsl::int_vector<> partition_leaves;

  grammar::Chunks<> slp_pts;
  grammar::AddSet<grammar::Chunks<>> add_set(slp_pts);
  auto predicate = grammar::MustBeSampled<grammar::Chunks<>>(
      grammar::AreChildrenTooBig<grammar::Chunks<>>(slp_pts, FLAGS_sf));

  std::vector<uint32_t> tmp_partition_leaves;
  auto leaf_action = [&tmp_partition_leaves, &add_set](const auto &tt_slp, const auto &tt_var) {
    tmp_partition_leaves.emplace_back(tt_var);
    add_set(tt_slp, tt_var);
  };

  for (auto _ : t_state) {
    slp_pts = grammar::Chunks<>();
    tmp_partition_leaves.clear();

    auto[l, leaves, first_children, parents, next_leaves] = grammar::PartitionSLP(
        slp, FLAGS_bs, predicate, leaf_action, add_set);

    partition = grammar::SLPPartition<>(l, leaves);
    partition_tree = grammar::SLPPartitionTree<>(l, first_children, parents, next_leaves);
    grammar::Construct(partition_leaves, tmp_partition_leaves);
    sdsl::util::bit_compress(partition_leaves);
  }

  sdsl::store_to_cache(partition, grammar::KEY_SLP_PARTITION + suffix, *t_config);
  sdsl::store_to_cache(partition_tree, grammar::KEY_SLP_PARTITION_TREE + suffix, *t_config);
  sdsl::store_to_cache(partition_leaves, grammar::KEY_SLP_PARTITION_VARS + suffix, *t_config);

  SerializeChunks(slp_pts, grammar::KEY_SLP_PTS, suffix, t_config);

  SetupCommonCounters(t_state);
  t_state.counters["leaves"] = partition.size();
  t_state.counters["nodes"] = slp_pts.size();
  t_state.counters["partition_pts"] = slp_pts.GetObjects().size();
};

auto BM_BuildSLPPartitionCSequence = [](benchmark::State &t_state, auto *t_config) {
  std::string suffix = "-" + std::to_string(FLAGS_bs) + "-" + std::to_string(FLAGS_sf);

  grammar::SLP<> slp;
  {
    auto slp_wrapper = grammar::BuildSLPWrapper(slp);
    grammar::RePairReader<true> reader;
    reader.Read(FLAGS_data, slp_wrapper);
  }

  std::vector<uint32_t> c_seq;
  {
    auto report_c_seq = [&c_seq](const auto &tt_var) { c_seq.emplace_back(tt_var); };
    grammar::ReadRePairCompactSequence(FLAGS_data + ".C", report_c_seq);
  }

  sdsl::int_vector<> partition_leaves;
  sdsl::load_from_cache(partition_leaves, grammar::KEY_SLP_PARTITION_VARS + suffix, *t_config);

  auto get_length = [&slp](const auto &tt_var) { return slp.SpanLength(tt_var); };

  grammar::Chunks<> partition_c_seq;
  auto action = [&partition_c_seq](auto &_set) { partition_c_seq.Insert(_set.begin(), _set.end()); };

  for (auto _ : t_state) {
    partition_c_seq = grammar::Chunks<>();

    grammar::ComputePartitionCover(slp, c_seq, partition_leaves, get_length, action, 0);
  }

  SerializeChunks(partition_c_seq, grammar::KEY_SLP_PARTITION_C_SEQ, suffix, t_config);

  SetupCommonCounters(t_state);
  t_state.counters["c_seq"] = c_seq.size();
  t_state.counters["partition_c_seq"] = partition_c_seq.GetObjects().size();
};

auto BM_BuildSLPPartitionRules = [](benchmark::State &t_state, auto *t_config) {
  std::string suffix = "-" + std::to_string(FLAGS_bs);

  grammar::SLP<> slp;
  {
    auto slp_wrapper = grammar::BuildSLPWrapper(slp);
    auto report_c_seq = [](const auto &tt_var) {};
    grammar::RePairReader<false> reader;
    reader.Read(FLAGS_data, slp_wrapper, report_c_seq);
  }

  std::vector<uint32_t> rules;

  for (auto _ : t_state) {
    rules = std::vector<uint32_t>();
    sdsl::bit_vector valid_rules(slp.Start(), 0);

    for (uint32_t i = 0; i <= slp.Start(); ++i) {
      if (slp.SpanLength(i) <= FLAGS_bs) {
        valid_rules[i] = true;
      }
    }

    sdsl::bit_vector::rank_1_type rank(&valid_rules);

    for (uint32_t i = slp.Sigma() + 1; i <= slp.Start(); ++i) {
      if (valid_rules[i]) {
        auto[left, right] = slp[i];
        rules.emplace_back(slp.IsTerminal(left) ? left : rank(left + 1));
        rules.emplace_back(slp.IsTerminal(right) ? right : rank(right + 1));
      }
    }
  }

  sdsl::store_to_cache(rules, grammar::KEY_SLP_RULES + "_v" + suffix, *t_config);

  {
    sdsl::int_vector<> rules_iv;
    grammar::Construct(rules_iv, rules);
    sdsl::util::bit_compress(rules_iv);
    sdsl::store_to_cache(rules_iv, grammar::KEY_SLP_RULES + "_iv" + suffix, *t_config);
  }

  SetupCommonCounters(t_state);
  t_state.counters["rules"] = slp.Start();
  t_state.counters["valid_rules"] = slp.Sigma() + rules.size() / 2;
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

  std::string suffix = "-" + std::to_string(FLAGS_bs) + "-" + std::to_string(FLAGS_sf);

  if (!cache_file_exists(grammar::KEY_SLP_PTS + "_obj_v" + suffix, config) || FLAGS_rebuild) {
    benchmark::RegisterBenchmark("BuildSLPPartition", BM_BuildSLPPartition, &config);
  }

  if (!cache_file_exists(grammar::KEY_SLP_PARTITION_C_SEQ + "_obj_v" + suffix, config) || FLAGS_rebuild) {
    benchmark::RegisterBenchmark("BuildSLPPartitionCSequence", BM_BuildSLPPartitionCSequence, &config);
  }

  if (!cache_file_exists(grammar::KEY_SLP_RULES + "_v" + std::to_string(FLAGS_bs), config) || FLAGS_rebuild) {
    benchmark::RegisterBenchmark("BuildSLPPartitionRules", BM_BuildSLPPartitionRules, &config);
  }

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();

  return 0;
}
