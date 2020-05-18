//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 5/16/20.
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
//  std::cout << datafile << " --- " << base_name << std::endl;
//  return 0;
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

  uint32_t diff_base;
  std::size_t seq_size;
  {
    std::ifstream in(datafile.string() + ".info");
    in >> seq_size;

    int32_t minimal;
    in >> minimal;
    diff_base = minimal < 0 ? std::abs(minimal) : 0;
  }
//  std::cout << "Diff base: " << diff_base << std::endl;

  sdsl::int_vector<> span_sums(slp.GetRules().size());
  std::size_t i = 0;
  auto sigma = slp.Sigma();
  auto report_sum = [&span_sums, sigma](auto _i, auto _sum) {
    span_sums[_i - sigma - 1] = _sum;
  };

  auto minmax = grammar::ComputeSpanSums(slp, diff_base, report_sum);

//  std::cout << "sigma: " << slp.Sigma() << std::endl;
//  std::cout << "# rules: " << slp.GetRules().size() << std::endl;
//  std::cout << "max len: " << *std::max_element(slp.GetRulesLengths().begin(), slp.GetRulesLengths().end()) << std::endl;
//
//  std::cout << "# sums: " << span_sums.size() << std::endl;
//  std::cout << "minmax original span sum: " << minmax.first << " --- " << minmax.second << std::endl;
//  auto mm = std::minmax_element(span_sums.begin(), span_sums.end());
//  std::cout << "minmax new span sum: " << *mm.first << " --- " << *mm.second << std::endl;

  sdsl::util::bit_compress(span_sums);

  sdsl::store_to_cache(span_sums, KEY_GRM_SPAN_SUMS, config);

  {
    std::ofstream out(cache_file_name(KEY_GRM_SPAN_SUMS, config) + ".info");
    out << minmax.first << std::endl;
    out << minmax.second << std::endl;
  }

  return 0;
}