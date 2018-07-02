//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 23-06-18.
//

#ifndef GRAMMAR_RE_PAIR_H
#define GRAMMAR_RE_PAIR_H

#include <cstddef>
#include <memory>
#include <functional>
#include <algorithm>

#include "complete_tree.h"


namespace grammar {

class RePairBasicEncoder;

template<bool kChomskyNormalForm>
class RePairEncoder;


/**
 * Basic RePair Encoder
 */
class RePairBasicEncoder {
 public:
  /**
   * Takes a sequence of integers and build a grammar to represent it using RePair algorithm. The rules are reported
   * using _report_rules.
   *
   * @tparam II Input iterator
   * @tparam ReportRule Rules reporter
   * @tparam HandleCSeq Final compact sequence handler
   *
   * @param _begin
   * @param _end
   * @param _report_rule
   * @param _handle_c_seq
   */
  template<typename II, typename ReportRule, typename HandleCSeq>
  void Encode(II _begin, II _end, ReportRule &_report_rule, HandleCSeq &_handle_c_seq) {
    auto length = std::distance(_begin, _end);
    auto C = (int *) malloc(length * sizeof(int));
    {
      auto it = _begin;
      for (int i = 0; i < length; ++i, ++it) {
        C[i] = *it;
      }
    }

    auto reporter = [&_report_rule](int left, int right, int length) -> void {
      _report_rule(left, right, length);
    };

    prepare(C, length);
    length = repair(C, length, reporter);

    _handle_c_seq(C, length);

    destroy();
    free(C);
  }

 protected:
  std::vector<int> rules_span_length_;
  std::vector<int> rules_height_;

  void prepare(int [], std::size_t);
  int repair(int [], std::size_t, std::function<void(int, int, int)>);
  void destroy();

  int get_rule_span_length(int _rule);
  int get_rule_height(int _rule);

 private:
  struct InternalData;

  std::shared_ptr<InternalData> data_;
};


/**
 * RePair Encoder
 *
 * Returns grammar rules and final compact sequence. The rules represent any pair of symbols that appear at least twice.
 */
template<>
class RePairEncoder<false> : public RePairBasicEncoder {
 public:
  /**
   * Takes a sequence of integers and build a grammar to represent it using RePair algorithm. The rules are reported
   * using _report_rules. The final compact sequence is reported using _report_c_seq.
   *
   * @tparam II Input iterator
   * @tparam ReportRule Rules reporter
   * @tparam ReportCSeq Final compact sequence reporter
   *
   * @param _begin
   * @param _end
   * @param _report_rule
   * @param _report_c_seq
   */
  template<typename II, typename ReportRule, typename ReportCSeq>
  void Encode(II _begin, II _end, ReportRule &_report_rule, ReportCSeq &_report_c_seq) {
    auto handler = [&_report_c_seq](int C[], std::size_t length) {
      for (int i = 0; i < length; ++i) {
        _report_c_seq(C[i]);
      }
    };

    RePairBasicEncoder::Encode(_begin, _end, _report_rule, handler);
  }
};


/**
 * RePair Encoder
 *
 * Return grammar rules. The grammar is in Chomsky Normal Form.
 */
template<>
class RePairEncoder<true> : public RePairBasicEncoder {
 public:
  /**
   * Takes a sequence of integers and build a grammar to represent it using RePair algorithm. The rules are reported
   * using _report_rules. The grammar is in Chomsky Normal Form.
   *
   * @tparam II Input iterator
   * @tparam ReportRule Rules reporter
   * @tparam CompleteTree Complete grammar tree heuristic
   *
   * @param _begin
   * @param _end
   * @param _report_rule
   * @param _complete
   */
  template<typename II, typename ReportRule, typename CompleteTree = BalanceTreeByWeight>
  void Encode(II _begin, II _end, ReportRule &_report_rule, const CompleteTree &_complete = BalanceTreeByWeight()) {
    auto handler = [&_complete, &_report_rule, this](int C[], std::size_t length) {
      auto max = *std::max_element(C, C + length);

      auto
          report = [&C, &length, max, &_report_rule, this](int id, int id_left_child, int id_right_child, auto height) {
        auto left = (id_left_child < length) ? C[id_left_child] : id_left_child - length + max + 1;
        auto right = (id_right_child < length) ? C[id_right_child] : id_right_child - length + max + 1;

        int rule_span_length = this->get_rule_span_length(left) + this->get_rule_span_length(right);

        _report_rule(left, right, rule_span_length);
        rules_span_length_.push_back(rule_span_length);
        rules_height_.push_back(std::max(this->get_rule_height(left), this->get_rule_height(right)) + 1);
      };

      auto get_height = [this](int rule) -> auto {
        return this->get_rule_height(rule);
      };

      _complete(C, C + length, report, get_height);
    };

    RePairBasicEncoder::Encode(_begin, _end, _report_rule, handler);
  }
};

}

#endif //GRAMMAR_RE_PAIR_H