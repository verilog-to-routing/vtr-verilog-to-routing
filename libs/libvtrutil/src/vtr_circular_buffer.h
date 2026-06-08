#pragma once

#include "vtr_assert.h"

#include <compare>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace vtr {

/**
 * @brief A fixed-capacity single-threaded circular buffer.
 *
 * Elements are stored in insertion order from oldest to newest. Once the buffer
 * is full, inserting a new element overwrites the oldest element. The backing
 * storage reserves the fixed capacity up front when supported by the container,
 * and grows as values are first inserted.
 *
 * The logical order is the public STL-style sequence: index 0 is the oldest
 * element, and index size() - 1 is the newest element. The physical order is
 * the underlying storage order, which may differ after the buffer wraps. For
 * example, after pushing 1, 2, 3 into a capacity-3 buffer and then pushing 4,
 * the logical order is [2, 3, 4], while the physical storage may be [4, 2, 3]
 * with front_ pointing at the oldest element and end_ pointing at the next
 * physical write position.
 */
template<typename T, template<typename...> typename Container = std::vector>
class circular_buffer {
  public:
    typedef T value_type;
    typedef Container<T> storage_type;
    typedef typename storage_type::size_type size_type;
    typedef typename storage_type::difference_type difference_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;

  private:
    /**
     * @brief Random-access iterator over circular_buffer logical order.
     *
     * The iterator stores a logical index into the buffer rather than a direct
     * physical storage iterator. Dereferencing converts that logical index to
     * the current physical storage slot, so iteration always visits elements
     * from oldest to newest even when the backing container has wrapped.
     *
     * Iterator invalidation follows the conservative rule that mutating the
     * buffer may invalidate all iterators.
     */
    template<bool IsConst>
    class basic_iterator {
      private:
        typedef std::conditional_t<IsConst, const circular_buffer, circular_buffer> buffer_type;

      public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef std::random_access_iterator_tag iterator_concept;
        typedef circular_buffer::difference_type difference_type;
        typedef circular_buffer::value_type value_type;
        typedef std::conditional_t<IsConst, circular_buffer::const_reference, circular_buffer::reference> reference;
        typedef std::conditional_t<IsConst, circular_buffer::const_pointer, circular_buffer::pointer> pointer;

        ///@brief Default constructor creates a singular iterator.
        constexpr basic_iterator() noexcept = default;

        ///@brief Converts a mutable iterator to a const iterator.
        constexpr basic_iterator(const basic_iterator<false>& other) noexcept
            requires(IsConst)
            : buffer_(other.buffer_)
            , index_(other.index_) {}

        ///@brief Dereferences the iterator.
        constexpr reference operator*() const {
            return (*buffer_)[index_];
        }

        ///@brief Returns a pointer to the dereferenced element.
        constexpr pointer operator->() const {
            return std::addressof(operator*());
        }

        ///@brief Returns the element at a relative iterator offset.
        constexpr reference operator[](difference_type offset) const {
            return *(*this + offset);
        }

        ///@brief Advances the iterator by one element.
        constexpr basic_iterator& operator++() noexcept {
            ++index_;
            return *this;
        }

        ///@brief Advances the iterator by one element and returns the previous iterator.
        constexpr basic_iterator operator++(int) noexcept {
            basic_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        ///@brief Moves the iterator back by one element.
        constexpr basic_iterator& operator--() noexcept {
            --index_;
            return *this;
        }

        ///@brief Moves the iterator back by one element and returns the previous iterator.
        constexpr basic_iterator operator--(int) noexcept {
            basic_iterator tmp = *this;
            --(*this);
            return tmp;
        }

        ///@brief Advances the iterator by offset elements.
        constexpr basic_iterator& operator+=(difference_type offset) noexcept {
            index_ += offset;
            return *this;
        }

        ///@brief Moves the iterator back by offset elements.
        constexpr basic_iterator& operator-=(difference_type offset) noexcept {
            index_ -= offset;
            return *this;
        }

        ///@brief Returns an iterator advanced by offset elements.
        friend constexpr basic_iterator operator+(basic_iterator iter, difference_type offset) noexcept {
            iter += offset;
            return iter;
        }

        ///@brief Returns an iterator advanced by offset elements.
        friend constexpr basic_iterator operator+(difference_type offset, basic_iterator iter) noexcept {
            iter += offset;
            return iter;
        }

        ///@brief Returns an iterator moved back by offset elements.
        friend constexpr basic_iterator operator-(basic_iterator iter, difference_type offset) noexcept {
            iter -= offset;
            return iter;
        }

        ///@brief Returns the distance between two iterators into the same buffer.
        friend constexpr difference_type operator-(const basic_iterator& lhs, const basic_iterator& rhs) {
            VTR_ASSERT_SAFE_MSG(lhs.buffer_ == rhs.buffer_, "Cannot subtract circular_buffer iterators from different buffers.");
            return lhs.index_ - rhs.index_;
        }

        ///@brief Returns true if two iterators refer to the same logical position.
        friend constexpr bool operator==(const basic_iterator& lhs, const basic_iterator& rhs) noexcept {
            return lhs.buffer_ == rhs.buffer_ && lhs.index_ == rhs.index_;
        }

        ///@brief Orders two iterators into the same buffer by logical position.
        friend constexpr std::strong_ordering operator<=>(const basic_iterator& lhs, const basic_iterator& rhs) {
            VTR_ASSERT_SAFE_MSG(lhs.buffer_ == rhs.buffer_, "Cannot compare circular_buffer iterators from different buffers.");
            return lhs.index_ <=> rhs.index_;
        }

      private:
        friend class circular_buffer;
        friend class basic_iterator<true>;

        ///@brief Constructs an iterator over a buffer at a logical index.
        constexpr basic_iterator(buffer_type* buffer, difference_type index) noexcept
            : buffer_(buffer)
            , index_(index) {}

        buffer_type* buffer_ = nullptr; ///< The buffer being iterated.
        difference_type index_ = 0;     ///< Logical index in the buffer.
    };

  public:
    typedef basic_iterator<false> iterator;
    typedef basic_iterator<true> const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    ///@brief Constructs an empty zero-capacity circular buffer.
    circular_buffer() = default;

    ///@brief Constructs an empty circular buffer with fixed capacity.
    explicit circular_buffer(size_type capacity)
        : capacity_(capacity) {
        reserve_storage_(capacity_);
    }

    ///@brief Constructs a circular buffer from a range, keeping the newest values when the range exceeds capacity.
    template<std::input_iterator InputIt>
    circular_buffer(size_type capacity, InputIt first, InputIt last)
        : circular_buffer(capacity) {
        for (; first != last; ++first) {
            push_back(*first);
        }
    }

    ///@brief Constructs a circular buffer from an initializer list, keeping the newest values when it exceeds capacity.
    circular_buffer(size_type capacity, std::initializer_list<T> values)
        : circular_buffer(capacity, values.begin(), values.end()) {}

    ///@brief Returns the element at the logical index without bounds checking.
    reference operator[](size_type index) {
        VTR_ASSERT_SAFE(index < size());
        return storage_[physical_index_(index)];
    }

    ///@brief Returns the element at the logical index without bounds checking.
    const_reference operator[](size_type index) const {
        VTR_ASSERT_SAFE(index < size());
        return storage_[physical_index_(index)];
    }

    ///@brief Returns the element at the logical index with bounds checking.
    reference at(size_type index) {
        if (index >= size()) {
            throw std::out_of_range("circular_buffer index out of range");
        }
        return (*this)[index];
    }

    ///@brief Returns the element at the logical index with bounds checking.
    const_reference at(size_type index) const {
        if (index >= size()) {
            throw std::out_of_range("circular_buffer index out of range");
        }
        return (*this)[index];
    }

    ///@brief Returns the oldest element in the buffer.
    reference front() {
        VTR_ASSERT_SAFE(!empty());
        return (*this)[0];
    }

    ///@brief Returns the oldest element in the buffer.
    const_reference front() const {
        VTR_ASSERT_SAFE(!empty());
        return (*this)[0];
    }

    ///@brief Returns the newest element in the buffer.
    reference back() {
        VTR_ASSERT_SAFE(!empty());
        return (*this)[size() - 1];
    }

    ///@brief Returns the newest element in the buffer.
    const_reference back() const {
        VTR_ASSERT_SAFE(!empty());
        return (*this)[size() - 1];
    }

    ///@brief Returns an iterator to the oldest element.
    iterator begin() noexcept {
        return iterator(this, 0);
    }

    ///@brief Returns a const iterator to the oldest element.
    const_iterator begin() const noexcept {
        return cbegin();
    }

    ///@brief Returns a const iterator to the oldest element.
    const_iterator cbegin() const noexcept {
        return const_iterator(this, 0);
    }

    ///@brief Returns an iterator one past the newest element.
    iterator end() noexcept {
        return iterator(this, static_cast<difference_type>(size()));
    }

    ///@brief Returns a const iterator one past the newest element.
    const_iterator end() const noexcept {
        return cend();
    }

    ///@brief Returns a const iterator one past the newest element.
    const_iterator cend() const noexcept {
        return const_iterator(this, static_cast<difference_type>(size()));
    }

    ///@brief Returns a reverse iterator to the newest element.
    reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }

    ///@brief Returns a const reverse iterator to the newest element.
    const_reverse_iterator rbegin() const noexcept {
        return crbegin();
    }

    ///@brief Returns a const reverse iterator to the newest element.
    const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(cend());
    }

    ///@brief Returns a reverse iterator one before the oldest element.
    reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }

    ///@brief Returns a const reverse iterator one before the oldest element.
    const_reverse_iterator rend() const noexcept {
        return crend();
    }

    ///@brief Returns a const reverse iterator one before the oldest element.
    const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(cbegin());
    }

    ///@brief Returns true when the buffer contains no elements.
    bool empty() const noexcept {
        return size_ == 0;
    }

    ///@brief Returns true when the buffer contains capacity() elements.
    bool full() const noexcept {
        return size() == capacity();
    }

    ///@brief Returns the number of elements currently stored.
    size_type size() const noexcept {
        return size_;
    }

    ///@brief Returns the fixed maximum number of elements the buffer can store.
    size_type capacity() const noexcept {
        return capacity_;
    }

    ///@brief Removes all elements from the buffer while preserving capacity.
    void clear() noexcept {
        storage_.clear();
        front_ = 0;
        end_ = 0;
        size_ = 0;
    }

    ///@brief Appends a copy of value, overwriting the oldest element if full.
    void push_back(const value_type& value) {
        emplace_back(value);
    }

    ///@brief Appends value, overwriting the oldest element if full.
    void push_back(value_type&& value) {
        emplace_back(std::move(value));
    }

    ///@brief Constructs a new newest element, overwriting the oldest element if full.
    template<typename... Args>
    void emplace_back(Args&&... args) {
        if (capacity() == 0) {
            return;
        }

        if (full()) {
            storage_[end_] = value_type(std::forward<Args>(args)...);
            front_ = increment_(front_);
            end_ = increment_(end_);
            return;
        }

        if (end_ == storage_.size()) {
            storage_.emplace_back(std::forward<Args>(args)...);
        } else {
            storage_[end_] = value_type(std::forward<Args>(args)...);
        }
        end_ = increment_(end_);
        ++size_;
    }

    ///@brief Removes the oldest element from the buffer.
    void pop_front() {
        VTR_ASSERT_SAFE(!empty());
        front_ = increment_(front_);
        --size_;
        reset_indices_if_empty_();
    }

    ///@brief Removes the newest element from the buffer.
    void pop_back() {
        VTR_ASSERT_SAFE(!empty());
        end_ = decrement_(end_);
        --size_;
        reset_indices_if_empty_();
    }

  private:
    ///@brief Converts a logical index to a physical storage index.
    size_type physical_index_(size_type logical_index) const noexcept {
        VTR_ASSERT_SAFE(capacity() > 0);
        return (front_ + logical_index) % capacity();
    }

    ///@brief Returns the next physical storage index.
    size_type increment_(size_type index) const noexcept {
        VTR_ASSERT_SAFE(capacity() > 0);
        ++index;
        return index == capacity() ? 0 : index;
    }

    ///@brief Returns the previous physical storage index.
    size_type decrement_(size_type index) const noexcept {
        VTR_ASSERT_SAFE(capacity() > 0);
        return index == 0 ? capacity() - 1 : index - 1;
    }

    ///@brief Reserves backing storage when the container provides reserve().
    void reserve_storage_(size_type capacity) {
        if constexpr (requires(storage_type& storage, size_type size) { storage.reserve(size); }) {
            storage_.reserve(capacity);
        }
    }

    ///@brief Resets circular indices when the buffer becomes empty.
    void reset_indices_if_empty_() noexcept {
        if (empty()) {
            front_ = 0;
            end_ = 0;
        }
    }

    storage_type storage_;   ///< Storage slots constructed so far.
    size_type capacity_ = 0; ///< Fixed maximum number of elements.
    size_type front_ = 0;    ///< Physical index of the oldest element.
    size_type end_ = 0;      ///< Physical index where the next element will be written.
    size_type size_ = 0;     ///< Number of logically stored elements.
};

} // namespace vtr
