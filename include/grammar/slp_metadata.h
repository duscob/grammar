//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/17/18.
//

#ifndef GRAMMAR_SLP_METADATA_H
#define GRAMMAR_SLP_METADATA_H

#include <cstdint>
#include <vector>
#include <map>
#include <algorithm>
#include <type_traits>
#include <cassert>

#include "slp_helper.h"
#include "utility.h"
#include "io.h"


namespace grammar {

/**
 * Precomputed Terminal Set
 */
template<typename _Container = std::vector<uint32_t>>
class PTS {
 public:
  typedef std::size_t size_type;

  PTS() = default;

  /**
   * Construct the set of terminals for each variable
   *
   * @tparam _SLP Straight-Line Program (Grammar)
   * @param slp
   */
  template<typename _SLP>
  PTS(const _SLP *slp) {
    Compute(slp);
  }

  /**
   * Compute the set of terminals for each variable
   *
   * @tparam _SLP Straight-Line Program (Grammar)
   * @param slp
   */
  template<typename _SLP>
  void Compute(const _SLP *slp) {
    sets_.reserve(slp->Variables() + 1);

    std::vector<typename _Container::value_type> set(0);
    sets_.emplace_back(set);

    std::size_t i = 1;
    set.reserve(1);
    set.push_back(0);
    for (; i <= slp->Sigma(); ++i) {
      set[0] = i;
      sets_.emplace_back(set);
    }

    set.reserve(slp->Sigma());
    for (; i <= slp->Variables(); ++i) {
      const auto &right_hand = (*slp)[i];
      const auto &left = sets_[right_hand.first];
      const auto &right = sets_[right_hand.second];

      set.clear();
      set_union(left.begin(), left.end(), right.begin(), right.end(), back_inserter(set));
      sets_.emplace_back(set);
    }
  }

  /**
   * Get the set of terminal of variable i
   *
   * @param i variable
   * @return set of terminal
   */
  const _Container &operator[](std::size_t i) const {
    return sets_.at(i);
  }

  bool operator==(const PTS<_Container> &_pts) const {
    if (sets_.size() != _pts.sets_.size())
      return false;

    bool equal = false;
    for (int i = 0; i < sets_.size(); ++i) {
      if (sets_[i].size() != _pts.sets_[i].size()
          || !std::equal(sets_[i].begin(), sets_[i].end(), _pts.sets_[i].begin()))
        return false;
    }

    return true;
  }

  bool operator!=(const PTS<_Container> &_pts) const {
    return !(*this == _pts);
  }

  std::size_t serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
    std::size_t written_bytes = 0;
    written_bytes += sdsl::serialize(sets_, out);

    return written_bytes;
  }

  void load(std::istream &in) {
    sdsl::load(sets_, in);
  }

 protected:
  std::vector<_Container> sets_;
};


template<typename _ObjContainer = std::vector<uint32_t>, typename _PosContainer = std::vector<uint32_t>>
class Chunks {
 public:
  typedef std::size_t size_type;

  Chunks() = default;

  template<typename __ObjContainer, typename __PosContainer, typename _ActionObj = NoAction, typename _ActionPos = NoAction>
  Chunks(const Chunks<__ObjContainer, __PosContainer> &_chunks,
         _ActionObj _action_obj = NoAction(),
         _ActionPos _action_pos = NoAction()) {
    Construct(objs_, _chunks.GetObjects());
    _action_obj(objs_);
    Construct(pos_, _chunks.GetChunksPositions());
    _action_pos(pos_);
  }

  auto size() const {
    return pos_.size();
  }

  typedef std::pair<typename _ObjContainer::const_iterator, typename _ObjContainer::const_iterator> Chunk;

  class FakeContainer : public Chunk {
   public:
    FakeContainer(Chunk &&_chunk) : Chunk(_chunk) {}

    auto begin() const {
      return Chunk::first;
    }

    auto end() const {
      return Chunk::second;
    }

    auto size() const {
      return end() - begin();
    }
  };

  FakeContainer operator[](std::size_t i) const {
    assert(i <= pos_.size());

    return std::make_pair(objs_.begin() + pos_[i - 1], (i < pos_.size()) ? objs_.begin() + pos_[i] : objs_.end());
  }

  const _ObjContainer &GetObjects() const {
    return objs_;
  }

  const _PosContainer &GetChunksPositions() const {
    return pos_;
  }

  template<typename _Value>
  auto Insert(_Value _value)
  -> typename std::enable_if<
      std::integral_constant<
          bool,
          has_push_back<_PosContainer, void(typename _ObjContainer::size_type)>::value
              && has_push_back<_ObjContainer, void(_Value)>::value
      >::value,
      typename _PosContainer::size_type
  >::type {
    pos_.push_back(objs_.size());
    objs_.push_back(_value);

    return pos_.size();
  }

  template<typename _II>
  auto Insert(_II _first, _II _last)
  -> typename std::enable_if<
      std::integral_constant<
          bool,
          has_push_back<_PosContainer, void(typename _ObjContainer::size_type)>::value
              && has_push_back<_ObjContainer, void(decltype(*_first))>::value
      >::value,
      typename _PosContainer::size_type
  >::type {

    pos_.push_back(objs_.size());
    std::copy(_first, _last, back_inserter(objs_));

    return pos_.size();
  }

  template<typename __ObjContainer, typename __PosContainer>
  bool operator==(const Chunks<__ObjContainer, __PosContainer> &_chunks) const {
    return objs_.size() == _chunks.objs_.size()
        && std::equal(objs_.begin(), objs_.end(), _chunks.objs_.begin())
        && pos_.size() == _chunks.pos_.size()
        && std::equal(pos_.begin(), pos_.end(), _chunks.pos_.begin());
  }

  template<typename __ObjContainer, typename __PosContainer>
  bool operator!=(const Chunks<__ObjContainer, __PosContainer> &_chunks) const {
    return !(*this == _chunks);
  }

  std::size_t serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
    std::size_t written_bytes = 0;
    written_bytes += sdsl::serialize(objs_, out);
    written_bytes += sdsl::serialize(pos_, out);

    return written_bytes;
  }

  void load(std::istream &in) {
    sdsl::load(objs_, in);
    sdsl::load(pos_, in);
  }

 protected:
  _ObjContainer objs_;
  _PosContainer pos_;
};


/**
 *  Sampled Precomputed Terminal Set
 *
 * @tparam _SLP
 * @tparam _ValueType
 * @tparam __block_size
 */
template<typename _SLP, typename _ValueType = uint32_t, uint32_t __block_size = 256>
class SampledPTS {
 public:
  typedef std::size_t size_type;

  SampledPTS() = default;

  template<typename _SLPValueType = uint32_t>
  SampledPTS(const _SLP *_slp, uint32_t _block_size = __block_size, float _storing_factor = 16) {
    Compute<_SLPValueType>(_slp, _block_size, _storing_factor);
  }

  template<typename _SLPValueType = uint32_t>
  void Compute(const _SLP *_slp, uint32_t _block_size = __block_size, float _storing_factor = 16) {
    slp_ = _slp;
    std::vector<_SLPValueType> nodes;

    auto add_set = [this, &nodes](const auto &_slp, std::size_t _curr_var) {
      auto set = _slp.Span(_curr_var);
      sort(set.begin(), set.end());
      set.erase(unique(set.begin(), set.end()), set.end());

      pts_.Insert(set.begin(), set.end());
      if (vars_.count(_curr_var) == 0) {
        vars_[_curr_var] = pts_.size();
      }

      nodes.emplace_back(_curr_var);
    };
    ComputeSampledSLPLeaves(*slp_, _block_size, add_set);

    grammar::MustBeSampled<decltype(pts_)> pred(grammar::AreChildrenTooBig<decltype(pts_)>(pts_, _storing_factor));

    auto build_inner_data = [&add_set, this](
        const _SLP &_slp,
        std::size_t _curr_var,
        const auto &_nodes,
        std::size_t _new_node,
        const auto &_left_ranges,
        const auto &_right_ranges) {
      add_set(_slp, _curr_var);
    };

    ComputeSampledSLPNodes(*slp_, _block_size, nodes, pred, build_inner_data);
  }

  std::vector<_ValueType> operator[](_ValueType i) const {
    std::vector<bool> _visited(i, false);
    return GetSet(i, _visited);
  }

  bool operator==(const SampledPTS<_SLP, _ValueType, __block_size> &_sampled_pts) const {
    return pts_ == _sampled_pts.pts_ && vars_ == _sampled_pts.vars_;
  }

  bool operator!=(const SampledPTS<_SLP, _ValueType, __block_size> &_sampled_pts) const {
    return !(*this == _sampled_pts);
  }

  std::size_t serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
    std::size_t written_bytes = 0;
    written_bytes += pts_.serialize(out);
    written_bytes += sdsl::serialize(vars_.size(), out);
    for (const auto &item : vars_) {
      written_bytes += std::serialize(item, out);
    }

    return written_bytes;
  }

  void load(std::istream &in) {
    sdsl::load(pts_, in);
    std::size_t size = 0;
    sdsl::load(size, in);
    for (int i = 0; i < size; ++i) {
      typename decltype(vars_)::value_type pair;
      std::load(pair, in);
      vars_.insert(pair);
    }
  }

  void SetSLP(const _SLP *_slp) {
    slp_ = _slp;
  }

 private:
  const _SLP *slp_ = nullptr;

  Chunks<std::vector<_ValueType>> pts_;
  std::map<_ValueType, std::size_t> vars_;

  template<typename _BitVector>
  std::vector<_ValueType> GetSet(_ValueType i, _BitVector &_visited) const {
    std::vector<_ValueType> set;
    auto it = vars_.find(i);
    if (it != vars_.end()) {
      auto s = pts_[it->second];
      copy(s.first, s.second, back_inserter(set));
    } else {
      assert(slp_ != nullptr);

      if (i <= slp_->Sigma()) {
        return {i};
      }

      auto children = (*slp_)[i];
      auto left = !_visited[children.first] ? GetSet(children.first, _visited) : std::vector<_ValueType>(0);
      auto right = !_visited[children.second] ? GetSet(children.second, _visited) : std::vector<_ValueType>(0);

      set_union(left.begin(), left.end(), right.begin(), right.end(), back_inserter(set));
    }

    _visited[i] = 1;

    return set;
  }
};


/**
 * Grammar-Compressed Chunks
 *
 * @tparam _SLP
 * @tparam kExpand
 * @tparam _Chunks
 */
template<typename _SLP, bool kExpand = true, typename _Chunks = Chunks<std::vector<typename _SLP::VariableType>>>
class GCChunks {
 public:
  typedef std::size_t size_type;

  GCChunks() = default;

  template<typename __SLP,
      typename __Chunks,
      typename __SLPAct1 = NoAction,
      typename __SLPAct2 = NoAction,
      typename __ChunksAct1 = NoAction,
      typename __ChunksAct2 = NoAction>
  GCChunks(const GCChunks<__SLP, kExpand, __Chunks> &_gcchunks,
           __SLPAct1 &&_slp_act1 = NoAction(),
           __SLPAct2 &&_slp_act2 = NoAction(),
           __ChunksAct1 &&_chunks_act1 = NoAction(),
           __ChunksAct2 &&_chunks_act2 = NoAction()):
      slp_(_gcchunks.GetSLP(), _slp_act1, _slp_act2),
      chunks_(_gcchunks.GetChunks(), _chunks_act1, _chunks_act2) {
  }

  template<typename _II, typename __Chunks, typename _Encoder>
  GCChunks(_II _begin, _II _end, const __Chunks &_chunks, _Encoder &_encoder) {
    Compute(_begin, _end, _chunks, _encoder);
  }

  template<typename _II, typename __Chunks, typename _Encoder>
  void Compute(_II _begin, _II _end, const __Chunks &_chunks, _Encoder &_encoder) {
    std::vector<typename _SLP::VariableType> cseq;

    auto report_cseq = [&cseq](auto v) {
      cseq.emplace_back(v);
    };

    auto wrapper = BuildSLPWrapper(slp_);
    _encoder.Encode(_begin, _end, wrapper, report_cseq);

    auto get_length = [](const auto &_chunk) -> auto {
      return _chunk.size();
    };

    if (kExpand) {
      auto action_expand = [this](auto &_set) { chunks_.Insert(_set.begin(), _set.end()); };

      ComputePartitionCover(slp_, cseq, _chunks, get_length, action_expand, 1);
    } else {
      auto action_no_expand = [this](auto &_set) {
        std::sort(_set.begin(), _set.end());
        chunks_.Insert(_set.begin(), _set.end());
      };

      ComputePartitionCover(slp_, cseq, _chunks, get_length, action_no_expand, 1);
    }
  }

  auto size() const {
    return chunks_.size();
  }

  template<typename _Index>
  auto operator[](_Index i) const -> typename std::enable_if<
      std::integral_constant<
          bool,
          kExpand && std::is_integral<_Index>::value
      >::value,
      std::vector<uint32_t>
  >::type {
    assert(i <= chunks_.size());

    std::vector<uint32_t> set;

    auto chunk = chunks_[i];
    for (auto it = chunk.begin(); it != chunk.end(); ++it) {
      auto value = *it;
      if (slp_.IsTerminal(value)) {
        set.push_back(value);
      } else {
        slp_.Span(value, back_inserter(set));
      }
    }
    return set;
  }

  template<typename _Index>
  auto operator[](_Index i) const -> typename std::enable_if<
      std::integral_constant<
          bool,
          !kExpand && std::is_integral<_Index>::value
      >::value,
      typename _Chunks::FakeContainer
  >::type {
    return chunks_[i];
  }

  const _SLP &GetSLP() const {
    return slp_;
  }

  const _Chunks &GetChunks() const {
    return chunks_;
  }

  template<typename __SLP, typename __Chunks>
  bool operator==(const GCChunks<__SLP, kExpand, __Chunks> &_gcchunks) const {
    return slp_ == _gcchunks.GetSLP() && chunks_ == _gcchunks.GetChunks();
  }

  template<typename __SLP, typename __Chunks>
  bool operator!=(const GCChunks<__SLP, kExpand, __Chunks> &_gcchunks) const {
    return !(*this == _gcchunks);
  }

  std::size_t serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
    std::size_t written_bytes = 0;
    written_bytes += sdsl::serialize(slp_, out);
    written_bytes += sdsl::serialize(chunks_, out);

    return written_bytes;
  }

  void load(std::istream &in) {
    sdsl::load(slp_, in);
    sdsl::load(chunks_, in);
  }

 private:
  _SLP slp_;
  _Chunks chunks_;
};


/**
 * Grammar-Compressed Chunks with Terminal & Variables separated
 *
 * @tparam _SLP
 * @tparam _TerminalChunks
 * @tparam _VariableChunks
 */
template<typename _SLP, typename _TerminalChunks = Chunks<>, typename _VariableChunks = Chunks<>>
class GCChunksTV {
 public:
  GCChunksTV() = default;

  template<typename __SLP,
      typename __TChunks,
      typename __VChunks,
      typename __SLPAct1 = NoAction,
      typename __SLPAct2 = NoAction,
      typename __TChunksAct1 = NoAction,
      typename __TChunksAct2 = NoAction,
      typename __VChunksAct1 = NoAction,
      typename __VChunksAct2 = NoAction>
  GCChunksTV(const __SLP &_slp,
             const __TChunks &_tchunks,
             const __VChunks &_vchunks,
             __SLPAct1 &&_slp_act1 = NoAction(),
             __SLPAct2 &&_slp_act2 = NoAction(),
             __TChunksAct1 &&_tchunks_act1 = NoAction(),
             __TChunksAct2 &&_tchunks_act2 = NoAction(),
             __VChunksAct1 &&_vchunks_act1 = NoAction(),
             __VChunksAct2 &&_vchunks_act2 = NoAction())
      : slp_(_slp, _slp_act1, _slp_act2),
        tchunks_(_tchunks, _tchunks_act1, _tchunks_act2),
        vchunks_(_vchunks, _vchunks_act1, _vchunks_act2) {
  }

  template<typename _II, typename _Chunks, typename _Encoder>
  GCChunksTV(_II _first, _II _last, const _Chunks &_chunks, _Encoder &_encoder) {
    Compute(_first, _last, _chunks, _encoder);
  }

  template<typename _II, typename _Chunks, typename _Encoder>
  void Compute(_II _first, _II _last, const _Chunks &_chunks, _Encoder &_encoder) {
    std::vector<typename _SLP::VariableType> cseq;

    auto report_cseq = [&cseq](auto v) {
      cseq.emplace_back(v);
    };

    auto wrapper = BuildSLPWrapper(slp_);
    _encoder.Encode(_first, _last, wrapper, report_cseq);

    auto get_length = [](const auto &_chunk) -> auto {
      return _chunk.size();
    };

    auto action = [this](auto &_set) {
      std::sort(_set.begin(), _set.end());
      auto it_last_terminal = std::lower_bound(_set.begin(), _set.end(), slp_.Sigma() + 1);
      tchunks_.Insert(_set.begin(), it_last_terminal);
      vchunks_.Insert(it_last_terminal, _set.end());
    };


    ComputePartitionCover(slp_, cseq, _chunks, get_length, action, 1);
  }

  auto size() const {
    return tchunks_.size();
  }

  auto GetTerminalSet(std::size_t i) const {
    return tchunks_[i];
  }

  auto GetVariableSet(std::size_t i) const {
    return vchunks_[i];
  }

  auto operator[](std::size_t i) const {
    return GetTerminalSet(i);
  }

  const auto &GetSLP() const {
    return slp_;
  }

  const auto &GetTerminalChunks() const {
    return tchunks_;
  }

  const auto &GetVariableChunks() const {
    return vchunks_;
  }

  template<typename __SLP, typename __TChunks, typename __VChunks>
  bool operator==(const GCChunksTV<__SLP, __TChunks, __VChunks> &_chunks) const {
    return slp_ == _chunks.GetSLP()
        && tchunks_ == _chunks.GetTerminalChunks()
        && vchunks_ == _chunks.GetVariableChunks();
  }

  template<typename __SLP, typename __TChunks, typename __VChunks>
  bool operator!=(const GCChunksTV<__SLP, __TChunks, __VChunks> &_chunks) const {
    return !(*this == _chunks);
  }

  typedef std::size_t size_type;

  std::size_t serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
    std::size_t written_bytes = 0;
    written_bytes += sdsl::serialize(slp_, out);
    written_bytes += sdsl::serialize(tchunks_, out);
    written_bytes += sdsl::serialize(vchunks_, out);

    return written_bytes;
  }

  void load(std::istream &in) {
    sdsl::load(slp_, in);
    sdsl::load(tchunks_, in);
    sdsl::load(vchunks_, in);
  }

 protected:
  _SLP slp_;
  _TerminalChunks tchunks_;
  _VariableChunks vchunks_;
};

}

#endif //GRAMMAR_SLP_METADATA_H
