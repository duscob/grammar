//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 5/9/20.
//

#ifndef GRAMMAR_DIFFERENTIAL_SLP_H_
#define GRAMMAR_DIFFERENTIAL_SLP_H_

#include <sdsl/bit_vectors.hpp>

#include <grammar/slp.h>

namespace grammar {
template<typename SLP, typename ReportSpanSum>
auto ComputeSpanSums(const SLP &_slp, std::size_t _diff_base, ReportSpanSum &_report_span_sum) {
  auto rules = _slp.GetRules();
  std::vector<int64_t> cover_sums;
  cover_sums.reserve(rules.size());

  auto sigma = _slp.Sigma();
  auto get_cover_sum = [&_slp, &_diff_base, &cover_sums, &sigma](auto _var) {
    return _slp.IsTerminal(_var) ? (_var - _diff_base) : (cover_sums[_var - sigma - 1]);
  };

  // Compute cover sum for each non-terminal
  for (std::size_t i = 0, j = sigma + 1; i < rules.size() / 2; ++i, ++j) {
    auto children = _slp[j];
    cover_sums.emplace_back(get_cover_sum(children.first) + get_cover_sum(children.second));
  }

  auto minmax = std::minmax_element(cover_sums.begin(), cover_sums.end());
  auto diff_base_sums = *minmax.first < 0 ? std::abs(*minmax.first) : 0;
  std::size_t i = sigma + 1;
  for (const auto &item : cover_sums) {
    _report_span_sum(i++, item + diff_base_sums);
  }

  return std::make_pair(*minmax.first, *minmax.second);
}

template<typename CompactSeq, typename SLP, typename GetSpanSum, typename ReportSample>
void ComputeSamplesOnCompactSequence(const CompactSeq &_compact_seq,
                                     const SLP &_slp,
                                     const GetSpanSum &_get_span_sum,
                                     std::size_t _max_span_length,
                                     const ReportSample &_report) {
  assert(0 < _compact_seq.size());

  std::size_t pos = 0;
  int64_t sum = 0;

  std::size_t i = 0;
  std::size_t root_span_length = _slp.SpanLength(_compact_seq[i]);

  while (i < _compact_seq.size()) {
    _report(pos, sum, i);

    std::size_t range_span_length = 0;
    do {
      range_span_length += root_span_length;
      sum += _get_span_sum(_compact_seq[i]);

      ++i;
    } while (i < _compact_seq.size()
        && range_span_length + (root_span_length = _slp.SpanLength(_compact_seq[i])) <= _max_span_length);

    pos += range_span_length;
  }
}

/*
template<typename SLP, typename CompactSeq, typename DifferentialBase, typename ReportCoverSum, typename ReportAccumulatedSum>
void ComputeCoverSumsAndSampleSum(const SLP &_slp,
                                  const CompactSeq &_compact_seq,
                                  const DifferentialBase &_diff_base,
                                  ReportCoverSum &_report_cover_sum,
                                  ReportAccumulatedSum &_report_acc_sum) {
  auto rules = _slp.GetRules();
  std::vector<int64_t> cover_sums(rules.size());

  auto sigma = _slp.Sigma();
  uint32_t diff_base = 0; // It is not considered for partial sums in each variable.
  auto get_cover_sum = [&_slp, &diff_base, &cover_sums, &sigma](auto _var) {
    return _slp.IsTerminal(_var) ? (_var - diff_base) : (cover_sums[_var - sigma - 1]
        - _slp.SpanLength(_var) * diff_base);
  };

  // Compute cover sum for each non-terminal
  for (std::size_t i = 0, j = sigma + 1; i < rules.size() / 2; ++i, ++j) {
    auto children = _slp[j];
    cover_sums[i] = get_cover_sum(children.first) + get_cover_sum(children.second);
    _report_cover_sum(i, cover_sums[i]);
  }

  // Compute accumulated sums on compact sequence
  uint32_t sum = 0;
  std::size_t pos = 0;
  diff_base = _diff_base; // Now, it is considered for accumulated sample sums.
  for (std::size_t i = 0; i < _compact_seq.size(); ++i) {
    _report_acc_sum(pos, sum);

    pos += _slp.SpanLength(_compact_seq[i]);
    sum += get_cover_sum(_compact_seq[i]);
  }
}
*/

template<typename SLP = grammar::SLP<>,
    typename Roots = std::vector<std::size_t>,
    typename SpanSums = std::vector<uint32_t>,
    typename Samples = std::vector<uint32_t>,
    typename SampleRootsPos = std::vector<uint32_t>,
    typename BitVector = sdsl::sd_vector<>,
    typename BitVectorRank = typename BitVector::rank_1_type,
    typename BitVectorSelect = typename BitVector::select_1_type>
class DifferentialSLPWrapper {
 public:
  DifferentialSLPWrapper(std::size_t _seq_size,
                         const SLP &_slp,
                         const Roots &_roots,
                         std::size_t _diff_base_seq,
                         const SpanSums &_span_sums,
                         std::size_t _diff_base_sums,
                         const Samples &_samples,
                         const SampleRootsPos &_sample_roots_pos,
                         const BitVector &_samples_pos,
                         const BitVectorRank &_samples_pos_rank,
                         const BitVectorSelect &_samples_pos_select)
      : seq_size_{_seq_size},
        slp_{_slp},
        roots_{_roots},
        diff_base_seq_{_diff_base_seq},
        span_sums_{_span_sums},
        diff_base_sums_{_diff_base_sums},
        samples_{_samples},
        sample_roots_pos_{_sample_roots_pos},
        samples_pos_{_samples_pos},
        samples_pos_rank_{_samples_pos_rank},
        samples_pos_select_{_samples_pos_select} {
  }

  auto IsTerminal(std::size_t _var) const {
    return slp_.IsTerminal(_var);
  }

  auto operator[](std::size_t _var) const {
    return slp_[_var];
  }

  auto SpanLength(std::size_t _var) const {
    return slp_.SpanLength(_var);
  }

  auto Root(std::size_t _i) const {
    assert(_i < roots_.size());
    return roots_[_i];
  }

  auto SpanSum(std::size_t _var) const {
    return slp_.IsTerminal(_var) ? (_var - diff_base_seq_) : (span_sums_[_var - slp_.Sigma() - 1] - diff_base_sums_);
  }

  auto DifferentialBase() const {
    return diff_base_seq_;
  }

  auto Sample(std::size_t _pos) const {
    return samples_pos_rank_(_pos + 1);
  }

  auto SamplePosition(std::size_t _sample) const {
    return samples_pos_select_(_sample);
  }

  auto SampleFirstRoot(std::size_t _sample) const {
    return sample_roots_pos_[_sample - 1];
  }

  auto SampleValue(std::size_t _sample) const {
    assert(0 < _sample);
    return samples_[_sample - 1];
  }

 private:
  std::size_t seq_size_;

  const SLP &slp_;
  const Roots &roots_; // Compact sequence. It is a sequence of variables (terminals/non-terminals), i.e. a forest.

  std::size_t diff_base_seq_; // Differential base added to differential sequence.

  const SpanSums &span_sums_; // For each non-terminal.
  std::size_t diff_base_sums_; // Differential base added to span sums.

  const Samples &samples_; // Values at sampled positions.
  const SampleRootsPos &sample_roots_pos_; // Position of the first root for each sample interval.
  const BitVector &samples_pos_; // Marks sampled positions in the original sequence.
  const BitVectorRank &samples_pos_rank_;
  const BitVectorSelect &samples_pos_select_;
};

template<typename SLP, typename Roots, typename SpanSums, typename Samples, typename SampleRootsPos, typename BitVector, typename BitVectorRank, typename BitVectorSelect>
auto MakeDifferentialSLPWrapper(std::size_t _seq_size,
                                const SLP &_slp,
                                const Roots &_roots,
                                std::size_t _diff_base_seq,
                                const SpanSums &_span_sums,
                                std::size_t _diff_base_sums,
                                const Samples &_samples,
                                const SampleRootsPos &_sample_roots_pos,
                                const BitVector &_samples_pos,
                                const BitVectorRank &_samples_pos_rank,
                                const BitVectorSelect &_samples_pos_select) {
  return DifferentialSLPWrapper(_seq_size,
                                _slp,
                                _roots,
                                _diff_base_seq,
                                _span_sums,
                                _diff_base_sums,
                                _samples,
                                _sample_roots_pos,
                                _samples_pos,
                                _samples_pos_rank,
                                _samples_pos_select);
}

/*
template<typename SLP = grammar::SLP<>,
    typename Roots = std::vector<std::size_t>,
    typename SpanSums = std::vector<uint32_t>,
    typename SampleSums = std::vector<uint32_t>,
    typename BitVector = sdsl::sd_vector<>,
    typename BitVectorRank = typename BitVector::rank_1_type,
    typename BitVectorSelect = typename BitVector::select_1_type>
class DifferentialSLP : public SLP {
 public:
  using size_type = std::size_t;

  DifferentialSLP() = default;

  template<typename OriginalSLP, typename CompactSeq, typename SpanSumsAction = NoAction, typename SampleSumsAction = NoAction, typename RootsAction = NoAction>
  void Compute(std::size_t _seq_size,
               const OriginalSLP &_slp,
               const CompactSeq &_compact_seq,
               uint64_t _differential_base,
               SpanSumsAction &&_span_sums_action = NoAction(),
               SampleSumsAction &&_sample_sums_action = NoAction(),
               RootsAction &&_roots_action = NoAction()) {

    differential_base_ = _differential_base;

    std::vector<typename SpanSums::value_type> span_sums;
    span_sums.reserve(_slp.GetRules().size());
    auto report_cover_sum = [&span_sums](auto _rule, auto _sum) {
      span_sums.emplace_back(_sum);
    };

    sdsl::bit_vector tmp_sample_pos(_seq_size + 1, 0);
    std::vector<typename SampleSums::value_type> sample_sums;
    auto report_acc_sum = [&tmp_sample_pos, &sample_sums](auto _pos, auto _sum) {
      tmp_sample_pos[_pos] = 1;
      sample_sums.emplace_back(_sum);
    };

    ComputeCoverSumsAndSampleSum(_slp, _compact_seq, _differential_base, report_cover_sum, report_acc_sum);
    tmp_sample_pos[tmp_sample_pos.size() - 1] = 1;

    OriginalSLP::operator=(_slp);

//    span_sums_ = SpanSums(span_sums);
    Construct(span_sums_, span_sums);
    _span_sums_action(span_sums_);
    span_sums.clear();

//    sample_sums_ = SampleSums(sample_sums);
    Construct(sample_sums_, sample_sums);
    _sample_sums_action(sample_sums_);
    sample_sums.clear();

    sample_pos_ = BitVector(tmp_sample_pos);
    sample_pos_rank_ = BitVectorRank(&sample_pos_);
    sample_pos_select_ = BitVectorSelect(&sample_pos_);

//    roots_ = Roots(_compact_seq);
    Construct(roots_, _compact_seq);
    _roots_action(roots_);

  }

  auto DifferentialBase() const {
    return differential_base_;
  }

  auto SpanSum(std::size_t _var) const {
    return SLP::IsTerminal(_var) ? _var : span_sums_[_var - SLP::sigma_ - 1];
  }

  auto Root(std::size_t _i) const {
    assert(_i < roots_.size());
    return roots_[_i];
  }

  std::size_t Sample(std::size_t _pos) const {
    return sample_pos_rank_(_pos + 1);
  }

  auto SamplePosition(std::size_t _sample) const {
    return sample_pos_select_(_sample);
  }

  auto SampleFirstRoot(std::size_t _sample) const {
    return _sample - 1;
  }

  auto SampleSum(std::size_t _sample) const {
    assert(0 < _sample);
    return sample_sums_[_sample - 1];
  }

  std::size_t serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, const std::string &name = "") const {
    std::size_t written_bytes = 0;
    written_bytes += SLP::serialize(out);

    written_bytes += sdsl::serialize(roots_, out);

    written_bytes += sdsl::serialize(differential_base_, out);
    written_bytes += sdsl::serialize(span_sums_, out);

    written_bytes += sample_pos_.serialize(out);
    written_bytes += sample_pos_rank_.serialize(out);
    written_bytes += sample_pos_select_.serialize(out);
    written_bytes += sdsl::serialize(sample_sums_, out);

    return written_bytes;
  }

  void load(std::istream &in) {
    SLP::load(in);

    sdsl::load(roots_, in);

    sdsl::load(differential_base_, in);
    sdsl::load(span_sums_, in);

    sample_pos_.load(in);
//    sample_pos_rank_.load(in);
    sample_pos_rank_ = BitVectorRank(&sample_pos_);
//    sample_pos_select_.load(in);
    sample_pos_select_ = BitVectorSelect(&sample_pos_);
    sdsl::load(sample_sums_, in);
  }

 private:
  Roots roots_; // Compact sequence. It is a sequence of variables (terminals/non-terminals), i.e. a forest.

  uint64_t differential_base_;
  SpanSums span_sums_; // For each non-terminal.

  BitVector sample_pos_; // Marks sampled positions in the original sequence.
  BitVectorRank sample_pos_rank_;
  BitVectorSelect sample_pos_select_;
  SampleSums sample_sums_;
};
 */


template<typename DiffSLP, typename Report>
void ExpandSLPFromLeft(const DiffSLP &_slp, std::size_t _var, std::size_t &_length, const Report &_report) {
  assert(0 < _length);

  if (_slp.IsTerminal(_var)) {
    _report(_var);
    --_length;
    return;
  }

  const auto &children = _slp[_var];
  ExpandSLPFromLeft(_slp, children.first, _length, _report);

  if (0 < _length) {
    ExpandSLPFromLeft(_slp, children.second, _length, _report);
  }
}

template<typename DiffSLP, typename Report, typename Skip>
void ExpandSLPFromLeft(const DiffSLP &_slp,
                       std::size_t _var,
                       std::size_t &_sp,
                       std::size_t &_length,
                       const Report &_report,
                       const Skip &_skip) {
  assert(0 < _sp);
  assert(0 < _length);

  const auto &children = _slp[_var];
  auto left_child_len = _slp.SpanLength(children.first);
  if (_sp < left_child_len) {
    ExpandSLPFromLeft(_slp, children.first, _sp, _length, _report, _skip);
  } else {
    _skip(children.first);
    _sp -= left_child_len;
  }

  if (0 < _sp) {
    ExpandSLPFromLeft(_slp, children.second, _sp, _length, _report, _skip);
  } else if (0 < _length) {
    ExpandSLPFromLeft(_slp, children.second, _length, _report);
  }
}

template<typename DiffSLP, typename Report, typename Skip>
void ExpandSLPFromFront(const DiffSLP &_slp,
                        std::size_t _idx_root,
                        std::size_t _sp,
                        std::size_t _length,
                        const Report &_report,
                        const Skip &_skip) {
  assert(0 < _length);

  std::size_t root;
  std::size_t span_length;
  while (root = _slp.Root(_idx_root), (span_length = _slp.SpanLength(root)) <= _sp) {
    _skip(root);

    _sp -= span_length;
    ++_idx_root;
  }

  if (0 < _sp) {
    ExpandSLPFromLeft(_slp, root, _sp, _length, _report, _skip);
    ++_idx_root;
  }

  while (0 < _length) {
    ExpandSLPFromLeft(_slp, _slp.Root(_idx_root), _length, _report);
    ++_idx_root;
  }
}

template<typename DiffSLP, typename Report>
void ExpandDifferentialSLP(const DiffSLP &_slp, std::size_t _sp, std::size_t _ep, const Report &_report) {
  assert(_sp <= _ep);

  auto sample = _slp.Sample(_sp);
  auto idx_root = _slp.SampleFirstRoot(sample);
  auto pos = _slp.SamplePosition(sample);
  auto sp = _sp - pos;
  auto len = _ep - _sp + 1;

  auto sum = _slp.SampleValue(sample);
  auto diff_base = _slp.DifferentialBase();
  auto report = [&_report, &sum, &diff_base](const auto &_terminal) {
    sum += _terminal - diff_base;
    _report(sum);
  };

  auto skip = [&sum, &_slp, &diff_base](const auto _var) {
    sum += _slp.SpanSum(_var);
  };

  ExpandSLPFromFront(_slp, idx_root, sp, len, report, skip);
}

} // namespace grammar
#endif //GRAMMAR_DIFFERENTIAL_SLP_H_
