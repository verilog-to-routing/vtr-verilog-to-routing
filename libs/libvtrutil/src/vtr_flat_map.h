#ifndef VTR_FLAT_MAP
#define VTR_FLAT_MAP
#include <functional>
#include <vector>
#include <algorithm>
#include <stdexcept>

#include "vtr_assert.h"

namespace vtr {

//Forward declaration
template<class K, class V, class Compare = std::less<K>>
class flat_map;

template<class K, class V, class Compare = std::less<K>>
class flat_map2;

//Helper function to create a flat map from a vector of pairs
//without haveing to explicity specify the key and value types
template<class K, class V>
flat_map<K, V> make_flat_map(std::vector<std::pair<K, V>> vec) {
    return flat_map<K, V>(vec);
}

//
// flat_map is a (nearly) std::map compatible container which uses a vector
// as it's underlying storage. Internally the stored elements are kept sorted
// allowing efficient look-up in O(logN) time via binary search.
//
// This container is typically useful in the following scenarios:
//    * Reduced memory usage if key/value are small (std::map needs to store pointers to
//      other BST nodes which can add substantial overhead for small keys/values)
//    * Faster search/iteration by exploiting data locality (all elments are in continguous
//      memory enabling better spatial locality)
//
// The container deviates from the behaviour of std::map in the following important ways:
//    * Insertion/erase takes O(N) instead of O(logN) time
//    * Iterators may be invalidated on insertion/erase (i.e. if the vector is reallocated)
//
// The slow insertion/erase performance makes this container poorly suited to maps that
// frequently add/remove new keys. If this is required you likely want std::map or
// std::unordered_map. However if the map is constructed once and then repeatedly quieried,
// consider using the range or vector-based constructors which initializes the flat_map in
// O(NlogN) time.
//
template<class K, class T, class Compare>
class flat_map {
  public:
    typedef K key_type;
    typedef T mapped_type;
    typedef std::pair<K, T> value_type;
    typedef Compare key_compare;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef typename std::vector<value_type>::iterator iterator;
    typedef typename std::vector<value_type>::const_iterator const_iterator;
    typedef typename std::vector<value_type>::reverse_iterator reverse_iterator;
    typedef typename std::vector<value_type>::const_reverse_iterator const_reverse_iterator;
    typedef typename std::vector<value_type>::difference_type difference_type;
    typedef typename std::vector<value_type>::size_type size_type;

    class value_compare;

  public:
    //Standard big 5
    flat_map() = default;
    flat_map(const flat_map&) = default;
    flat_map(flat_map&&) = default;
    flat_map& operator=(const flat_map&) = default;
    flat_map& operator=(flat_map&&) = default;

    //range constructor
    template<class InputIterator>
    flat_map(InputIterator first, InputIterator last) {
        //Copy the values
        std::copy(first, last, std::back_inserter(vec_));

        sort();
        uniquify();
    }

    //direct vector constructor
    flat_map(std::vector<value_type> values) {
        //By moving the values this should be more efficient
        //than the range constructor which must copy each element
        vec_ = std::move(values);

        sort();
        uniquify();
    }

    iterator begin() { return vec_.begin(); }
    const_iterator begin() const { return vec_.begin(); }
    iterator end() { return vec_.end(); }
    const_iterator end() const { return vec_.end(); }
    reverse_iterator rbegin() { return vec_.rbegin(); }
    const_reverse_iterator rbegin() const { return vec_.rbegin(); }
    reverse_iterator rend() { return vec_.rend(); }
    const_reverse_iterator rend() const { return vec_.rend(); }
    const_iterator cbegin() const { return vec_.begin(); }
    const_iterator cend() const { return vec_.end(); }
    const_reverse_iterator crbegin() const { return vec_.rbegin(); }
    const_reverse_iterator crend() const { return vec_.rend(); }

    bool empty() const { return vec_.empty(); }
    size_type size() const { return vec_.size(); }
    size_type max_size() const { return vec_.max_size(); }

    const mapped_type& operator[](const key_type& key) const {
        auto iter = find(key);
        if (iter == end()) {
            //Not found
            throw std::out_of_range("Invalid key");
        }

        return iter->second;
    }

    mapped_type& operator[](const key_type& key) {
        auto iter = find(key);
        if (iter == end()) {
            //Not found
            iter = insert(std::make_pair(key, mapped_type())).first;
        }

        return iter->second;
    }

    mapped_type& at(const key_type& key) {
        return const_cast<mapped_type&>(const_cast<const flat_map*>(this)->at(key));
    }

    const mapped_type& at(const key_type& key) const {
        auto iter = find(key);
        if (iter == end()) {
            throw std::out_of_range("Invalid key");
        }
        return iter->second;
    }

    //Insert value
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

    //Insert value with position hint
    iterator insert(const_iterator position, const value_type& value) {
        //In a legal position
        VTR_ASSERT(position == begin() || value_comp()(*(position - 1), value));
        VTR_ASSERT((size() > 0 && position == --end()) || position == end() || !value_comp()(*(position + 1), value));

        iterator iter = vec_.insert(position, value);

        return iter;
    }

    //Insert range
    template<class InputIterator>
    void insert(InputIterator first, InputIterator last) {
        vec_.insert(vec_.end(), first, last);

        //TODO: could be more efficient
        sort();
        uniquify();
    }

    //Erase by key
    void erase(const key_type& key) {
        auto iter = find(key);
        if (iter != end()) {
            vec_.erase(iter);
        }
    }

    //Erase at iterator
    void erase(const_iterator position) {
        vec_.erase(position);
    }

    //Erase range
    void erase(const_iterator first, const_iterator last) {
        vec_.erase(first, last);
    }

    void swap(flat_map& other) { std::swap(*this, other); }

    void clear() { vec_.clear(); }

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

    template<class... Args>
    iterator emplace_hint(const_iterator position, Args&&... args) {
        return vec_.emplace(position, std::forward<Args>(args)...);
    }

    void reserve(size_type n) { vec_.reserve(n); }
    void shrink_to_fit() { vec_.shrink_to_fit(); }

    key_compare key_comp() const { return key_compare(); }
    value_compare value_comp() const { return value_compare(key_comp()); }

    iterator find(const key_type& key) {
        const_iterator const_iter = const_cast<const flat_map*>(this)->find(key);
        return convert_to_iterator(const_iter);
    }

    const_iterator find(const key_type& key) const {
        auto iter = lower_bound(key);
        if (iter != end() && keys_equivalent(iter->first, key)) {
            //Found
            return iter;
        }
        return end();
    }

    size_type count(const key_type& key) const {
        return (find(key) == end()) ? 0 : 1;
    }

    iterator lower_bound(const key_type& key) {
        const_iterator const_iter = const_cast<const flat_map*>(this)->lower_bound(key);
        return convert_to_iterator(const_iter);
    }

    const_iterator lower_bound(const key_type& key) const {
        return std::lower_bound(begin(), end(), key, value_comp());
    }

    iterator upper_bound(const key_type& key) {
        const_iterator const_iter = const_cast<const flat_map*>(this)->upper_bound(key);
        return convert_to_iterator(const_iter);
    }

    const_iterator upper_bound(const key_type& key) const {
        return std::upper_bound(begin(), end(), key, value_comp());
    }

    std::pair<iterator, iterator> equal_range(const key_type& key) {
        auto const_iter_pair = const_cast<const flat_map*>(this)->equal_range(key);
        return std::pair<iterator, iterator>(iterator(const_iter_pair.first), iterator(const_iter_pair.second));
    }

    std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
        return std::equal_range(begin(), end(), key);
    }

  public:
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
        //This is a work around for the fact that there is no conversion between
        //a const_iterator and iterator.
        //
        //We intiailize i to the start of the container and then advance it by
        //the distance to const_iter. The resulting i points to the same element
        //as const_iter
        //
        //Note that to be able to call std::distance with an iterator and
        //const_iterator we need to specify the type as const_iterator (relying
        //on the implicit conversion from iterator to const_iterator for i)
        //
        //Since the iterators are really vector (i.e. random-access) iterators
        //this takes constant time
        iterator i = begin();
        std::advance(i, std::distance<const_iterator>(i, const_iter));
        return i;
    }

  private:
    std::vector<value_type> vec_;
};

//Like flat_map, but operator[] never inserts and directly returns the mapped value
template<class K, class T, class Compare>
class flat_map2 : public flat_map<K, T, Compare> {
  public:
    const T& operator[](const K& key) const {
        auto itr = this->find(key);
        if (itr == this->end()) {
            throw std::logic_error("Key not found");
        }
        return itr->second;
    }

    T& operator[](const K& key) {
        return const_cast<T&>(const_cast<const flat_map2*>(this)->operator[](key));
    }
};

template<class K, class T, class Compare>
class flat_map<K, T, Compare>::value_compare {
    friend class flat_map;

  public:
    bool operator()(const value_type& x, const value_type& y) const {
        return comp(x.first, y.first);
    }

    //For std::lower_bound/std::upper_bound
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
