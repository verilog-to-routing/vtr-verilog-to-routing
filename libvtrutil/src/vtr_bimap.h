#ifndef VTR_BIMAP
#define VTR_BIMAP
#include <unordered_map>
#include "vtr_error.h"

namespace vtr {

/*
 * A map-like class which provides a bi-directonal mapping between key and value
 *
 * Keys and values can be looked up directly by passing either the key or value.
 * the indexing operator will throw if the key/value does not exist.
 */
template<class K, class V>
class bimap {
    public: //Public types
        typedef typename std::unordered_map<K,V>::const_iterator iterator;
        typedef typename std::unordered_map<V,K>::const_iterator inverse_iterator;
    public: //Accessors

        //Iterators
        iterator begin() const { return map_.begin(); }
        iterator end() const { return map_.end(); }
        inverse_iterator inverse_begin() const { return inverse_map_.begin(); }
        inverse_iterator inverse_end() const { return inverse_map_.end(); }

        iterator find(const K key) const {
            return map_.find(key);
        }
        inverse_iterator find(const V val) const {
            return inverse_map_.find(val);
        }

        const V& operator[] (const K key) const { 
            auto iter = find(key);
            if(iter == end()) {
                throw VtrError("Invalid bimap key during look-up", __FILE__, __LINE__);
            }
            return iter->second;
        }
        const K& operator[] (const V val) const {
            auto iter = find(val);
            if(iter == inverse_end()) {
                throw VtrError("Invalid bimap value during inverse look-up", __FILE__, __LINE__);
            }
            return iter->second;
        }

        std::size_t size() const { VTR_ASSERT(map_.size() == inverse_map_.size()); return map_.size(); }

        bool empty() const { return (size() == 0); }

        bool contains(const K key) const { return find(key) != end(); }
        bool contains(const V val) const { return find(val) != inverse_end(); }

    public: //Mutators
        
        void clear() { map_.clear(); inverse_map_.clear(); }

        void insert(const K key, const V value) {
            map_.insert({key,value});
            inverse_map_.insert({value,key});
        }

        void erase(const K key) {
            auto iter = map_.find(key);
            if(iter != map_.end()) {
                V val = iter->second;
                map_.erase(iter);

                auto inv_iter = inverse_map_.find(val);
                VTR_ASSERT(inv_iter != inverse_map_.end());
                inverse_map_.erase(inv_iter);
            }
        }

        void erase(const V val) {
            auto inv_iter = inverse_map_.find(val);
            if(inv_iter != inverse_map_.end()) {
                K key = inv_iter->second;
                inverse_map_.erase(inv_iter);

                auto iter = map_.find(val);
                VTR_ASSERT(iter != map_.end());
                map_.erase(iter);
            }
        }

        //Swap (this enables std::swap via ADL)
        friend void swap(bimap<K,V>& x, bimap<K,V>& y) {
            std::swap(x.map_, y.map_);
            std::swap(x.inverse_map_, y.inverse_map_);
        }
    private:
        std::unordered_map<K,V> map_;
        std::unordered_map<V,K> inverse_map_;
};

}

#endif
