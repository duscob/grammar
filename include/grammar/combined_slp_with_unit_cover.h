//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 4/26/26.
//
// Thin wrapper that adds the GCDA-shaped TSLP contract's missing methods
// (`Cover(leaf)`, `IsTerminal(v)`) on top of `grammar::CombinedSLP`. Has
// no data members of its own — `serialize` / `load` slice cleanly to the
// inherited `CombinedSLP` base, so the on-disk encoding is byte-identical
// to that of the wrapped CSLP (cache files are still kept distinct via
// SDSL's type hash).
//
// `Cover(leaf)` returns a length-1 array `{ Map(leaf) }` — a "unit cover".
// Compared to `LightSLP` (which precomputes covers as `GCChunks`), this
// wrapper trades the precomputed-covers space for runtime cost: every
// sampled leaf expands its single mapped variable on demand.
//

#ifndef GRAMMAR_COMBINED_SLP_WITH_UNIT_COVER_H_
#define GRAMMAR_COMBINED_SLP_WITH_UNIT_COVER_H_

#include <array>
#include <cstddef>

#include "grammar/sampled_slp.h"

namespace grammar {

template<typename _CombinedSLP = CombinedSLP<>>
class CombinedSLPWithUnitCover : public _CombinedSLP {
 public:
  using Base = _CombinedSLP;
  using VariableType = std::size_t;
  using typename Base::size_type;

  using Base::Base;

  bool IsTerminal(VariableType v) const { return v <= Base::Sigma(); }

  std::array<VariableType, 1> Cover(std::size_t leaf) const {
    return {Base::Map(leaf)};
  }

  bool operator==(const CombinedSLPWithUnitCover &other) const {
    return Base::operator==(other);
  }

  bool operator!=(const CombinedSLPWithUnitCover &other) const {
    return !(*this == other);
  }
};

}  // namespace grammar

#endif  // GRAMMAR_COMBINED_SLP_WITH_UNIT_COVER_H_
