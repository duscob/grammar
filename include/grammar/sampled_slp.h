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
#include "slp_metadata.h"
#include "slp_partition.h"


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
    std::tie(l, b_l, b_f, f, n) = PartitionSLP(_slp, _block_size, _pred, _leaf_action, _node_action);

    b_l_rank = _BVLeafNodesMarksRank(&b_l);
    b_l_select = _BVLeafNodesMarksSelect(&b_l);

    b_f_rank = _BVFirstChildrenRank(&b_f);
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

  std::size_t serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, const std::string &name = "") const {
    std::size_t written_bytes = 0;
    written_bytes += sdsl::serialize(l, out);
    written_bytes += b_l.serialize(out);
    written_bytes += b_l_rank.serialize(out);
    written_bytes += b_l_select.serialize(out);
    written_bytes += b_f.serialize(out);
    written_bytes += b_f_rank.serialize(out);
    written_bytes += sdsl::serialize(f, out);
    written_bytes += sdsl::serialize(n, out);

    return written_bytes;
  }

  void load(std::istream &in) {
    sdsl::load(l, in);

    b_l.load(in);
    b_l_rank.load(in, &b_l);
    b_l_select.load(in, &b_l);

    b_f.load(in);
    b_f_rank.load(in, &b_f);

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
  using typename _SLP::size_type;

  CombinedSLP() = default;

  CombinedSLP(const _SLP &_slp) : _SLP(_slp) {}

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

  const auto &GetLeaves() const {
    return leaves_;
  }

  bool operator==(const CombinedSLP &_combined_slp) const {
    return _SLP::operator==(_combined_slp)
        && _SampledSLP::operator==(_combined_slp)
        && leaves_ == _combined_slp.leaves_;
  }

  bool operator!=(const CombinedSLP &_combined_slp) const {
    return !(*this == _combined_slp);
  }

  std::size_t serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, const std::string &name = "") const {
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


template<typename _SLP = grammar::SLP<>,
    typename _SampledSLP = grammar::SampledSLP<>,
    typename _Chunks = grammar::Chunks<>>
class LightSLP : public _SLP, public _SampledSLP {
 public:
  using typename _SLP::size_type;

  LightSLP() = default;

  template<typename __SLP,
      typename __SampledSLP,
      typename __Chunks,
      typename __SLPAct1 = NoAction,
      typename __SLPAct2 = NoAction,
      typename __ChunksAct1 = NoAction,
      typename __ChunksAct2 = NoAction>
  LightSLP(const LightSLP<__SLP, __SampledSLP, __Chunks> &_lslp,
           __SLPAct1 &&_slp_act1 = NoAction(),
           __SLPAct2 &&_slp_act2 = NoAction(),
           __ChunksAct1 &&_chunks_act1 = NoAction(),
           __ChunksAct2 &&_chunks_act2 = NoAction()): _SLP(_lslp, _slp_act1, _slp_act2),
                                                      _SampledSLP(_lslp),
                                                      covers_(_lslp.GetCovers(), _chunks_act1, _chunks_act2) {}

  template<typename _II, typename _Encoder, typename _CombinedSLP>
  void Compute(_II _first, _II _last, _Encoder _encoder, const _CombinedSLP &_cslp) {
    std::vector<typename _SLP::VariableType> cseq;

    auto report_cseq = [&cseq](auto v) {
      cseq.emplace_back(v);
    };

    auto wrapper = BuildSLPWrapper(*this);
    _encoder.Encode(_first, _last, wrapper, report_cseq);

    const auto &leaves = _cslp.GetLeaves();

    auto get_length = [&_cslp](const auto &_leaf) -> auto {
      return _cslp.SpanLength(_leaf);
    };

    auto action = [this](auto &_set) {
      covers_.Insert(_set.begin(), _set.end());
    };

    ComputePartitionCover(*this, cseq, leaves, get_length, action, 0);

    _SampledSLP::operator=(_cslp);
  }

  template<typename _CSeq, typename _CombinedSLP>
  void Compute(const _SLP &_slp, const _CSeq &_cseq, const _CombinedSLP &_cslp) {
    _SLP::operator=(_slp);

    const auto &leaves = _cslp.GetLeaves();

    auto get_length = [&_cslp](const auto &_leaf) -> auto {
      return _cslp.SpanLength(_leaf);
    };

    auto action = [this](auto &_set) {
      covers_.Insert(_set.begin(), _set.end());
    };

    ComputePartitionCover(*this, _cseq, leaves, get_length, action, 0);

    _SampledSLP::operator=(_cslp);
  }

  auto Cover(std::size_t _leaf) const {
    return covers_[_leaf];
  }

  const auto &GetCovers() const {
    return covers_;
  }

  template<typename __SLP, typename __SampledSLP, typename __Chunks>
  bool operator==(const LightSLP<__SLP, __SampledSLP, __Chunks> &_lslp) const {
    return _SLP::operator==(_lslp)
        && _SampledSLP::operator==(_lslp)
        && covers_ == _lslp.GetCovers();
  }

  template<typename __SLP, typename __SampledSLP, typename __Chunks>
  bool operator!=(const LightSLP<__SLP, __SampledSLP, __Chunks> &_lslp) const {
    return !(*this == _lslp);
  }

  std::size_t serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, const std::string &name = "") const {
    std::size_t written_bytes = 0;
    written_bytes += _SLP::serialize(out);
    written_bytes += _SampledSLP::serialize(out);
    written_bytes += sdsl::serialize(covers_, out);

    return written_bytes;
  }

  void load(std::istream &in) {
    _SLP::load(in);
    _SampledSLP::load(in);
    sdsl::load(covers_, in);
  }

 protected:
  _Chunks covers_;
};

}

#endif //GRAMMAR_SAMPLED_SLP_H
