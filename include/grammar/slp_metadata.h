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
    if (_first == _last)
      return 0;

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

  typedef std::size_t size_type;

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

    auto add_set = [this](const auto &_slp, std::size_t _curr_var) {
      auto set = _slp.Span(_curr_var);
      sort(set.begin(), set.end());
      set.erase(unique(set.begin(), set.end()), set.end());

      pts_.Insert(set.begin(), set.end());
      if (vars_.count(_curr_var) == 0) {
        vars_[_curr_var] = pts_.size();
      }
    };
    ComputeSampledSLPLeaves(*slp_, _block_size, back_inserter(nodes), add_set);

    grammar::MustBeSampled<decltype(pts_)> pred(pts_, _storing_factor);

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


template<typename _SLP, bool kExpand = true, typename _Chunks = Chunks<std::vector<typename _SLP::VariableType>>>
class GrammarCompressedChunks : public _Chunks {
 public:
  GrammarCompressedChunks() = default;

  template<typename _II, typename __Chunks, typename _Encoder>
  GrammarCompressedChunks(_II _begin, _II _end, const __Chunks &_chunks, _Encoder &_encoder) {
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

    this->pos_.reserve(_chunks.size());

    std::size_t pos = 0;
    std::size_t inner_pos = 0;
    for (int i = 1; i <= _chunks.size();
         ++i, (kExpand ? NoAction()() : std::sort(this->objs_.begin() + this->pos_.back(), this->objs_.end()))) {
      this->pos_.emplace_back(this->objs_.size());

      auto size = _chunks[i].size();

      if (inner_pos != 0) {
        // Find inner variables
        grammar::ComputeSpanCover(slp_, inner_pos, inner_pos + size, std::back_inserter(this->objs_), cseq[pos]);

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
        this->objs_.emplace_back(cseq[pos]);

        size -= var_len;
        ++pos;
      }

      if (0 < size) {
        // Find inner variables
        grammar::ComputeSpanCoverEnding(slp_, size, std::back_inserter(this->objs_), cseq[pos]);
      }

      inner_pos = size;
    }
  }

  template<typename _Index>
  auto operator[](_Index i) const -> typename std::enable_if<
      std::integral_constant<
          bool,
          kExpand && std::is_integral<_Index>::value
      >::value,
      std::vector<uint32_t>
  >::type {
    assert(i <= this->pos_.size());

    std::vector<uint32_t> set;

    auto end = (i < this->pos_.size()) ? this->objs_.begin() + this->pos_[i] : this->objs_.end();
    for (auto it = this->objs_.begin() + this->pos_[i - 1]; it != end; ++it) {
      if (slp_.IsTerminal(*it)) {
        set.push_back(*it);
      } else {
        const auto &span = slp_.Span(*it);
        std::copy(span.begin(), span.end(), back_inserter(set));
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
    return _Chunks::operator[](i);
  }

  const _SLP &GetSLP() const {
    return slp_;
  }

  bool operator==(const GrammarCompressedChunks<_SLP> &_chunks) const {
    return Chunks<std::vector<typename _SLP::VariableType>>::operator==(_chunks) && slp_ == _chunks.slp_;
  }

  bool operator!=(const GrammarCompressedChunks<_SLP> &_compact_chunks) const {
    return !(*this == _compact_chunks);
  }

  std::size_t serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
    return Chunks<std::vector<typename _SLP::VariableType>>::serialize(out, v, name) + sdsl::serialize(slp_, out);
  }

  void load(std::istream &in) {
    Chunks<std::vector<typename _SLP::VariableType>>::load(in);
    sdsl::load(slp_, in);
  }

 private:
  _SLP slp_;
};

};

#endif //GRAMMAR_SLP_METADATA_H
