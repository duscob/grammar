//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 5/17/20.
//

#include <iostream>
//#include <filesystem>

#include <boost/filesystem.hpp>

#include <gflags/gflags.h>

#include <sdsl/config.hpp>
#include <sdsl/util.hpp>

#include <grammar/slp.h>
#include <grammar/re_pair.h>
#include <grammar/slp_helper.h>
#include <grammar/differential_slp.h>

#include "definitions.h"


DEFINE_string(data, "", "Data file. (MANDATORY)");
DEFINE_int32(max_size, 0, "Maximum size of sampled intervals. (MANDATORY)");

int main(int argc, char **argv) {
  gflags::SetUsageMessage("This program calculates the differential SLP for the given differential data.");
  gflags::AllowCommandLineReparsing();
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  if (FLAGS_data.empty()) {
    std::cerr << "Command-line error!!!" << std::endl;
    return 1;
  }

  boost::filesystem::path datafile = FLAGS_data;
  auto base_name = datafile.stem();

  sdsl::cache_config config(false, ".", base_name.string());

  grammar::SLP<> slp;
  std::vector<std::size_t> compact_seq;
  {
    grammar::RePairReader<false> re_pair_reader;
    auto slp_wrapper = grammar::BuildSLPWrapper(slp);

    auto report_compact_seq = [&compact_seq](const auto &_var) {
      compact_seq.emplace_back(_var);
    };

    re_pair_reader.Read(datafile.string(), slp_wrapper, report_compact_seq);
  }

  std::size_t seq_size;
  uint32_t diff_base_seq;
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

  auto get_span_sum = [&slp, &diff_base_seq, &span_sums, &diff_base_sums](auto _var){
    return slp.IsTerminal(_var) ? (_var - diff_base_seq) : (span_sums[_var - slp.Sigma() - 1] - diff_base_sums);
  };

  sdsl::bit_vector tmp_sample_pos(seq_size, 0);
  std::vector<std::size_t> tmp_samples;
  tmp_samples.reserve(compact_seq.size());
  std::vector<std::size_t> tmp_sample_roots_pos;
  tmp_sample_roots_pos.reserve(compact_seq.size());
  auto report_sample = [&tmp_sample_pos, &tmp_samples, &tmp_sample_roots_pos](auto _seq_pos, auto _sum, auto _root_pos) {
    tmp_sample_pos[_seq_pos] = 1;
    tmp_samples.emplace_back(_sum);
    tmp_sample_roots_pos.emplace_back(_root_pos);
  };

  grammar::ComputeSamplesOnCompactSequence(compact_seq, slp, get_span_sum, FLAGS_max_size, report_sample);

  sdsl::int_vector<> samples;
  grammar::Construct(samples, tmp_samples);
  sdsl::util::bit_compress(samples);
  sdsl::store_to_cache(samples, KEY_GRM_SAMPLE_VALUES /*+ ("_" + std::to_string(FLAGS_max_size))*/, config);

  sdsl::int_vector<> sample_roots_pos;
  grammar::Construct(sample_roots_pos, tmp_sample_roots_pos);
  sdsl::util::bit_compress(sample_roots_pos);
  sdsl::store_to_cache(sample_roots_pos, KEY_GRM_SAMPLE_ROOTS_POSITIONS /*+ ("_" + std::to_string(FLAGS_max_size))*/, config);

  sdsl::sd_vector<> sample_pos(tmp_sample_pos);
  sdsl::store_to_cache(sample_pos, KEY_GRM_SAMPLE_POSITIONS /*+ ("_" + std::to_string(FLAGS_max_size))*/, config);

  return 0;
}
