#ifndef VTR_BIMAP
#define VTR_BIMAP

/**
 * @file
 * @brief The vtr_bimap.h header provides a bi-directonal mapping between key and value which means that it can be addressed by either the key or the value
 *
 * It provides this bi-directional feature for all the map-like containers defined in vtr:
 *      - unordered map
 *      - flat map
 *      - linear map
 * 
 * One example where this container might be so useful is the mapping between the atom and clustered net Id. See atom_lookup.h
 */

#include <map>
#include <unordered_map>
#include "vtr_flat_map.h"
#include "vtr_linear_map.h"

#include "vtr_error.h"

namespace vtr {

/**
 * @brief A map-like class which provides a bi-directonal mapping between key and value
 *
 * Keys and values can be looked up directly by passing either the key or value.
 * the indexing operator will throw if the key/value does not exist.
 */
template<class K, class V, template<typename...> class Map = std::map, template<typename...> class InvMap = std::map>
class bimap {
  public: //Public types
    typedef typename Map<K, V>::const_iterator iterator;
    typedef typename InvMap<V, K>::const_iterator inverse_iterator;

  public: //Accessors
    //Iterators

    ///@brief Return an iterator to the begin of the map
    iterator begin() const { return map_.begin(); }

    ///@brief Return an iterator to the end of the map
    iterator end() const { return map_.end(); }

    ///@brief Return an iterator to the begin of the inverse map
    inverse_iterator inverse_begin() const { return inverse_map_.begin(); }

    ///@brief Return an iterator to the end of the inverse map
    inverse_iterator inverse_end() const { return inverse_map_.end(); }

    ///@brief Return an iterator to the key-value pair matching key, or end() if not found
    iterator find(const K key) const {
        return map_.find(key);
    }

    ///@brief Return an iterator to the value-key pair matching value, or inverse_end() if not found
    inverse_iterator find(const V value) const {
        return inverse_map_.find(value);
    }

    ///@brief Return an immutable reference to the value matching key (throw an exception if key is not found)
    const V& operator[](const K key) const {
        auto iter = find(key);
        if (iter == end()) {
            throw VtrError("Invalid bimap key during look-up", __FILE__, __LINE__);
        }
        return iter->second;
    }

    ///@brief Return an immutable reference to the key matching value (throw an exception if value is not found)
    const K& operator[](const V value) const {
        auto iter = find(value);
        if (iter == inverse_end()) {
            throw VtrError("Invalid bimap value during inverse look-up", __FILE__, __LINE__);
        }
        return iter->second;
    }

    ///@brief Return the number of key-value pairs stored
    std::size_t size() const {
        VTR_ASSERT(map_.size() == inverse_map_.size());
        return map_.size();
    }

    ///@brief Return true if there are no key-value pairs stored
    bool empty() const { return (size() == 0); }

    ///@brief Return true if the specified key exists
    bool contains(const K key) const { return find(key) != end(); }

    ///@brief Return true if the specified value exists
    bool contains(const V value) const { return find(value) != inverse_end(); }

  public: //Mutators
    ///@brief Drop all stored key-values
    void clear() {
        map_.clear();
        inverse_map_.clear();
    }

    ///@brief Insert a key-value pair, if not already in map
    std::pair<iterator, bool> insert(const K key, const V value) {
        auto ret1 = map_.insert({key, value});
        auto ret2 = inverse_map_.insert({value, key});

        VTR_ASSERT(ret1.second == ret2.second);

        // Return true if inserted
        return ret1;
    }

    ///@brief Update a key-value pair, will insert if not already in map
    void update(const K key, const V value) {
        map_[key] = value;
        inverse_map_[value] = key;
    }

    ///@brief Remove the specified key (and it's associated value)
    void erase(const K key) {
        auto iter = map_.find(key);
        if (iter != map_.end()) {
            V val = iter->second;
            map_.erase(iter);

            auto inv_iter = inverse_map_.find(val);
            VTR_ASSERT(inv_iter != inverse_map_.end());
            inverse_map_.erase(inv_iter);
        }
    }

    ///@brief Remove the specified value (and it's associated key)
    void erase(const V val) {
        auto inv_iter = inverse_map_.find(val);
        if (inv_iter != inverse_map_.end()) {
            K key = inv_iter->second;
            inverse_map_.erase(inv_iter);

            auto iter = map_.find(key);
            VTR_ASSERT(iter != map_.end());
            map_.erase(iter);
        }
    }

    ///@brief Swap (this enables std::swap via ADL)
    friend void swap(bimap<K, V, Map, InvMap>& x, bimap<K, V, Map, InvMap>& y) {
        std::swap(x.map_, y.map_);
        std::swap(x.inverse_map_, y.inverse_map_);
    }

  private:
    Map<K, V> map_;
    InvMap<V, K> inverse_map_;
};

template<class K, class V>
using unordered_bimap = bimap<K, V, std::unordered_map, std::unordered_map>;

template<class K, class V>
using flat_bimap = bimap<K, V, vtr::flat_map, vtr::flat_map>;

template<class K, class V>
using linear_bimap = bimap<K, V, vtr::linear_map, vtr::linear_map>;

} // namespace vtr

#endif
