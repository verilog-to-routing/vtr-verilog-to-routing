#ifndef VTR_VECTOR
#define VTR_VECTOR
#include <vector>
#include <cstddef>
#include <iterator>
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

  public:
    /**
     * @brief Iterator class which is convertable to the key_type
     *
     * This allows end-users to call the parent class's keys() member
     * to iterate through the keys with a range-based for loop
     */
    class key_iterator : public std::iterator<std::bidirectional_iterator_tag, key_type> {
      public:
        ///@brief We use the intermediate type my_iter to avoid a potential ambiguity for which clang generates errors and warnings
        using my_iter = typename std::iterator<std::bidirectional_iterator_tag, K>;
        using typename my_iter::iterator;
        using typename my_iter::pointer;
        using typename my_iter::reference;
        using typename my_iter::value_type;

        ///@brief constructor
        key_iterator(key_iterator::value_type init)
            : value_(init) {}

        /*
         * vtr::vector assumes that the key time is convertable to size_t.
         *
         * It also assumes all the underlying IDs are zero-based and contiguous. That means
         * we can just increment the underlying Id to build the next key.
         */
        ///@brief ++ operator
        key_iterator operator++() {
            value_ = value_type(size_t(value_) + 1);
            return *this;
        }
        ///@brief decrement operator
        key_iterator operator--() {
            value_ = value_type(size_t(value_) - 1);
            return *this;
        }
        ///@brief dereference oeprator
        reference operator*() { return value_; }
        ///@brief -> operator
        pointer operator->() { return &value_; }

        ///@brief == operator
        friend bool operator==(const key_iterator lhs, const key_iterator rhs) { return lhs.value_ == rhs.value_; }
        ///@brief != operator
        friend bool operator!=(const key_iterator lhs, const key_iterator rhs) { return !(lhs == rhs); }

      private:
        value_type value_;
    };

  private:
    key_iterator key_begin() const { return key_iterator(key_type(0)); }
    key_iterator key_end() const { return key_iterator(key_type(size())); }
};

} // namespace vtr
#endif
