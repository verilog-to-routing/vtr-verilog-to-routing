#ifndef VTR_LINEAR_MAP_H
#define VTR_LINEAR_MAP_H
#include <vector>
#include <stdexcept>

#include "vtr_sentinels.h"

namespace vtr {

//A std::map-like container which is indexed by K
//
//The main use of this container is to behave like a std::map which is optimized to hold
//mappings between a dense linear range of keys (e.g. vtr::StrongId).
//
//Requires that K be convertable to size_t with the size_t operator (i.e. size_t()), and
//that the conversion results in a linearly increasing index into the underlying vector.
//Also requires that K() return the sentinel value used to mark invalid entries.
//
//If you only need to access the value associated with the key consider using vtr::vector_map
//instead, which provides a similar but more std::vector-like interface.
//
//Note that it is possible to use linear_map with sparse/non-contiguous keys, but this is typically
//memory inefficient as the underlying vector will allocate space for [0..size_t(max_key)-1],
//where max_key is the largest key that has been inserted.
//
//As with a std::vector, it is the caller's responsibility to ensure there is sufficient space
//when a given index/key before it is accessed. The exception to this are the find() and insert()
//methods which handle non-existing keys gracefully.
template<class K, class T, class Sentinel = DefaultSentinel<K>>
class linear_map {
  public:
    typedef K key_type;
    typedef T mapped_type;
    typedef std::pair<K, T> value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef typename std::vector<value_type>::iterator iterator;
    typedef typename std::vector<value_type>::const_iterator const_iterator;
    typedef typename std::vector<value_type>::reverse_iterator reverse_iterator;
    typedef typename std::vector<value_type>::const_reverse_iterator const_reverse_iterator;
    typedef typename std::vector<value_type>::difference_type difference_type;
    typedef typename std::vector<value_type>::size_type size_type;

  public:
    //Standard big 5
    linear_map() = default;
    linear_map(const linear_map&) = default;
    linear_map(linear_map&&) = default;
    linear_map& operator=(const linear_map&) = default;
    linear_map& operator=(linear_map&&) = default;

    linear_map(size_t num_keys)
        : vec_(num_keys, std::make_pair(sentinel(), T())) //Initialize all with sentinel values
    {}

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

    mapped_type& operator[](const key_type& key) {
        auto iter = find(key);
        if (iter == end()) {
            //Not found, create it
            iter = insert(std::make_pair(key, mapped_type())).first;
        }

        return iter->second;
    }

    mapped_type& at(const key_type& key) {
        return const_cast<mapped_type&>(const_cast<const linear_map*>(this)->at(key));
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
        auto iter = find(value.first);
        if (iter != end()) {
            //Found existing
            return std::make_pair(iter, false);
        } else {
            //Insert
            size_t index = size_t(value.first);

            if (index >= vec_.size()) {
                //Make space, initialize empty slots with sentinel values
                vec_.resize(index + 1, std::make_pair(sentinel(), T()));
            }

            vec_[index] = value;

            return std::make_pair(vec_.begin() + index, true);
        }
    }

    //Insert range
    template<class InputIterator>
    void insert(InputIterator first, InputIterator last) {
        for (InputIterator iter = first; iter != last; ++iter) {
            insert(*iter);
        }
    }

    //Erase by key
    void erase(const key_type& key) {
        auto iter = find(key);
        if (iter != end()) {
            erase(iter);
        }
    }

    //Erase at iterator
    void erase(const_iterator position) {
        iterator pos = convert_to_iterator(position);
        pos->first = sentinel(); //Mark invalid
    }

    //Erase range
    void erase(const_iterator first, const_iterator last) {
        for (auto iter = first; iter != last; ++iter) {
            erase(iter);
        }
    }

    void swap(linear_map& other) { std::swap(vec_, other.vec_); }

    void clear() { vec_.clear(); }

    template<class... Args>
    std::pair<iterator, bool> emplace(const key_type& key, Args&&... args) {
        auto iter = find(key);
        if (iter != end()) {
            //Found
            return std::make_pair(iter, false);
        } else {
            //Emplace
            size_t index = size_t(key);

            if (index >= vec_.size()) {
                //Make space, initialize empty slots with sentinel values
                vec_.resize(index + 1, value_type(sentinel(), T()));
            }

            vec_[index] = value_type(key, std::forward<Args>(args)...);

            return std::make_pair(vec_.begin() + index, true);
        }
    }

    void reserve(size_type n) { vec_.reserve(n); }
    void shrink_to_fit() { vec_.shrink_to_fit(); }

    iterator find(const key_type& key) {
        const_iterator const_iter = const_cast<const linear_map*>(this)->find(key);
        return convert_to_iterator(const_iter);
    }

    const_iterator find(const key_type& key) const {
        size_t index = size_t(key);

        if (index < vec_.size() && vec_[index].first != sentinel()) {
            return vec_.begin() + index;
        }
        return end();
    }

    size_type count(const key_type& key) const {
        return (find(key) == end()) ? 0 : 1;
    }

    iterator lower_bound(const key_type& key) {
        const_iterator const_iter = const_cast<const linear_map*>(this)->lower_bound(key);
        return convert_to_iterator(const_iter);
    }

    const_iterator lower_bound(const key_type& key) const {
        return find(key);
    }

    iterator upper_bound(const key_type& key) {
        const_iterator const_iter = const_cast<const linear_map*>(this)->upper_bound(key);
        return convert_to_iterator(const_iter);
    }

    const_iterator upper_bound(const key_type& key) const {
        auto iter = find(key);
        return (iter != end()) ? iter + 1 : end();
    }

    std::pair<iterator, iterator> equal_range(const key_type& key) {
        auto const_iter_pair = const_cast<const linear_map*>(this)->equal_range(key);
        return std::pair<iterator, iterator>(iterator(const_iter_pair.first), iterator(const_iter_pair.second));
    }

    std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
        auto lb_iter = lower_bound(key);
        auto ub_iter = upper_bound(key);
        return (lb_iter != end()) ? std::make_pair(lb_iter, ub_iter) : std::make_pair(ub_iter, ub_iter);
    }

    size_type valid_size() const {
        size_t valid_cnt = 0;
        for (const auto& kv : vec_) {
            if (kv.first != sentinel()) {
                ++valid_cnt;
            }
        }
        return valid_cnt;
    }

  public:
    friend void swap(linear_map& lhs, linear_map& rhs) {
        std::swap(lhs.vec_, rhs.vec_);
    }

  private:
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
        //both distance and advance take constant time
        iterator i = begin();
        std::advance(i, std::distance<const_iterator>(i, const_iter));
        return i;
    }

    constexpr K sentinel() const { return Sentinel::INVALID(); }

  private:
    std::vector<value_type> vec_;
};

} // namespace vtr
#endif
