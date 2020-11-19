#ifndef VTR_FLAT_MAP
#define VTR_FLAT_MAP
#include <functional>
#include <vector>
#include <algorithm>
#include <stdexcept>

#include "vtr_assert.h"

namespace vtr {

//Forward declaration
template<class K, class V, class Compare = std::less<K>, class Storage = std::vector<std::pair<K, V>>>
class flat_map;

template<class K, class V, class Compare = std::less<K>, class Storage = std::vector<std::pair<K, V>>>
class flat_map2;

/**
 * @brief A function to create a flat map
 *
 * Helper function to create a flat map from a vector of pairs
 * without haveing to explicity specify the key and value types
 */
template<class K, class V>
flat_map<K, V> make_flat_map(std::vector<std::pair<K, V>>&& vec) {
    return flat_map<K, V>(std::move(vec));
}

///@brief Same as make_flat_map but for flat_map2
template<class K, class V>
flat_map2<K, V> make_flat_map2(std::vector<std::pair<K, V>>&& vec) {
    return flat_map2<K, V>(std::move(vec));
}

/**
 * @brief flat_map is a (nearly) std::map compatible container 
 * 
 * It uses a vector as it's underlying storage. Internally the stored elements 
 * are kept sorted allowing efficient look-up in O(logN) time via binary search.
 *
 *
 * This container is typically useful in the following scenarios:
 *    - Reduced memory usage if key/value are small (std::map needs to store pointers to
 *      other BST nodes which can add substantial overhead for small keys/values)
 *    - Faster search/iteration by exploiting data locality (all elments are in continguous
 *      memory enabling better spatial locality)
 *
 * The container deviates from the behaviour of std::map in the following important ways:
 *    - Insertion/erase takes O(N) instead of O(logN) time
 *    - Iterators may be invalidated on insertion/erase (i.e. if the vector is reallocated)
 *
 * The slow insertion/erase performance makes this container poorly suited to maps that
 * frequently add/remove new keys. If this is required you likely want std::map or
 * std::unordered_map. However if the map is constructed once and then repeatedly quieried,
 * consider using the range or vector-based constructors which initializes the flat_map in
 * O(NlogN) time.
 */
template<class K, class T, class Compare, class Storage>
class flat_map {
  public:
    typedef K key_type;
    typedef T mapped_type;
    typedef std::pair<K, T> value_type;
    typedef Compare key_compare;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef typename Storage::iterator iterator;
    typedef typename Storage::const_iterator const_iterator;
    typedef typename Storage::reverse_iterator reverse_iterator;
    typedef typename Storage::const_reverse_iterator const_reverse_iterator;
    typedef typename Storage::difference_type difference_type;
    typedef typename Storage::size_type size_type;

    class value_compare;

  public:
    ///@brief Standard constructors
    flat_map() = default;
    flat_map(const flat_map&) = default;
    flat_map(flat_map&&) = default;
    flat_map& operator=(const flat_map&) = default;
    flat_map& operator=(flat_map&&) = default;

    ///@brief range constructor
    template<class InputIterator>
    flat_map(InputIterator first, InputIterator last) {
        // Copy the values
        std::copy(first, last, std::back_inserter(vec_));

        sort();
        uniquify();
    }

    ///@brief direct vector constructor
    explicit flat_map(Storage&& values) {
        assign(std::move(values));
    }

    /**
     * @brief Move the values
     * 
     * Should be more efficient than the range constructor which 
     * must copy each element
     */
    void assign(Storage&& values) {
        vec_ = std::move(values);

        sort();
        uniquify();
    }

    ///@brief By moving the values this should be more efficient than the range constructor which must copy each element
    void assign_sorted(Storage&& values) {
        vec_ = std::move(values);
        if (vec_.size() > 1) {
            for (size_t i = 0; i < vec_.size() - 1; ++i) {
                VTR_ASSERT_SAFE(vec_[i].first < vec_[i + 1].first);
            }
        }
    }

    ///@brief Return an iterator pointing to the first element in the sequence:
    iterator begin() { return vec_.begin(); }

    ///@brief Return a constant iterator pointing to the first element in the sequence:
    const_iterator begin() const { return vec_.begin(); }

    ///@brief Returns an iterator referring to the past-the-end element in the vector container.
    iterator end() { return vec_.end(); }

    ///@brief Returns a constant iterator referring to the past-the-end element in the vector container.
    const_iterator end() const { return vec_.end(); }

    ///@brief Returns a reverse iterator which points to the last element of the map.
    reverse_iterator rbegin() { return vec_.rbegin(); }

    ///@brief Returns a constant reverse iterator which points to the last element of the map.
    const_reverse_iterator rbegin() const { return vec_.rbegin(); }

    ///@brief Returns a reverse iterator pointing to the theoretical element preceding the first element in the vector (which is considered its reverse end).
    reverse_iterator rend() { return vec_.rend(); }

    ///@brief Returns a constant reverse iterator pointing to the theoretical element preceding the first element in the vector (which is considered its reverse end).
    const_reverse_iterator rend() const { return vec_.rend(); }

    ///@brief Returns a constant_iterator to the first element in the underlying vector
    const_iterator cbegin() const { return vec_.begin(); }

    ///@brief Returns a const_iterator pointing to the past-the-end element in the container.
    const_iterator cend() const { return vec_.end(); }

    ///@brief Returns a const_reverse_iterator pointing to the last element in the container (i.e., its reverse beginning).
    const_reverse_iterator crbegin() const { return vec_.rbegin(); }

    ///@brief Returns a const_reverse_iterator pointing to the theoretical element preceding the first element in the container (which is considered its reverse end).
    const_reverse_iterator crend() const { return vec_.rend(); }

    ///@brief Return true if the underlying vector is empty
    bool empty() const { return vec_.empty(); }

    ///@brief Return the container size
    size_type size() const { return vec_.size(); }

    ///@brief Return the underlying vector's max size
    size_type max_size() const { return vec_.max_size(); }

    ///@brief The constant version of operator []
    const mapped_type& operator[](const key_type& key) const {
        auto iter = find(key);
        if (iter == end()) {
            //Not found
            throw std::out_of_range("Invalid key");
        }

        return iter->second;
    }

    ///@brief operator []
    mapped_type& operator[](const key_type& key) {
        auto iter = std::lower_bound(begin(), end(), key, value_comp());
        if (iter == end()) {
            // The new element should be placed at the end, so do so.
            vec_.emplace_back(std::make_pair(key, mapped_type()));
            return vec_.back().second;
        } else {
            if (iter->first == key) {
                // The element already exists, return it.
                return iter->second;
            } else {
                // The element does not exist, insert such that vector remains
                // sorted.
                iter = vec_.emplace(iter, std::make_pair(key, mapped_type()));
                return iter->second;
            }
        }
    }

    ///@brief operator at()
    mapped_type& at(const key_type& key) {
        return const_cast<mapped_type&>(const_cast<const flat_map*>(this)->at(key));
    }

    ///@brief The constant version of at() operator
    const mapped_type& at(const key_type& key) const {
        auto iter = find(key);
        if (iter == end()) {
            throw std::out_of_range("Invalid key");
        }
        return iter->second;
    }

    ///@brief Insert value
    std::pair<iterator, bool> insert(const value_type& value) {
        auto iter = lower_bound(value.first);
        if (iter != end() && keys_equivalent(iter->first, value.first)) {
            //Found existing
            return std::make_pair(iter, false);
        } else {
            //Insert
            iter = insert(iter, value);

            return std::make_pair(iter, true);
        }
    }

    ///@brief Emplace function
    std::pair<iterator, bool> emplace(const value_type&& value) {
        auto iter = lower_bound(value.first);
        if (iter != end() && keys_equivalent(iter->first, value.first)) {
            //Found existing
            return std::make_pair(iter, false);
        } else {
            //Emplace
            iter = emplace(iter, value);

            return std::make_pair(iter, true);
        }
    }

    ///@brief Insert value with position hint
    iterator insert(const_iterator position, const value_type& value) {
        //In a legal position
        VTR_ASSERT(position == begin() || value_comp()(*(position - 1), value));
        VTR_ASSERT((size() > 0 && (position + 1) == end()) || position == end() || !value_comp()(*(position + 1), value));

        iterator iter = vec_.insert(position, value);

        return iter;
    }

    ///@brief Emplace value with position hint
    iterator emplace(const_iterator position, const value_type& value) {
        //In a legal position
        VTR_ASSERT(position == begin() || value_comp()(*(position - 1), value));
        VTR_ASSERT((size() > 0 && (position + 1) == end()) || position == end() || !value_comp()(*(position + 1), value));

        iterator iter = vec_.emplace(position, value);

        return iter;
    }

    ///@brief Insert range
    template<class InputIterator>
    void insert(InputIterator first, InputIterator last) {
        vec_.insert(vec_.end(), first, last);

        //TODO: could be more efficient
        sort();
        uniquify();
    }

    ///@brief Erase by key
    void erase(const key_type& key) {
        auto iter = find(key);
        if (iter != end()) {
            vec_.erase(iter);
        }
    }

    ///@brief Erase at iterator
    void erase(const_iterator position) {
        vec_.erase(position);
    }

    ///@brief Erase range
    void erase(const_iterator first, const_iterator last) {
        vec_.erase(first, last);
    }

    ///@brief swap two flat maps
    void swap(flat_map& other) { std::swap(*this, other); }

    ///@brief clear the flat map
    void clear() { vec_.clear(); }

    ///@brief templated emplace function
    template<class... Args>
    iterator emplace(const key_type& key, Args&&... args) {
        auto iter = lower_bound(key);
        if (iter != end() && keys_equivalent(iter->first, key)) {
            //Found
            return std::make_pair(iter, false);
        } else {
            //Emplace
            iter = emplace_hint(iter, key, std::forward<Args>(args)...);
            return std::make_pair(iter, true);
        }
    }

    ///@brief templated emplace_hint function
    template<class... Args>
    iterator emplace_hint(const_iterator position, Args&&... args) {
        return vec_.emplace(position, std::forward<Args>(args)...);
    }

    ///@brief Reserve a minimum capacity for the underlying vector
    void reserve(size_type n) { vec_.reserve(n); }

    ///@brief Reduce the capacity of the underlying vector to fit its size
    void shrink_to_fit() { vec_.shrink_to_fit(); }

    ///@brief
    key_compare key_comp() const { return key_compare(); }

    ///@brief
    value_compare value_comp() const { return value_compare(key_comp()); }

    ///@brief Find a key and return an iterator to the found key
    iterator find(const key_type& key) {
        const_iterator const_iter = const_cast<const flat_map*>(this)->find(key);
        return convert_to_iterator(const_iter);
    }

    ///@brief Find a key and return a constant iterator to the found key
    const_iterator find(const key_type& key) const {
        auto iter = lower_bound(key);
        if (iter != end() && keys_equivalent(iter->first, key)) {
            //Found
            return iter;
        }
        return end();
    }

    ///@brief Return the count of occurances of a key
    size_type count(const key_type& key) const {
        return (find(key) == end()) ? 0 : 1;
    }

    ///@brief lower bound function
    iterator lower_bound(const key_type& key) {
        const_iterator const_iter = const_cast<const flat_map*>(this)->lower_bound(key);
        return convert_to_iterator(const_iter);
    }

    ///@brief Return a constant iterator to the lower bound
    const_iterator lower_bound(const key_type& key) const {
        return std::lower_bound(begin(), end(), key, value_comp());
    }

    ///@brief upper bound function
    iterator upper_bound(const key_type& key) {
        const_iterator const_iter = const_cast<const flat_map*>(this)->upper_bound(key);
        return convert_to_iterator(const_iter);
    }

    ///@brief Return a constant iterator to the upper bound
    const_iterator upper_bound(const key_type& key) const {
        return std::upper_bound(begin(), end(), key, value_comp());
    }

    ///@brief Returns a range containing all elements equivalent to "key"
    std::pair<iterator, iterator> equal_range(const key_type& key) {
        auto const_iter_pair = const_cast<const flat_map*>(this)->equal_range(key);
        return std::pair<iterator, iterator>(iterator(const_iter_pair.first), iterator(const_iter_pair.second));
    }

    ///@brief Returns a constant range containing all elements equivalent to "key"
    std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
        return std::equal_range(begin(), end(), key);
    }

  public:
    ///@brief Swaps 2 flat maps
    friend void swap(flat_map& lhs, flat_map& rhs) { std::swap(lhs.vec_, rhs.vec_); }

  private:
    bool keys_equivalent(const key_type& lhs, const key_type& rhs) const {
        return !key_comp()(lhs, rhs) && !key_comp()(rhs, lhs);
    }

    void sort() {
        std::sort(vec_.begin(), vec_.end(), value_comp());
    }

    void uniquify() {
        //Uniquify
        auto key_equal_pred = [this](const value_type& lhs, const value_type& rhs) {
            return !value_comp()(lhs, rhs) && !value_comp()(rhs, lhs);
        };
        vec_.erase(std::unique(vec_.begin(), vec_.end(), key_equal_pred), vec_.end());
    }

    iterator convert_to_iterator(const_iterator const_iter) {
        /*
         * A work around as there is no conversion betweena const_iterator and iterator.
         *
         * We intiailize i to the start of the container and then advance it by
         * the distance to const_iter. The resulting i points to the same element
         * as const_iter
         * 
         * Note that to be able to call std::distance with an iterator and
         * const_iterator we need to specify the type as const_iterator (relying
         * on the implicit conversion from iterator to const_iterator for i)
         *
         * Since the iterators are really vector (i.e. random-access) iterators
         * this takes constant time
         */
        iterator i = begin();
        std::advance(i, std::distance<const_iterator>(i, const_iter));
        return i;
    }

  private:
    Storage vec_;
};

/**
 * @brief Another flat_map container
 *
 * Like flat_map, but operator[] never inserts and directly returns the mapped value
 */
template<class K, class T, class Compare, class Storage>
class flat_map2 : public flat_map<K, T, Compare, Storage> {
  public:
    ///@brief Constructor
    flat_map2() {}
    explicit flat_map2(std::vector<typename flat_map2<K, T, Compare, Storage>::value_type>&& values)
        : flat_map<K, T, Compare>(std::move(values)) {}

    ///@brief const [] operator
    const T& operator[](const K& key) const {
        auto itr = this->find(key);
        if (itr == this->end()) {
            throw std::logic_error("Key not found");
        }
        return itr->second;
    }

    ///@brief [] operator
    T& operator[](const K& key) {
        return const_cast<T&>(const_cast<const flat_map2*>(this)->operator[](key));
    }
};

///@brief A class to perform the comparison operation for the flat map
template<class K, class T, class Compare, class Storage>
class flat_map<K, T, Compare, Storage>::value_compare {
    friend class flat_map;

  public:
    bool operator()(const value_type& x, const value_type& y) const {
        return comp(x.first, y.first);
    }

    //For std::lower_bound, std::upper_bound
    bool operator()(const value_type& x, const key_type& y) const {
        return comp(x.first, y);
    }
    bool operator()(const key_type& x, const value_type& y) const {
        return comp(x, y.first);
    }

  private:
    value_compare(Compare c)
        : comp(c) {}

    Compare comp;
};

} // namespace vtr
#endif
