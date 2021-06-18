//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 8/14/18.
//

#ifndef GRAMMAR_UTILITY_H
#define GRAMMAR_UTILITY_H

#include <type_traits>
#include <algorithm>
#include <vector>
#include <map>

#include <sdsl/bit_vectors.hpp>


namespace grammar {

class NoAction {
 public:
  template<typename ...Args>
  void operator()(Args... args) {}
};


template<typename _X, typename _Y>
typename std::enable_if<std::is_constructible<_X, _Y>::value>::type Construct(_X &_x, const _Y &_y) {
  _x = _X(_y);
}


template<typename _X, typename _Y>
typename std::enable_if<!std::is_constructible<_X, _Y>::value>::type Construct(_X &_x, const _Y &_y) {
  _x = _X(_y.size());
  std::copy(_y.begin(), _y.end(), _x.begin());
}


template<typename TBv>
void Construct(TBv &_bv, const std::vector<bool> &_tmp_bv) {
  sdsl::bit_vector bv(_tmp_bv.size());
  std::copy(_tmp_bv.begin(), _tmp_bv.end(), bv.begin());

  _bv = TBv(bv);
}


template<typename TV>
void Construct(TV &_v, const std::map<std::size_t, std::size_t> &_tmp_v) {
  std::vector<std::size_t> v(0);
  v.reserve(_tmp_v.size());
  for (const auto &item : _tmp_v) {
    v.emplace_back(item.second);
  }

  Construct(_v, v);
}


// Primary template with a static assertion
// for a meaningful error message
// if it ever gets instantiated.
// We could leave it undefined if we didn't care.
template<typename, typename T>
struct has_push_back {
  static_assert(
      std::integral_constant<T, false>::value,
      "Second template parameter needs to be of function type.");
};


// specialization that does the checking
template<typename C, typename Ret, typename... Args>
struct has_push_back<C, Ret(Args...)> {
 private:
  template<typename T>
  static constexpr auto check(T *)
  -> typename std::is_same<
      decltype(std::declval<T>().push_back(std::declval<Args>()...)),
      Ret    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  >::type;  // attempt to call it and see if the return type is correct

  template<typename>
  static constexpr std::false_type check(...);

  typedef decltype(check<C>(0)) type;

 public:
  static constexpr bool value = type::value;
};
}

#endif //GRAMMAR_UTILITY_H
