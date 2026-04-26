//
// Compact BP-encoded grammar SLP class. Mirrors `LightSLP`'s public surface
// so it is a drop-in `TSLP` substitute (e.g. for `dret::gcda::DocListIdxGCDA`),
// while storing the derivation forest as a balanced-parentheses bit-vector
// over `grammar::CreateCompactGrammarTreeWithBP`.
//
// Compared to `LightSLP`, this class:
//   - Stores the grammar as the BP-tree alone (no compact-leaf chunks).
//   - Has unit cover semantics: `Cover(leaf) = { Map(leaf) }`.
//   - Resolves `operator[]` in O(1) amortised via `bp_support_*::find_close`.
//
// The sampled base is fixed to `grammar::SampledSLP<>` to match the
// `grammar::CombinedSLP<>` typically used to drive `Compute`. Varying the
// sampled base is intentionally not exposed in this version.
//

#ifndef GRAMMAR_COMPACT_BP_SLP_H_
#define GRAMMAR_COMPACT_BP_SLP_H_

#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>

#include <sdsl/bit_vectors.hpp>
#include <sdsl/bp_support_sada.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/io.hpp>
#include <sdsl/sd_vector.hpp>
#include <sdsl/util.hpp>

#include "grammar/sampled_slp.h"
#include "grammar/slp_compact.h"

namespace grammar {

template<typename _BVTree         = sdsl::bit_vector,
         typename _BPSupport      = sdsl::bp_support_sada<>,
         typename _BVLeafMarks    = sdsl::sd_vector<>,
         typename _BVLeafMarksRank = typename _BVLeafMarks::rank_1_type,
         typename _LeavesVec      = sdsl::int_vector<>,
         typename _SampledLeaves  = sdsl::int_vector<>>
class CompactBPSLP : public SampledSLP<> {
 public:
  using VariableType = std::size_t;
  using SampledBase  = SampledSLP<>;

  CompactBPSLP() = default;

  // Build from a fully-Compute'd `grammar::CombinedSLP<>` (or anything with
  // the same `Sigma()` / `Start()` / `operator[]` / `GetLeaves()` /
  // `SampledSLP<>` base interface).
  template<typename _CombinedSLP>
  void Compute(const _CombinedSLP &cslp) {
    sigma_ = cslp.Sigma();

    // 1) Copy the sampled-tree base. Thanks to the user-defined
    //    SampledSLP::operator= (which rebinds rank/select supports against
    //    *this), this is safe even if cslp dies later.
    static_cast<SampledBase &>(*this) = static_cast<const SampledBase &>(cslp);

    // 2) Build the BP compact tree over cslp's grammar rules.
    auto get_children = [&cslp](auto v) { return cslp[v]; };
    std::array<typename _CombinedSLP::VariableType, 1> roots{cslp.Start()};
    auto[tree, leaves, nt_id] =
        CreateCompactGrammarTreeWithBP(roots, sigma_, get_children);

    // 3) Pack the BP tree and build the bp-support.
    bv_tree_ = _BVTree(tree.size());
    for (std::size_t i = 0; i < tree.size(); ++i) bv_tree_[i] = tree[i];
    sdsl::util::init_support(bp_support_, &bv_tree_);

    // 4) Build a leaf-marks bitvector with a 1 at every leaf-opening BP
    //    position (the `1` of a leaf's `(1, 0)` pair). plain rank_0 over
    //    bv_tree_ would conflate leaf-close bits with internal-close bits,
    //    so a dedicated leaf-marks vector is required.
    {
      sdsl::bit_vector marks(tree.size(), 0);
      for (std::size_t p = 0; p + 1 < tree.size(); ++p) {
        if (tree[p] == 1 && tree[p + 1] == 0) marks[p] = 1;
      }
      bv_leaf_marks_ = _BVLeafMarks(marks);
      sdsl::util::init_support(bv_leaf_marks_rank_, &bv_leaf_marks_);
    }

    // 5) Pack the compact-leaves vector (already remapped to compact ids by
    //    the helper).
    compact_leaves_ = _LeavesVec(leaves.size());
    for (std::size_t i = 0; i < leaves.size(); ++i) compact_leaves_[i] = leaves[i];
    sdsl::util::bit_compress(compact_leaves_);

    // 6) Translate cslp.GetLeaves() (original SLP variable ids) into compact
    //    ids via nt_id. Terminals (<= sigma_) pass through unchanged.
    const auto &src = cslp.GetLeaves();
    sampled_leaves_ = _SampledLeaves(src.size());
    for (std::size_t i = 0; i < src.size(); ++i) {
      auto v = src[i];
      if (static_cast<std::size_t>(v) <= sigma_) {
        sampled_leaves_[i] = v;
      } else {
        auto it = nt_id.find(v);
        assert(it != nt_id.end() && "sampled leaf must appear in compact tree");
        sampled_leaves_[i] = it->second;
      }
    }
    sdsl::util::bit_compress(sampled_leaves_);
  }

  // ---- TSLP / SLP-shaped public API ----

  std::size_t Sigma() const { return sigma_; }
  bool IsTerminal(VariableType v) const { return v <= sigma_; }

  // O(1) amortised: both children resolved through `readChild`, which
  // dispatches on the BP encoding of the position.
  std::pair<VariableType, VariableType> operator[](VariableType v) const {
    auto p = v - sigma_ - 1;                                    // BP open of v
    auto first  = readChild(p + 1);
    auto second = readChild(bp_support_.find_close(p + 1) + 1);
    return {first, second};
  }

  // Unit cover: a sampled leaf covers exactly its `Map`'d variable.
  std::array<VariableType, 1> Cover(std::size_t leaf) const {
    return {Map(leaf)};
  }

  VariableType Map(std::size_t leaf) const { return sampled_leaves_[leaf - 1]; }

  // ---- Public accessors for free-function helpers (e.g. dret::collectSizes,
  //      dret::ExpandSLP). CamelCase to match grammar's existing convention.

  const _BVTree         &BvTree() const          { return bv_tree_; }
  const _BPSupport      &BpSupport() const       { return bp_support_; }
  const _BVLeafMarks    &BvLeafMarks() const     { return bv_leaf_marks_; }
  const _BVLeafMarksRank &BvLeafMarksRank() const { return bv_leaf_marks_rank_; }
  const _LeavesVec      &CompactLeaves() const   { return compact_leaves_; }
  const _SampledLeaves  &SampledLeaves() const   { return sampled_leaves_; }

  // ---- Equality / serialization ----

  bool operator==(const CompactBPSLP &o) const {
    if (sigma_ != o.sigma_) return false;
    if (!(static_cast<const SampledBase &>(*this) == static_cast<const SampledBase &>(o))) return false;
    if (bv_tree_.size() != o.bv_tree_.size() ||
        !std::equal(bv_tree_.begin(), bv_tree_.end(), o.bv_tree_.begin())) return false;
    if (bv_leaf_marks_.size() != o.bv_leaf_marks_.size() ||
        !std::equal(bv_leaf_marks_.begin(), bv_leaf_marks_.end(), o.bv_leaf_marks_.begin())) return false;
    if (compact_leaves_.size() != o.compact_leaves_.size() ||
        !std::equal(compact_leaves_.begin(), compact_leaves_.end(), o.compact_leaves_.begin())) return false;
    if (sampled_leaves_.size() != o.sampled_leaves_.size() ||
        !std::equal(sampled_leaves_.begin(), sampled_leaves_.end(), o.sampled_leaves_.begin())) return false;
    return true;
  }

  bool operator!=(const CompactBPSLP &o) const { return !(*this == o); }

  std::size_t serialize(std::ostream &out,
                        sdsl::structure_tree_node *v = nullptr,
                        const std::string &name = "") const {
    std::size_t written = 0;
    written += sdsl::serialize(sigma_, out);
    written += bv_tree_.serialize(out);
    written += bv_leaf_marks_.serialize(out);
    written += sdsl::serialize(compact_leaves_, out);
    written += sdsl::serialize(sampled_leaves_, out);
    written += SampledBase::serialize(out, v, name);
    return written;
    // bp_support_ and bv_leaf_marks_rank_ are NOT persisted; they are rebuilt
    // in load(), matching the `SampledSLP::load` / `DifferentialSLP::load`
    // pattern.
  }

  void load(std::istream &in) {
    sdsl::load(sigma_, in);
    bv_tree_.load(in);
    bv_leaf_marks_.load(in);
    sdsl::load(compact_leaves_, in);
    sdsl::load(sampled_leaves_, in);
    SampledBase::load(in);
    sdsl::util::init_support(bp_support_, &bv_tree_);
    sdsl::util::init_support(bv_leaf_marks_rank_, &bv_leaf_marks_);
  }

 private:
  // BP child-decoding: at a child's opening BP position `pos`,
  //   - a leaf is encoded as the pattern `1 0` (the open bit followed by the
  //     close bit). Look up its variable id in `compact_leaves_` keyed by
  //     `bv_leaf_marks_rank_(pos)` (the number of preceding leaf-opens).
  //   - an internal node is encoded as `1 ... 0`. Its compact variable id is
  //     the BP opening position offset by `sigma_ + 1`.
  VariableType readChild(std::size_t pos) const {
    if (bv_tree_[pos] == 1 && bv_tree_[pos + 1] == 0) {
      return compact_leaves_[bv_leaf_marks_rank_(pos)];
    }
    return pos + sigma_ + 1;
  }

  std::size_t sigma_ = 0;
  _BVTree bv_tree_;
  _BPSupport bp_support_;
  _BVLeafMarks bv_leaf_marks_;
  _BVLeafMarksRank bv_leaf_marks_rank_;
  _LeavesVec compact_leaves_;
  _SampledLeaves sampled_leaves_;
};

}  // namespace grammar

#endif  // GRAMMAR_COMPACT_BP_SLP_H_
