#ifndef VTR_VECTOR_MAP
#define VTR_VECTOR_MAP
#include <vector>

#include "vtr_assert.h"
#include "vtr_sentinels.h"

namespace vtr {

/**
 * @brief A vector-like container which is indexed by K (instead of size_t as in std::vector).
 * 
 * The main use of this container is to behave like a std::vector which is indexed by
 * vtr::StrongId.
 * 
 * Requires that K be convertable to size_t with the size_t operator (i.e. size_t()), and
 * that the conversion results in a linearly increasing index into the underlying vector.
 * 
 * This results in a container that is somewhat similar to a std::map (i.e. converts from one
 * type to another), but requires contiguously ascending (i.e. linear) keys. Unlike std::map
 * only the values are stored (at the specified index/key), reducing memory usage and improving
 * cache locality. Furthermore, operator[] and find() return the value or iterator directly
 * associated with the value (like std::vector) rather than a std::pair (like std::map).
 * insert() takes both the key and value as separate arguments and has no return value.
 * 
 * Additionally, vector_map will silently create values for 'gaps' in the index range (i.e.
 * those elements are initialized with Sentinel::INVALID()).
 * 
 * If you need a fully featured std::map like container without the above differences see
 * vtr::linear_map.
 * 
 * If you do not need std::map-like features see vtr::vector. Note that vtr::vector_map is very similar 
 * to vtr::vector. Unless there is a specific reason that vtr::vector_map is needed, it is better to use vtr::vector.
 * 
 * Note that it is possible to use vector_map with sparse/non-contiguous keys, but this is typically
 * memory inefficient as the underlying vector will allocate space for [0..size_t(max_key)-1],
 * where max_key is the largest key that has been inserted.
 * 
 * As with a std::vector, it is the caller's responsibility to ensure there is sufficient space
 * when a given index/key before it is accessed. The exception to this are the find(), insert() and
 * update() methods which handle non-existing keys gracefully.
 */

template<typename K, typename V, typename Sentinel = DefaultSentinel<V>>
class vector_map {
  public: //Public types
    typedef typename std::vector<V>::const_reference const_reference;
    typedef typename std::vector<V>::reference reference;

    typedef typename std::vector<V>::iterator iterator;
    typedef typename std::vector<V>::const_iterator const_iterator;
    typedef typename std::vector<V>::const_reverse_iterator const_reverse_iterator;

  public:
    ///@brief Constructor
    template<typename... Args>
    vector_map(Args&&... args)
        : vec_(std::forward<Args>(args)...) {}

  public: //Accessors
    ///@brief Returns an iterator referring to the first element in the map container.
    const_iterator begin() const { return vec_.begin(); }
    ///@brief Returns an iterator referring to the past-the-end element in the map container.
    const_iterator end() const { return vec_.end(); }
    ///@begin Returns a reverse iterator pointing to the last element in the container (i.e., its reverse beginning).
    const_reverse_iterator rbegin() const { return vec_.rbegin(); }
    ///@brief Returns a reverse iterator pointing to the theoretical element right before the first element in the map container (which is considered its reverse end).
    const_reverse_iterator rend() const { return vec_.rend(); }

    //Indexing
    ///@brief [] operator immutable
    const_reference operator[](const K n) const {
        size_t index = size_t(n);

        /**
         * Shouldn't check for index >= 0, since size_t is unsigned thus won't be negative
         *
         * A negative input to n would result in an absurdly large number close the maximum size of size_t, and be caught by index < vec_.size()
         * http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3690.pdf chapter 4.7 para 2
         */

        VTR_ASSERT_SAFE_MSG(index < vec_.size(), "Out-of-range index");
        return vec_[index];
    }

    ///@brief Searches the container for an element with a key equivalent to k and returns an iterator to it if found, otherwise it returns an iterator to vector_map::end.
    const_iterator find(const K key) const {
        if (size_t(key) < vec_.size()) {
            return vec_.begin() + size_t(key);
        } else {
            return vec_.end();
        }
    }

    ///@brief Returns the number of elements in the container.
    std::size_t size() const { return vec_.size(); }

    ///@brief Returns true if the container is empty
    bool empty() const { return vec_.empty(); }

    ///@brief Returns true if the container contains key
    bool contains(const K key) const { return size_t(key) < vec_.size(); }
    ///@brief Returns 1 if the container contains key, 0 otherwise
    size_t count(const K key) const { return contains(key) ? 1 : 0; }

  public: //Mutators
    // Delegate potentially overloaded functions to the underlying vector with perfect forwarding
    ///@brief push_back function
    template<typename... Args>
    void push_back(Args&&... args) { vec_.push_back(std::forward<Args>(args)...); }

    ///@brief emplace_back function
    template<typename... Args>
    void emplace_back(Args&&... args) { vec_.emplace_back(std::forward<Args>(args)...); }

    ///@brief resize function
    template<typename... Args>
    void resize(Args&&... args) { vec_.resize(std::forward<Args>(args)...); }

    ///@brief clears the container
    void clear() { vec_.clear(); }

    ///@brief Returns the capacity of the container
    size_t capacity() const { return vec_.capacity(); }
    ///@brief Requests the container to reduce its capacity to fit its size.
    void shrink_to_fit() { vec_.shrink_to_fit(); }

    ///@brief Returns an iterator referring to the first element in the map container.
    iterator begin() { return vec_.begin(); }
    ///@brief Returns an iterator referring to the past-the-end element in the map container.
    iterator end() { return vec_.end(); }

    ///@brief Indexing
    reference operator[](const K n) {
        VTR_ASSERT_SAFE_MSG(size_t(n) < vec_.size(), "Out-of-range index");
        return vec_[size_t(n)];
    }

    ///@brief Returns an iterator to the first element in the container that compares equal to val. If no such element is found, the function returns end().
    iterator find(const K key) {
        if (size_t(key) < vec_.size()) {
            return vec_.begin() + size_t(key);
        } else {
            return vec_.end();
        }
    }

    ///@brief Extends the container by inserting new elements, effectively increasing the container size by the number of elements inserted.
    void insert(const K key, const V value) {
        if (size_t(key) >= vec_.size()) {
            //Resize so key is in range
            vec_.resize(size_t(key) + 1, Sentinel::INVALID());
        }

        //Insert the value
        operator[](key) = value;
    }

    ///@brief Inserts the new key value pair in the container
    void update(const K key, const V value) { insert(key, value); }

    ///@brief Swap (this enables std::swap via ADL)
    friend void swap(vector_map<K, V>& x, vector_map<K, V>& y) {
        std::swap(x.vec_, y.vec_);
    }

  private:
    std::vector<V> vec_;
};

} // namespace vtr
#endif
