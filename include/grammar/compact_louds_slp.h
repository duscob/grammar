//
// Compact LOUDS-encoded grammar SLP class. Companion to `CompactBPSLP`,
// using `grammar::CreateCompactGrammarForest` to encode the derivation
// forest as ~half the bits (one bit per node — `1` for an internal-open,
// `0` for a leaf — with no closing markers).
//
// Compared to `CompactBPSLP`:
//   - Tree size is roughly 2n bits instead of 4n bits (n = number of nodes).
//   - `operator[]` is O(left subtree) — the second child requires an excess
//     scan over the left subtree. The hot path consumed by GCDA goes through
//     a dedicated `dret::ExpandSLP` overload that uses
//     `grammar::ExpandCompactSLPForward`/`Backward`, NOT `operator[]`.
//   - Leaf rank is direct `rank_0` over `bv_tree_` (no separate leaf-marks
//     bitvector), since LOUDS leaves are bare `0` bits.
//
// The sampled base is fixed to `grammar::SampledSLP<>` to match the
// `grammar::CombinedSLP<>` typically used to drive `Compute`.
//

#ifndef GRAMMAR_COMPACT_LOUDS_SLP_H_
#define GRAMMAR_COMPACT_LOUDS_SLP_H_

#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>

#include <sdsl/bit_vectors.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/io.hpp>
#include <sdsl/rank_support.hpp>
#include <sdsl/util.hpp>

#include "grammar/sampled_slp.h"
#include "grammar/slp_compact.h"

namespace grammar {

template<typename _BVTree        = sdsl::bit_vector,
         typename _BVTreeRank0   = sdsl::rank_support_v5<0>,
         typename _LeavesVec     = sdsl::int_vector<>,
         typename _SampledLeaves = sdsl::int_vector<>>
class CompactLOUDSSLP : public SampledSLP<> {
 public:
  using VariableType = std::size_t;
  using SampledBase  = SampledSLP<>;

  CompactLOUDSSLP() = default;

  // Build from a fully-Compute'd `grammar::CombinedSLP<>` (same contract as
  // CompactBPSLP::Compute).
  template<typename _CombinedSLP>
  void Compute(const _CombinedSLP &cslp) {
    sigma_ = cslp.Sigma();

    // 1) Copy the sampled-tree base (safe: SampledSLP::operator= rebinds
    //    rank/select supports against *this).
    static_cast<SampledBase &>(*this) = static_cast<const SampledBase &>(cslp);

    // 2) Build the LOUDS compact tree.
    auto get_children = [&cslp](auto v) { return cslp[v]; };
    std::array<typename _CombinedSLP::VariableType, 1> roots{cslp.Start()};
    auto[tree, leaves, nt_id] =
        CreateCompactGrammarForest(roots, sigma_, get_children);

    // 3) Pack tree and build rank_0 over it. LOUDS leaves are bare `0`
    //    bits, so rank_0(p) directly gives the leaf rank into compact_leaves_.
    bv_tree_ = _BVTree(tree.size());
    for (std::size_t i = 0; i < tree.size(); ++i) bv_tree_[i] = tree[i];
    sdsl::util::init_support(leaf_rank_, &bv_tree_);

    // 4) Pack the compact-leaves vector (already remapped).
    compact_leaves_ = _LeavesVec(leaves.size());
    for (std::size_t i = 0; i < leaves.size(); ++i) compact_leaves_[i] = leaves[i];
    sdsl::util::bit_compress(compact_leaves_);

    // 5) Translate cslp.GetLeaves() into compact ids via nt_id. Terminals
    //    (<= sigma_) pass through unchanged.
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

  // O(left subtree) via excess scan to find the second child. The GCDA hot
  // path bypasses this method and uses the dedicated dret::ExpandSLP overload
  // for CompactLOUDSSLP, which calls grammar::ExpandCompactSLPForward/Backward
  // directly. operator[] is correctness-only and used by Span(), recursive
  // expansion in non-hot paths, and external code that doesn't know about
  // the dedicated overload.
  std::pair<VariableType, VariableType> operator[](VariableType v) const {
    auto p = v - sigma_ - 1;                // LOUDS open of v (a `1` bit)
    auto first_pos  = p + 1;
    auto second_pos = secondChildPos(first_pos);
    return {readChild(first_pos), readChild(second_pos)};
  }

  std::array<VariableType, 1> Cover(std::size_t leaf) const {
    return {Map(leaf)};
  }

  VariableType Map(std::size_t leaf) const { return sampled_leaves_[leaf - 1]; }

  // ---- Public accessors for free-function helpers (e.g. dret::collectSizes,
  //      the dret::ExpandSLP overload).

  const _BVTree        &BvTree() const         { return bv_tree_; }
  const _BVTreeRank0   &LeafRank() const       { return leaf_rank_; }
  const _LeavesVec     &CompactLeaves() const  { return compact_leaves_; }
  const _SampledLeaves &SampledLeaves() const  { return sampled_leaves_; }

  // ---- Equality / serialization ----

  bool operator==(const CompactLOUDSSLP &o) const {
    if (sigma_ != o.sigma_) return false;
    if (!(static_cast<const SampledBase &>(*this) == static_cast<const SampledBase &>(o))) return false;
    if (bv_tree_.size() != o.bv_tree_.size() ||
        !std::equal(bv_tree_.begin(), bv_tree_.end(), o.bv_tree_.begin())) return false;
    if (compact_leaves_.size() != o.compact_leaves_.size() ||
        !std::equal(compact_leaves_.begin(), compact_leaves_.end(), o.compact_leaves_.begin())) return false;
    if (sampled_leaves_.size() != o.sampled_leaves_.size() ||
        !std::equal(sampled_leaves_.begin(), sampled_leaves_.end(), o.sampled_leaves_.begin())) return false;
    return true;
  }

  bool operator!=(const CompactLOUDSSLP &o) const { return !(*this == o); }

  std::size_t serialize(std::ostream &out,
                        sdsl::structure_tree_node *v = nullptr,
                        const std::string &name = "") const {
    std::size_t written = 0;
    written += sdsl::serialize(sigma_, out);
    written += bv_tree_.serialize(out);
    written += sdsl::serialize(compact_leaves_, out);
    written += sdsl::serialize(sampled_leaves_, out);
    written += SampledBase::serialize(out, v, name);
    return written;
    // leaf_rank_ is NOT persisted; rebuilt in load().
  }

  void load(std::istream &in) {
    sdsl::load(sigma_, in);
    bv_tree_.load(in);
    sdsl::load(compact_leaves_, in);
    sdsl::load(sampled_leaves_, in);
    SampledBase::load(in);
    sdsl::util::init_support(leaf_rank_, &bv_tree_);
  }

 private:
  // Resolve the variable id at LOUDS position `pos`.
  //   - bv_tree_[pos] == 0 : leaf, look up in compact_leaves_ via leaf_rank_.
  //   - bv_tree_[pos] == 1 : internal node; its compact id is pos + sigma_ + 1.
  VariableType readChild(std::size_t pos) const {
    if (bv_tree_[pos] == 0) {
      return compact_leaves_[leaf_rank_(pos)];
    }
    return pos + sigma_ + 1;
  }

  // In LOUDS-without-close, each subtree contributes excess = -1 over its
  // bits (each `1` is +1, each `0` is -1; a complete subtree's net sum is
  // -1 because internal nodes have one open per two children, etc.).
  // Starting from the first child's first bit, walk forward tracking
  // running excess. When excess hits -1, the first child's subtree has
  // ended; the second child starts at the next position.
  std::size_t secondChildPos(std::size_t first_pos) const {
    long excess = 0;
    std::size_t p = first_pos;
    while (true) {
      excess += (bv_tree_[p] == 1) ? +1 : -1;
      ++p;
      if (excess < 0) return p;
    }
  }

  std::size_t sigma_ = 0;
  _BVTree bv_tree_;
  _BVTreeRank0 leaf_rank_;
  _LeavesVec compact_leaves_;
  _SampledLeaves sampled_leaves_;
};

}  // namespace grammar

#endif  // GRAMMAR_COMPACT_LOUDS_SLP_H_
