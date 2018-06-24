//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 23-06-18.
//

#ifndef GRAMMAR_RE_PAIR_H
#define GRAMMAR_RE_PAIR_H

#include <cstddef>
#include <memory>
#include <functional>
#include <algorithm>


namespace grammar {

class RePairEncoder {
 public:
  template<typename II, typename Output>
  void encode(II _begin, II _end, Output _output) {
    auto length = std::distance(_begin, _end);
    auto C = (int *) malloc(length * sizeof(int));
    {
      auto it = _begin;
      for (int i = 0; i < length; ++i, ++it) {
        C[i] = *it;
      }
    }

    auto writer = [&_output](int left, int right, int length) -> void {
      _output(left, right, length);
    };

    prepare(C, length);
    repair(C, length, writer);
  }

 protected:
  void prepare(int [], std::size_t);
  int repair(int [], std::size_t, std::function<void(int, int, int)>);

 private:
  struct InternalData;

  std::shared_ptr<InternalData> data_;
};

}

#endif //GRAMMAR_RE_PAIR_H
