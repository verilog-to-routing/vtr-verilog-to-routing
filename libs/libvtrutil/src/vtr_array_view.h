#ifndef _VTR_ARRAY_VIEW_H
#define _VTR_ARRAY_VIEW_H

#include <cstddef>
#include <stdexcept>
#include <iterator>
#include "vtr_range.h"

namespace vtr {

/**
 * @brief An array view class to avoid copying data
 */
template<typename T>
class array_view {
  public:
    ///@brief default constructor
    explicit constexpr array_view()
        : data_(nullptr)
        , size_(0) {}

    ///@brief A constructor with data initialization
    explicit constexpr array_view(T* str, size_t size)
        : data_(str)
        , size_(size) {}

    constexpr array_view(const array_view& other) noexcept = default;
    constexpr array_view& operator=(const array_view& view) noexcept {
        data_ = view.data_;
        size_ = view.size_;
        return *this;
    }

    ///@brief [] operator
    constexpr T& operator[](size_t pos) {
        return data_[pos];
    }

    ///@brief constant [] operator
    constexpr const T& operator[](size_t pos) const {
        return data_[pos];
    }

    ///@brief at() operator
    T& at(size_t pos) {
        if (pos >= size()) {
            throw std::out_of_range("Pos is out of range.");
        }

        return data_[pos];
    }

    ///@brief const at() operator
    const T& at(size_t pos) const {
        if (pos >= size()) {
            throw std::out_of_range("Pos is out of range.");
        }

        return data_[pos];
    }

    ///@brief get the first element of the array
    constexpr T& front() {
        return data_[0];
    }

    ///@brief get the first element of the array (can't update it)
    constexpr const T& front() const {
        return data_[0];
    }

    ///@brief get the last element of the array
    constexpr T& back() {
        return data_[size() - 1];
    }

    ///@brief get the last element of the array (can't update it)
    constexpr const T& back() const {
        return data_[size() - 1];
    }

    ///@brief return the underlying pointer
    constexpr T* data() {
        return data_;
    }

    ///@brief return the underlying pointer (constant pointer)
    constexpr const T* data() const {
        return data_;
    }

    ///@brief return thr array size
    constexpr size_t size() const noexcept {
        return size_;
    }

    ///@brief return the array size
    constexpr size_t length() const noexcept {
        return size_;
    }

    ///@brief check if the array is empty
    constexpr bool empty() const noexcept {
        return size_ != 0;
    }

    ///@brief return a pointer to the first element of the array
    constexpr T* begin() noexcept {
        return data_;
    }

    ///@brief return a constant pointer to the first element of the array
    constexpr const T* begin() const noexcept {
        return data_;
    }

    ///@brief return a constant pointer to the first element of the array
    constexpr const T* cbegin() const noexcept {
        return data_;
    }

    ///@brief return a pointer to the last element of the array
    constexpr T* end() noexcept {
        return data_ + size_;
    }

    ///@brief return a constant pointer to the last element of the array
    constexpr const T* end() const noexcept {
        return data_ + size_;
    }

    ///@brief return a constant pointer to the last element of the array
    constexpr const T* cend() const noexcept {
        return data_ + size_;
    }

  private:
    T* data_;
    size_t size_;
};

/**
 * @brief Implements a fixed length view to an array which is indexed by vtr::StrongId
 *
 * The main use of this container is to behave like a std::span which is
 * indexed by a vtr::StrongId instead of size_t. It assumes that K is explicitly 
 * convertable to size_t 
 * (i.e. via operator size_t()), and can be explicitly constructed from a size_t.
 */
template<typename K, typename V>
class array_view_id : private array_view<V> {
    using storage = array_view<V>;

  public:
    explicit constexpr array_view_id(V* str, size_t a_size)
        : array_view<V>(str, a_size) {}

    typedef K key_type;

    class key_iterator;
    typedef vtr::Range<key_iterator> key_range;

    // Don't include operator[] and at() from std::vector, since we redine them to take key_type instead of size_t
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
        auto i = size_t(id);
        return storage::at(i);
    }
    ///@brief constant at() operator
    const V& at(const key_type id) const {
        auto i = size_t(id);
        return storage::at(i);
    }

    ///@brief Returns a range containing the keys
    key_range keys() const {
        return vtr::make_range(key_begin(), key_end());
    }

    using storage::begin;
    using storage::cbegin;
    using storage::cend;
    using storage::end;

    using storage::empty;
    using storage::size;

    using storage::back;
    using storage::data;
    using storage::front;

  public:
    /**
     * @brief Iterator class which is convertable to the key_type
     *
     * This allows end-users to call the parent class's keys() member
     * to iterate through the keys with a range-based for loop
     *
     */
    class key_iterator : public std::iterator<std::bidirectional_iterator_tag, key_type> {
      public:
        /**
         * @brief Intermediate type my_iter
         *
         * We use the intermediate type my_iter to avoid a potential ambiguity for which
         * clang generates errors and warnings
         */
        using my_iter = typename std::iterator<std::bidirectional_iterator_tag, K>;
        using typename my_iter::iterator;
        using typename my_iter::pointer;
        using typename my_iter::reference;
        using typename my_iter::value_type;

        key_iterator(key_iterator::value_type init)
            : value_(init) {}

        /**
         * @brief Note
         *
         * vtr::vector assumes that the key time is convertable to size_t and
         * that all the underlying IDs are zero-based and contiguous. That means
         * we can just increment the underlying Id to build the next key.
         */

        ///@brief increment the iterator
        key_iterator operator++() {
            value_ = value_type(size_t(value_) + 1);
            return *this;
        }

        ///@brief decrement the iterator
        key_iterator operator--() {
            value_ = value_type(size_t(value_) - 1);
            return *this;
        }

        ///@brief dereference operator (*)
        reference operator*() { return value_; }

        ///@brief -> operator
        pointer operator->() { return &value_; }

        friend bool operator==(const key_iterator lhs, const key_iterator rhs) { return lhs.value_ == rhs.value_; }
        friend bool operator!=(const key_iterator lhs, const key_iterator rhs) { return !(lhs == rhs); }

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

#endif /* _VTR_ARRAY_VIEW_H */
