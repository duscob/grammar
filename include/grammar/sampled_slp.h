//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/26/18.
//

#ifndef GRAMMAR_SAMPLED_SLP_H
#define GRAMMAR_SAMPLED_SLP_H

#include <cstdint>
#include <vector>
#include <iterator>

#include <sdsl/bit_vectors.hpp>
#include <sdsl/vlc_vector.hpp>

#include "slp_helper.h"
#include "slp.h"


namespace grammar {

template<typename _PTS>
class AddSet {
 public:
  AddSet(_PTS &_pts) : pts_(_pts) {}

  template<typename _SLP>
  void operator()(const _SLP &_slp, std::size_t _curr_var) {
    auto set = _slp.Span(_curr_var);
    sort(set.begin(), set.end());
    set.erase(unique(set.begin(), set.end()), set.end());

    pts_.Insert(set.begin(), set.end());
  }

  template<typename _SLP, typename _Nodes, typename _RangeContainer>
  void operator()(const _SLP &_slp,
                  std::size_t _curr_var,
                  const _Nodes &_nodes,
                  std::size_t _new_node,
                  const _RangeContainer &_left_ranges,
                  const _RangeContainer &_right_ranges) {
    (*this)(_slp, _curr_var);
  }

 private:
  _PTS &pts_;
};


template<typename _BV>
void Construct(_BV &_bv, const std::vector<bool> &_tmp_bv) {
  sdsl::bit_vector bv(_tmp_bv.size());
  std::copy(_tmp_bv.begin(), _tmp_bv.end(), bv.begin());

  _bv = _BV(bv);
}


template<typename _V>
void Construct(_V &_v, const std::map<std::size_t, std::size_t> &_tmp_v) {
  std::vector<std::size_t> v(0);
  v.reserve(_tmp_v.size());
  for (const auto &item : _tmp_v) {
    v.emplace_back(item.second);
  }

  _v = _V(v);
}


template<typename _BVLeafNodesMarks = sdsl::sd_vector<>,
    typename _BVLeafNodesMarksRank = typename _BVLeafNodesMarks::rank_1_type,
    typename _BVLeafNodesMarksSelect = typename _BVLeafNodesMarks::select_1_type,
    typename _BVFirstChildren = sdsl::sd_vector<>,
    typename _BVFirstChildrenRank = typename _BVFirstChildren::rank_1_type,
    typename _Parents = sdsl::vlc_vector<>,
    typename _NextLeaves = sdsl::vlc_vector<>>
class SampledSLP {
 public:
  typedef std::size_t size_type;

  SampledSLP() = default;

  template<typename _SLP, typename _LeafAction, typename _NodeAction, typename _Predicate>
  SampledSLP(const _SLP &_slp,
             uint32_t _block_size,
             _LeafAction &&_leaf_action,
             _NodeAction &&_node_action,
             const _Predicate &_pred) {
    Compute(_slp, _block_size, _leaf_action, _node_action, _pred);
  }

  template<typename _SLP, typename _LeafAction, typename _NodeAction, typename _Predicate>
  void Compute(const _SLP &_slp,
               uint32_t _block_size,
               _LeafAction &&_leaf_action,
               _NodeAction &&_node_action,
               const _Predicate &_pred) {
    std::vector<typename _SLP::VariableType> nodes;

    ComputeSampledSLPLeaves(_slp, _block_size, back_inserter(nodes), _leaf_action);

    l = nodes.size();

    {
      sdsl::bit_vector tmp_b_l(_slp.SpanLength(_slp.Start()) + 1, 0);
      std::size_t pos = 0;
      for (const auto &item : nodes) {
        tmp_b_l[pos] = 1;
        pos += _slp.SpanLength(item);
      }
      tmp_b_l[tmp_b_l.size() - 1] = 1;
      b_l = _BVLeafNodesMarks(tmp_b_l);
      b_l_rank = _BVLeafNodesMarksRank(&b_l);
      b_l_select = _BVLeafNodesMarksSelect(&b_l);
    }

    std::vector<bool> tmp_b_f(nodes.size(), 0);
    std::map<std::size_t, std::size_t> tmp_f;
    std::map<std::size_t, std::size_t> tmp_n;

    auto build_inner_data = [&_node_action, &tmp_b_f, &tmp_f, &tmp_n, this](
        const _SLP &_slp,
        std::size_t _curr_var,
        const auto &_nodes,
        std::size_t _new_node,
        const auto &_left_ranges,
        const auto &_right_ranges) {
      _node_action(_slp, _curr_var, _nodes, _new_node, _left_ranges, _right_ranges);

      tmp_b_f.emplace_back(0);
      tmp_b_f[_left_ranges.front().first] = 1;

      auto nn = _new_node - l;
      tmp_f[_left_ranges.front().first] = nn;
      auto last_child = _right_ranges.back().first + _right_ranges.back().second;
      tmp_n[nn] = (last_child <= l) ? last_child : tmp_n[last_child - l - 1];
    };

    ComputeSampledSLPNodes(_slp, _block_size, nodes, _pred, build_inner_data);

    Construct(b_f, tmp_b_f);
    b_f_rank = _BVFirstChildrenRank(&b_f);

    Construct(f, tmp_f);
    Construct(n, tmp_n);
  }

  std::size_t Leaf(std::size_t _pos) const {
    return b_l_rank(_pos + 1);
  }

  std::size_t Position(std::size_t _leaf) const {
    return b_l_select(_leaf);
  }

  std::pair<std::size_t, std::size_t> Parent(std::size_t _i) const {
    auto p = f[b_f_rank(_i) - 1];
    return {l + p + 1, n[p] + 1};
  }

  bool IsFirstChild(std::size_t _i) const {
    return b_f[_i - 1];
  }

  bool operator==(const SampledSLP<_BVLeafNodesMarks,
                                   _BVLeafNodesMarksRank,
                                   _BVLeafNodesMarksSelect,
                                   _BVFirstChildren,
                                   _BVFirstChildrenRank,
                                   _Parents,
                                   _NextLeaves> &_sampled_slp) const {
    return l == _sampled_slp.l &&
        b_l.size() == _sampled_slp.b_l.size() && std::equal(b_l.begin(), b_l.end(), _sampled_slp.b_l.begin()) &&
        b_f.size() == _sampled_slp.b_f.size() && std::equal(b_f.begin(), b_f.end(), _sampled_slp.b_f.begin()) &&
        f.size() == _sampled_slp.f.size() && std::equal(f.begin(), f.end(), _sampled_slp.f.begin()) &&
        n.size() == _sampled_slp.n.size() && std::equal(n.begin(), n.end(), _sampled_slp.n.begin());
  }

  bool operator!=(const SampledSLP<_BVLeafNodesMarks,
                                   _BVLeafNodesMarksRank,
                                   _BVLeafNodesMarksSelect,
                                   _BVFirstChildren,
                                   _BVFirstChildrenRank,
                                   _Parents,
                                   _NextLeaves> &_sampled_slp) const {
    return !(*this == _sampled_slp);
  }

  std::size_t serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
    std::size_t written_bytes = 0;
    written_bytes += sdsl::serialize(l, out);
    written_bytes += b_l.serialize(out);
    written_bytes += b_f.serialize(out);
    written_bytes += sdsl::serialize(f, out);
    written_bytes += sdsl::serialize(n, out);

    return written_bytes;
  }

  void load(std::istream &in) {
    sdsl::load(l, in);

    b_l.load(in);
    b_l_rank = _BVLeafNodesMarksRank(&b_l);
    b_l_select = _BVLeafNodesMarksSelect(&b_l);

    b_f.load(in);
    b_f_rank = _BVFirstChildrenRank(&b_f);

    sdsl::load(f, in);
    sdsl::load(n, in);
  }

 protected:
  std::size_t l = 0;
  _BVLeafNodesMarks b_l;
  _BVLeafNodesMarksRank b_l_rank;
  _BVLeafNodesMarksSelect b_l_select;
  _BVFirstChildren b_f;
  _BVFirstChildrenRank b_f_rank;
  _Parents f;
  _NextLeaves n;
};


template<typename _SLP = grammar::SLP<>,
    typename _SampledSLP = grammar::SampledSLP<>,
    typename _LeavesContainer = std::vector<typename _SLP::VariableType>>
class CombinedSLP : public _SLP, public _SampledSLP {
 public:
  using _SLP::size_type;

  CombinedSLP() = default;

  template<typename _LeafAction, typename _NodeAction, typename _Predicate>
  void Compute(uint32_t _block_size, _LeafAction &&_leaf_action, _NodeAction &&_node_action, const _Predicate &_pred) {
    auto leaf_action = [this, &_leaf_action](const auto &_slp, auto _curr_var) {
      _leaf_action(_slp, _curr_var);

      leaves_.push_back(_curr_var);
    };

    _SampledSLP::Compute(*this, _block_size, leaf_action, _node_action, _pred);
  }

  auto Map(std::size_t _leaf) const {
    return leaves_[_leaf - 1];
  }

  bool operator==(const CombinedSLP &_combined_slp) const {
    return _SLP::operator==(_combined_slp)
        && _SampledSLP::operator==(_combined_slp)
        && leaves_ == _combined_slp.leaves_;
  }

  bool operator!=(const CombinedSLP &_combined_slp) const {
    return !(*this == _combined_slp);
  }

  std::size_t serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
    std::size_t written_bytes = 0;
    written_bytes += _SLP::serialize(out);
    written_bytes += _SampledSLP::serialize(out);
    written_bytes += sdsl::serialize(leaves_, out);

    return written_bytes;
  }

  void load(std::istream &in) {
    _SLP::load(in);
    _SampledSLP::load(in);
    sdsl::load(leaves_, in);
  }

 protected:
  _LeavesContainer leaves_;
};


template<typename _SLP, typename _SampledSLP, typename _Chunks>
class LightSLP {
 public:
  template<typename _II, typename _Encoder, typename _CombinedSLP>
  void Compute(_II _first, _II _last, _Encoder _encoder, const _CombinedSLP &_cslp) {
    std::vector<typename _SLP::VariableType> cseq;

    auto report_cseq = [&cseq](auto v) {
      cseq.emplace_back(v);
    };

    auto wrapper = BuildSLPWrapper(slp_);
    _encoder.Encode(_first, _last, wrapper, report_cseq);

    std::vector<typename _SLP::VariableType> set;

    const auto &leaves = _cslp.GetLeaves();

    std::size_t pos = 0;
    std::size_t inner_pos = 0;
    for (int i = 0; i < leaves.size(); ++i, chunks_.Insert(set.begin(), set.end())) {

      set.clear();

      auto size = _cslp.SpanLength(leaves[i]);

      if (inner_pos != 0) {
        // Find inner variables
        grammar::ComputeSpanCover(slp_, inner_pos, inner_pos + size, back_inserter(set), cseq[pos]);

        auto rest = slp_.SpanLength(cseq[pos]) - inner_pos;
        if (rest <= size) {
          // Get to the end of current variable's expansion
          size -= rest;
          ++pos;
        } else {
          // Still remain elements in the current variable's expansion
          inner_pos += size;
          continue;
        }
      }

      std::size_t var_len;
      while (pos < cseq.size() && (var_len = slp_.SpanLength(cseq[pos])) <= size) {
        // Add complete covered variables
        set.emplace_back(cseq[pos]);

        size -= var_len;
        ++pos;
      }

      if (0 < size) {
        // Find inner variables
        grammar::ComputeSpanCoverEnding(slp_, size, back_inserter(set), cseq[pos]);
      }

      inner_pos = size;
    }

    sampled_slp_ = _SampledSLP(_cslp);
  }

 protected:
  _SLP slp_;
  _SampledSLP sampled_slp_;
  _Chunks chunks_;
};

}

#endif //GRAMMAR_SAMPLED_SLP_H
