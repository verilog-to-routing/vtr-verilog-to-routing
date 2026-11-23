#pragma once

#include <cstddef>
#include <iterator>
#include <span>
#include <stdexcept>
#include "vtr_range.h"

namespace vtr {

/**
 * @brief Implements a fixed length view to an array which is indexed by vtr::StrongId
 *
 * The main use of this container is to behave like a std::span which is
 * indexed by a vtr::StrongId instead of size_t. It assumes that K is explicitly 
 * convertible to size_t 
 * (i.e. via operator size_t()), and can be explicitly constructed from a size_t.
 */
template<typename K, typename V>
class array_view_id : private std::span<V> {
    using storage = std::span<V>;

  public:
    explicit constexpr array_view_id(V* str, size_t a_size)
        : storage(str, a_size) {}

    typedef K key_type;

    class key_iterator;
    typedef vtr::Range<key_iterator> key_range;

    // Do not expose operator[] from std::span, since it is redefined here to take key_type instead of size_t
    ///@brief [] operator
    V& operator[](const key_type id) {
        auto i = size_t(id);
        return storage::operator[](i);
    }
    ///@brief constant [] operator
    const V& operator[](const key_type id) const {
        auto i = size_t(id);
        return storage::operator[](i);
    }
    ///@brief at() operator
    V& at(const key_type id) {
        size_t i = size_t(id);
        if (i >= storage::size()) {
            throw std::out_of_range("Pos is out of range.");
        }
        return storage::operator[](i);
    }
    ///@brief constant at() operator
    const V& at(const key_type id) const {
        size_t i = size_t(id);
        if (i >= storage::size()) {
            throw std::out_of_range("Pos is out of range.");
        }
        return storage::operator[](i);
    }

    ///@brief Returns a range containing the keys
    key_range keys() const {
        return vtr::make_range(key_begin(), key_end());
    }

    using storage::begin;
    using storage::end;

    using storage::empty;
    using storage::size;

    using storage::back;
    using storage::data;
    using storage::front;

  public:
    /**
     * @brief Iterator class which is convertible to the key_type
     *
     * This allows end-users to call the parent class's keys() member
     * to iterate through the keys with a range-based for loop
     *
     */
    class key_iterator {
      public:
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = key_type;
        using pointer = key_type*;
        using reference = key_type&;

        key_iterator(value_type init)
            : value_(init) {}

        /**
         * @brief Note
         *
         * vtr::vector assumes that the key time is convertible to size_t and
         * that all the underlying IDs are zero-based and contiguous. That means
         * we can just increment the underlying Id to build the next key.
         */

        ///@brief increment the iterator
        key_iterator& operator++() {
            value_ = value_type(size_t(value_) + 1);
            return *this;
        }

        ///@brief decrement the iterator
        key_iterator& operator--() {
            value_ = value_type(size_t(value_) - 1);
            return *this;
        }

        ///@brief dereference operator (*)
        reference operator*() { return value_; }

        ///@brief -> operator
        pointer operator->() { return &value_; }

        friend bool operator==(const key_iterator& lhs, const key_iterator& rhs) { return lhs.value_ == rhs.value_; }
        friend bool operator!=(const key_iterator& lhs, const key_iterator& rhs) { return !(lhs == rhs); }

      private:
        value_type value_;
    };

  private:
    key_iterator key_begin() const { return key_iterator(key_type(0)); }
    key_iterator key_end() const { return key_iterator(key_type(size())); }
};

template<typename Container>
array_view_id<typename Container::key_type, const typename Container::value_type> make_const_array_view_id(Container& container) {
    return array_view_id<typename Container::key_type, const typename Container::value_type>(
        container.data(), container.size());
}

} // namespace vtr
