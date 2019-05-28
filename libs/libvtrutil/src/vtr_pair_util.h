#ifndef VTR_PAIR_UTIL_H
#define VTR_PAIR_UTIL_H

#include "vtr_range.h"

namespace vtr {

//Iterator which derefernces the 'first' element of a std::pair iterator
template<typename PairIter>
class pair_first_iter {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = typename PairIter::value_type::first_type;
    using difference_type = void;
    using pointer = value_type*;
    using reference = value_type&;

    pair_first_iter(PairIter init)
        : iter_(init) {}
    auto operator++() {
        iter_++;
        return *this;
    }
    auto operator--() {
        iter_--;
        return *this;
    }
    auto operator*() { return iter_->first; }
    auto operator-> () { return &iter_->first; }

    friend bool operator==(const pair_first_iter lhs, const pair_first_iter rhs) { return lhs.iter_ == rhs.iter_; }
    friend bool operator!=(const pair_first_iter lhs, const pair_first_iter rhs) { return !(lhs == rhs); }

  private:
    PairIter iter_;
};

//Iterator which derefernces the 'second' element of a std::pair iterator
template<typename PairIter>
class pair_second_iter {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = typename PairIter::value_type::second_type;
    using difference_type = void;
    using pointer = value_type*;
    using reference = value_type&;

    pair_second_iter(PairIter init)
        : iter_(init) {}
    auto operator++() {
        iter_++;
        return *this;
    }
    auto operator--() {
        iter_--;
        return *this;
    }
    auto operator*() { return iter_->second; }
    auto operator-> () { return &iter_->second; }

    friend bool operator==(const pair_second_iter lhs, const pair_second_iter rhs) { return lhs.iter_ == rhs.iter_; }
    friend bool operator!=(const pair_second_iter lhs, const pair_second_iter rhs) { return !(lhs == rhs); }

  private:
    PairIter iter_;
};

} // namespace vtr
#endif
