#ifndef VTR_FLAT_MAP
#define VTR_FLAT_MAP
#include <functional>
#include <vector>
#include <algorithm>
#include <stdexcept>

namespace vtr {

//Forward delcaration
template<class K, class V, class Compare=std::less<K>>
class flat_map;

//Helper function to create a flat map from a vector of pairs
//without haveing to explicity specify the key and value types
template<class K, class V>
flat_map<K,V> make_flat_map(std::vector<std::pair<K, V>> vec) {
    return flat_map<K,V>(vec);
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
        typedef std::pair<K,T> value_type;
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
        template <class InputIterator>
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

        iterator                begin()             { return vec_.begin(); }
        const_iterator          begin()     const   { return vec_.begin(); }
        iterator                end()               { return vec_.end(); }
        const_iterator          end()       const   { return vec_.end(); }
        reverse_iterator        rbegin()            { return vec_.rbegin(); }
        const_reverse_iterator  rbegin()    const   { return vec_.rbegin(); }
        reverse_iterator        rend()              { return vec_.rend(); }
        const_reverse_iterator  rend()      const   { return vec_.rend(); }
        const_iterator          cbegin()    const   { return vec_.begin(); }
        const_iterator          cend()      const   { return vec_.end(); }
        const_reverse_iterator  crbegin()   const   { return vec_.rbegin(); }
        const_reverse_iterator  crend()     const   { return vec_.rend(); }

        bool empty() const { return vec_.empty(); }
        size_type size() const { return vec_.size(); }
        size_type max_size() const { return vec_.max_size(); }

        reference operator[](const key_type& key) {
            auto iter = find(key);
            if(iter == end()) {
                //Not found
                iter = insert(std::make_pair(key,mapped_type())).first;
            }

            return *iter;
        }

        reference at(const key_type& key) {
            return const_cast<reference>(const_cast<const flat_map*>(this)->at(key));
        }

        const_reference at(const key_type& key) const {
            auto iter = find(key);
            if(iter == end()) {
                throw std::out_of_range("Invalid key");
            }
            return *iter;
        }

        //Insert value
        std::pair<iterator,bool> insert(const value_type& value) {
            auto iter = lower_bound(value.first);
            if(iter != end() && iter->first == value.first) {
                //Found existing
                return std::make_pair(iter, true);
            } else {
                //Insert
                iter = insert(iter, value);

                return std::make_pair(iter, false);
            }
        }

        //Insert value with position hint
        iterator insert(const_iterator position, const value_type& value) {
            //In a legal position
            VTR_ASSERT(key_compare((position - 1)->first, value.first));
            VTR_ASSERT(!key_compare((position + 1)->first, value.first));

            vec_.insert(position, value);
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
            if(iter != end()) {
                vec_.erase(iter);
            }
        }

        //Erase at iterator
        void erase(const_iterator position) {
            vec_.erase(position); 
        }

        //Erase range
        void erase(const_iterator first, const_iterator last) {
            vec_.erase(first,last);
        }

        void swap(flat_map& other) { std::swap(*this, other); }

        void clear() { vec_.clear(); }

        template<class ...Args>
        iterator emplace(const key_type& key, Args&&... args) {
            auto iter = lower_bound(key);
            if(iter != end() && iter->first == key) {
                //Found
                return std::make_pair(iter, false);
            } else {
                //Emplace
                iter = emplace_hint(iter, key, std::forward<Args>(args)...);
                return std::make_pair(iter, true);
            }
        }

        template<class ...Args>
        iterator emplace_hint(const_iterator position, Args&&... args) {
            return vec_.emplace(position, std::forward<Args>(args)...);
        }

        void reserve(size_type n) { vec_.reserve(n); }
        void shrink_to_fit() { vec_.shrink_to_fit(); }

        key_compare key_comp() const { return key_compare(); }
        value_compare value_comp() const { return value_compare(key_comp()); }

        iterator find(const key_type& key) {
            const_iterator const_iter = const_cast<const flat_map*>(this)->find(key);
            return iterator(const_iter);
        }

        const_iterator find(const key_type& key) const {
            auto iter = lower_bound(key); 
            if(iter != end() && iter->first == key) {
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
            return iterator(const_iter);
        }

        const_iterator lower_bound(const key_type& key) const {
            return std::lower_bound(begin(), end(), key, key_compare()); 
        }

        iterator upper_bound(const key_type& key) {
            const_iterator const_iter = const_cast<const flat_map*>(this)->upper_bound(key); 
            return iterator(const_iter); 
        }

        const_iterator upper_bound(const key_type& key) const {
            return std::upper_bound(begin(), end(), key, key_compare());
        }

        std::pair<iterator,iterator> equal_range(const key_type& key) {
            auto const_iter_pair = const_cast<const flat_map*>(this)->equal_range(key);
            return std::pair<iterator,iterator>(iterator(const_iter_pair.first), iterator(const_iter_pair.second));
        }

        std::pair<const_iterator,const_iterator> equal_range(const key_type& key) const {
            return std::equal_range(begin(), end(), key);
        }

    public:
        friend void swap(flat_map& lhs, flat_map& rhs) { std::swap(lhs.vec_, rhs.vec_); }

    private:
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

    private:
        std::vector<value_type> vec_;
};

template<class K, class T, class Compare>
class flat_map<K,T,Compare>::value_compare {
    friend class flat_map;
    public:
        typedef bool result_type;
        typedef value_type first_argument_type;
        typedef value_type second_argument_type;
        bool operator() (const value_type& x, const value_type& y) const {
            return comp(x.first, y.first);
        }
    private:
        value_compare(Compare c) : comp(c) {}

        Compare comp;
};

} //namespace
#endif
