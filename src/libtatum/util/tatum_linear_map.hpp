#ifndef TATUM_LINEAR_MAP
#define TATUM_LINEAR_MAP
#include <vector>

#include "tatum_assert.hpp"

namespace tatum { namespace util {

//A vector-like container which is indexed by K (instead of size_t as in std::vector).
//Requires that K be convertable to size_t with the size_t operator (i.e. size_t()), and
//that the conversion results in a linearly increasing index into the underlying vector.
//
//This results in a container that is similar to a std::map (i.e. converts from one type to
//another), but requires contigously ascending (i.e. linear) keys. Unlike std::map only the
//values are stored (at the specified index/key), reducing memory usage and improving cache
//locality. 
//
//As with a std::vector, it is the caller's responsibility to ensure there is sufficient space 
//for a given index/key before it is accessed.
template<typename K, typename V>
class linear_map {
    public: //Public types
        typedef typename std::vector<V>::const_reference const_reference;
        typedef typename std::vector<V>::reference reference;

        typedef typename std::vector<V>::const_iterator const_iterator;
        typedef typename std::vector<V>::const_reverse_iterator const_reverse_iterator;

    public: //Mutators
        
        template<typename... Args>
        linear_map(Args&&... args)
            : vec_(std::forward<Args>(args)...)
        { }

        //Delegate potentially overloaded functions to the underlying vector with perfect
        //forwarding
        template<typename... Args>
        void push_back(Args&&... args) { vec_.push_back(std::forward<Args>(args)...); }

        template<typename... Args>
        void emplace_back(Args&&... args) { vec_.emplace_back(std::forward<Args>(args)...); }

        template<typename... Args>
        void resize(Args&&... args) { vec_.resize(std::forward<Args>(args)...); }

        void clear() { vec_.clear(); }

        void shrink_to_fit() { vec_.shrink_to_fit(); }

        //Indexing
        reference operator[] (const K n) { 
            TATUM_ASSERT_SAFE_MSG(size_t(n) < vec_.size(), "Out-of-range index");
            return vec_[size_t(n)]; 
        }

        //Swap (this enables std::swap via ADL)
        friend void swap(linear_map<K,V>& x, linear_map<K,V>& y) {
            std::swap(x.vec_, y.vec_);
        }
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

        std::size_t size() const { return vec_.size(); }

        bool empty() const { return vec_.empty(); }

    private:
        std::vector<V> vec_;
};


}} //namespace
#endif
