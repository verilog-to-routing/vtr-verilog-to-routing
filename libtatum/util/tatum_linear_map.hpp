#ifndef TATUM_LINEAR_MAP
#define TATUM_LINEAR_MAP
#include <vector>

#include "tatum_assert.hpp"

namespace tatum { namespace util {

//A vector-like container which is indexed by K (instead of size_t as in std::vector).
//Requires that K be convertible to size_t with the size_t operator (i.e. size_t()), and
//that the conversion results in a linearly increasing index into the underlying vector.
//
//This results in a container that is similar to a std::map (i.e. converts from one type to
//another), but requires contiguously ascending (i.e. linear) keys. Unlike std::map only the
//values are stored (at the specified index/key), reducing memory usage and improving cache
//locality. Furthermore, find() returns an iterator to the value directly, rather than a std::pair,
//and insert() takes both the key and value as separate arguments and has no return value.
//
//Note that it is possible to use linear_map with sparse/non-contiguous keys, but this is typically
//memory inefficient as the underlying vector will allocate space for [0..size_t(max_key)-1],
//where max_key is the largest key that has been inserted.
//
//As with a std::vector, it is the caller's responsibility to ensure there is sufficient space 
//for a given index/key before it is accessed. The exception to this are the find() and insert() 
//methods which handle non-existing keys gracefully.
template<typename K, typename V>
class linear_map {
    public: //Public types
        typedef typename std::vector<V>::const_reference const_reference;
        typedef typename std::vector<V>::reference reference;

        typedef typename std::vector<V>::iterator iterator;
        typedef typename std::vector<V>::const_iterator const_iterator;
        typedef typename std::vector<V>::const_reverse_iterator const_reverse_iterator;

    public: //Constructor

        //Standard constructors
        linear_map() = default;
        linear_map(const linear_map&) = default;
        linear_map(linear_map&&) = default;
        linear_map& operator=(linear_map&) = default;
        linear_map& operator=(linear_map&&) = default;

        //Vector-like constructors
        explicit linear_map(size_t n) : vec_(n) {}
        explicit linear_map(size_t n, V init_val) : vec_(n, init_val) {}
        explicit linear_map(std::vector<V>&& values) : vec_(values) {}

        /*
         *template<typename... Args>
         *linear_map(Args&&... args)
         *    : vec_(std::forward<Args>(args)...)
         *{ }
         */

    public: //Accessors

        //Iterators
        const_iterator begin() const { return vec_.begin(); }
        const_iterator end() const { return vec_.end(); }
        const_reverse_iterator rbegin() const { return vec_.rbegin(); }
        const_reverse_iterator rend() const { return vec_.rend(); }

        //Indexing
        const_reference operator[] (const K n) const { 
            TATUM_ASSERT_SAFE_MSG(size_t(n) < vec_.size(), "Out-of-range index");
            return vec_[size_t(n)]; 
        }

        const_iterator find(const K key) const {
            if(size_t(key) < vec_.size()) {
                return vec_.begin() + size_t(key);
            } else {
                return vec_.end();
            }
        }

        std::size_t size() const { return vec_.size(); }

        bool empty() const { return vec_.empty(); }

        bool contain(const K key) const { return size_t(key) < vec_.size(); }

    public: //Mutators
        
        //Delegate potentially overloaded functions to the underlying vector with perfect
        //forwarding
        template<typename... Args>
        void push_back(Args&&... args) { vec_.push_back(std::forward<Args>(args)...); }

        template<typename... Args>
        void emplace_back(Args&&... args) { vec_.emplace_back(std::forward<Args>(args)...); }

        template<typename... Args>
        void resize(Args&&... args) { vec_.resize(std::forward<Args>(args)...); }

        void clear() { vec_.clear(); }

        size_t capacity() const { return vec_.capacity(); }
        void shrink_to_fit() { vec_.shrink_to_fit(); }

        //Iterators
        iterator begin() { return vec_.begin(); }
        iterator end() { return vec_.end(); }

        //Indexing
        reference operator[] (const K n) { 
            TATUM_ASSERT_SAFE_MSG(size_t(n) < vec_.size(), "Out-of-range index");
            return vec_[size_t(n)]; 
        }

        iterator find(const K key) {
            if(size_t(key) < vec_.size()) {
                return vec_.begin() + size_t(key);
            } else {
                return vec_.end();
            }
        }

        void insert(const K key, const V value) {
            if(size_t(key) >= vec_.size()) {
                //Resize so key is in range
                vec_.resize(size_t(key) + 1);
            }

            //Insert the value
            operator[](key) = value; 
        }

        //Swap (this enables std::swap via ADL)
        friend void swap(linear_map<K,V>& x, linear_map<K,V>& y) {
            std::swap(x.vec_, y.vec_);
        }
    private:
        std::vector<V> vec_;
};


}} //namespace
#endif
