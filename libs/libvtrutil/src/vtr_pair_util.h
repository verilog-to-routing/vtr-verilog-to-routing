#pragma once

#include <iterator>

namespace vtr {
/**
 * @brief Iterator which dereferences the 'first' element of a std::pair iterator
 */
template<typename PairIter>
class pair_first_iter {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = typename PairIter::value_type::first_type;
    using difference_type = void;
    using pointer = value_type*;
    using reference = value_type&;

    ///@brief constructor
    pair_first_iter(PairIter init)
        : iter_(init) {}

    ///@brief increment operator (++)
    auto operator++() {
        iter_++;
        return *this;
    }

    ///@brief decrement operator (\-\-)
    auto operator--() {
        iter_--;
        return *this;
    }

    ///@brief dereference * operator
    auto operator*() { return iter_->first; }

    ///@brief -> operator
    auto operator->() { return &iter_->first; }

    ///@brief == operator
    friend bool operator==(const pair_first_iter lhs, const pair_first_iter rhs) { return lhs.iter_ == rhs.iter_; }

    ///@brief != operator
    friend bool operator!=(const pair_first_iter lhs, const pair_first_iter rhs) { return !(lhs == rhs); }

  private:
    PairIter iter_;
};

/**
 *Iterator which dereferences the 'second' element of a std::pair iterator
 */
template<typename PairIter>
class pair_second_iter {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = typename PairIter::value_type::second_type;
    using difference_type = void;
    using pointer = value_type*;
    using reference = value_type&;

    ///@brief constructor
    pair_second_iter(PairIter init)
        : iter_(init) {}

    ///@brief increment operator (++)
    auto operator++() {
        iter_++;
        return *this;
    }

    ///@brief decrement operator (--)
    auto operator--() {
        iter_--;
        return *this;
    }

    ///@brief dereference * operator
    auto operator*() { return iter_->second; }

    ///@brief -> operator
    auto operator->() { return &iter_->second; }

    ///@brief == operator
    friend bool operator==(const pair_second_iter lhs, const pair_second_iter rhs) { return lhs.iter_ == rhs.iter_; }

    ///@brief != operator
    friend bool operator!=(const pair_second_iter lhs, const pair_second_iter rhs) { return !(lhs == rhs); }

  private:
    PairIter iter_;
};

} // namespace vtr
