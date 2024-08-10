#ifndef VTR_VECTOR
#define VTR_VECTOR
#include <vector>
#include <cstddef>
#include <iterator>
#include <algorithm>
#include "vtr_range.h"

namespace vtr {

/**
 * @brief A std::vector container which is indexed by K (instead of size_t).
 *
 * The main use of this container is to behave like a std::vector which is
 * indexed by a vtr::StrongId. It assumes that K is explicitly convertable to size_t
 * (i.e. via operator size_t()), and can be explicitly constructed from a size_t.
 *
 * It includes all the following std::vector functions:
 *      - begin
 *      - cbegin
 *      - cend
 *      - crbegin
 *      - crend
 *      - end
 *      - rbegin
 *      - rend
 *      - capacity
 *      - empty
 *      - max_size
 *      - reserve
 *      - resize
 *      - shrink_to_fit
 *      - size
 *      - back
 *      - front
 *      - assign
 *      - clear
 *      - emplace
 *      - emplace_back
 *      - erase
 *      - get_allocator
 *      - insert
 *      - pop_back
 *      - push_back
 *
 * If you need more std::map-like (instead of std::vector-like) behaviour see
 * vtr::vector_map.
 */
template<typename K, typename V, typename Allocator = std::allocator<V>>
class vector : private std::vector<V, Allocator> {
    using storage = std::vector<V, Allocator>;

  public:
    typedef K key_type;

    class key_iterator;
    typedef vtr::Range<key_iterator> key_range;

    class pair_iterator;
    typedef vtr::Range<pair_iterator> pair_range;

  public:
    //Pass through std::vector's types
    using typename storage::allocator_type;
    using typename storage::const_iterator;
    using typename storage::const_pointer;
    using typename storage::const_reference;
    using typename storage::const_reverse_iterator;
    using typename storage::difference_type;
    using typename storage::iterator;
    using typename storage::pointer;
    using typename storage::reference;
    using typename storage::reverse_iterator;
    using typename storage::size_type;
    using typename storage::value_type;

    //Pass through storagemethods
    using std::vector<V, Allocator>::vector;

    using storage::begin;
    using storage::cbegin;
    using storage::cend;
    using storage::crbegin;
    using storage::crend;
    using storage::end;
    using storage::rbegin;
    using storage::rend;

    using storage::capacity;
    using storage::empty;
    using storage::max_size;
    using storage::reserve;
    using storage::resize;
    using storage::shrink_to_fit;
    using storage::size;

    using storage::back;
    using storage::front;

    using storage::assign;
    using storage::clear;
    using storage::emplace;
    using storage::emplace_back;
    using storage::erase;
    using storage::get_allocator;
    using storage::insert;
    using storage::pop_back;
    using storage::push_back;

    /*
     * We can't using-forward storage::data, as it might not exist
     * in the particular specialization (typically: vector<bool>)
     * causing compiler complains.
     * Instead, implement it as inline forwarding method whose
     * compilation is deferred to when it is actually requested.
     */
    ///@brief Returns a pointer to the vector's data
    inline V* data() { return storage::data(); }
    ///@brief Returns a pointer to the vector's data (immutable)
    inline const V* data() const { return storage::data(); }

    /*
     * Don't include operator[] and at() from std::vector,
     *
     * since we redine them to take key_type instead of size_t
     */
    ///@brief [] operator
    reference operator[](const key_type id) {
        auto i = size_t(id);
        return storage::operator[](i);
    }
    ///@brief [] operator immutable
    const_reference operator[](const key_type id) const {
        auto i = size_t(id);
        return storage::operator[](i);
    }
    ///@brief at() operator
    reference at(const key_type id) {
        auto i = size_t(id);
        return storage::at(i);
    }
    ///@brief at() operator immutable
    const_reference at(const key_type id) const {
        auto i = size_t(id);
        return storage::at(i);
    }

    // We must re-define swap to avoid inaccessible base class errors
    ///@brief swap function
    void swap(vector<K, V, Allocator>& other) {
        std::swap(*this, other);
    }

    ///@brief Returns a range containing the keys
    key_range keys() const {
        return vtr::make_range(key_begin(), key_end());
    }

    /**
     * @brief Returns a range containing the key-value pairs.
     *
     * This function returns a range object that represents the sequence of key-value
     * pairs within the vector. The range can be used to iterate over the pairs using
     * standard range-based loops or algorithms.
     *
     * @return A `pair_range` object representing the range of key-value pairs.
     */
    pair_range pairs() const {
        return vtr::make_range(pair_begin(), pair_end());
    }

  public:
    /**
     * @brief Iterator class which is convertable to the key_type
     *
     * This allows end-users to call the parent class's keys() member
     * to iterate through the keys with a range-based for loop
     */
    class key_iterator {
      public:
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = key_type;
        using pointer = key_type*;
        using reference = key_type&;

        ///@brief constructor
        key_iterator(value_type init)
            : value_(init) {}

        /*
         * vtr::vector assumes that the key time is convertable to size_t.
         *
         * It also assumes all the underlying IDs are zero-based and contiguous. That means
         * we can just increment the underlying Id to build the next key.
         */
        ///@brief ++ operator
        key_iterator& operator++() {
            value_ = value_type(size_t(value_) + 1);
            return *this;
        }
        ///@brief decrement operator
        key_iterator& operator--() {
            value_ = value_type(size_t(value_) - 1);
            return *this;
        }
        ///@brief dereference operator
        reference operator*() { return value_; }
        ///@brief -> operator
        pointer operator->() { return &value_; }

        ///@brief == operator
        friend bool operator==(const key_iterator& lhs, const key_iterator& rhs) { return lhs.value_ == rhs.value_; }
        ///@brief != operator
        friend bool operator!=(const key_iterator& lhs, const key_iterator& rhs) { return !(lhs == rhs); }

      private:
        value_type value_;
    };

        /**
         * @brief A bidirectional iterator for a vtr:vector object.
         *
         * The `pair_iterator` class provides a way to iterate over key-value pairs
         * within a vtr::vector container. It supports bidirectional iteration,
         * allowing the user to traverse the container both forwards and backwards.
         */
        class pair_iterator {
          public:
            using iterator_category = std::bidirectional_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = std::pair<key_type, V>;
            using pointer = value_type*;
            using reference = value_type&;

            /// @brief constructor
            pair_iterator(vector<K, V, Allocator>& vec, key_type init)
                : vec_(vec), value_(init, vec[init]) {}

            /// @brief ++ operator
            pair_iterator& operator++() {
                value_ = std::make_pair(key_type(size_t(value_.first) + 1), vec_[key_type(size_t(value_.first) + 1)]);
                return *this;
            }
            /// @brief -- operator
            pair_iterator& operator--() {
                value_ = std::make_pair(key_type(size_t(value_.first) - 1), vec_[key_type(size_t(value_.first) - 1)]);
                return *this;
            }
            /// @brief dereference operator
            reference operator*() { return value_; }
            /// @brief -> operator
            pointer operator->() { return &value_; }

            /// @brief == operator
            friend bool operator==(const pair_iterator& lhs, const pair_iterator& rhs) { return lhs.value_.first == rhs.value_.first; }
            /// @brief != operator
            friend bool operator!=(const pair_iterator& lhs, const pair_iterator& rhs) { return !(lhs == rhs); }

          private:
            /// @brief Reference to the vector of key-value pairs.
            vector<K, V, Allocator>& vec_;
            // @brief The current key-value pair being pointed to by the iterator.
            value_type value_;
        };

  private:
    key_iterator key_begin() const { return key_iterator(key_type(0)); }
    key_iterator key_end() const { return key_iterator(key_type(size())); }

    pair_iterator pair_begin() const { return pair_iterator(*const_cast<vector<K, V, Allocator>*>(this), key_type(0)); }
    pair_iterator pair_end() const { return pair_iterator(*const_cast<vector<K, V, Allocator>*>(this), key_type(size())); }
};

} // namespace vtr
#endif
