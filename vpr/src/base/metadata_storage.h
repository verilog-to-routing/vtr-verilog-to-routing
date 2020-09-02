#ifndef _METADATA_STORAGE_H_
#define _METADATA_STORAGE_H_

#include "vtr_string_interning.h"

/**
 * MetadataStorage is a two phase data structure.  In the first phase,
 * metadata is added cheaply by simply pushing onto a vector.  This is
 * unsuitable for lookup, but is fast, and because strings are interned, not
 * too expensive memory wise.
 *
 * The second phase occurs after all data has been inserted.  When/if a lookup
 * is needed, the vector is sorted and the final metadata lookup is generated.
 * In the event that the lookup is never needed, this saves the time spent
 * building the lookup, and reduces the number of outstanding memory
 * allocations dramatically.
 */
template<typename LookupKey>
class MetadataStorage {
  public:
    void reserve(size_t s) {
        VTR_ASSERT(map_.empty());
        data_.reserve(s);
    }
    void add_metadata(const LookupKey& lookup_key, vtr::interned_string meta_key, vtr::interned_string meta_value) {
        // Can only add metadata prior to building the map.
        VTR_ASSERT(map_.empty());
        data_.push_back(std::make_tuple(lookup_key, meta_key, meta_value));
    }

    // Use the given mapping function to change the keys
    void remap_keys(std::function<LookupKey(LookupKey)> key_map) {
        if (map_.empty()) {
            for (auto& entry : data_) {
                std::get<0>(entry) = key_map(std::get<0>(entry));
            }
        } else {
            VTR_ASSERT(data_.empty());
            for (auto& dict : map_) {
                for (auto& entry : dict.second) {
                    for (auto& value : entry.second) {
                        data_.push_back(std::make_tuple(key_map(dict.first), entry.first, value.as_string()));
                    }
                }
            }
            map_.clear();
            build_map();
        }
    }

    typename vtr::flat_map<LookupKey, t_metadata_dict>::const_iterator find(const LookupKey& lookup_key) const {
        check_for_map();

        return map_.find(lookup_key);
    }

    void clear() {
        data_.clear();
        map_.clear();
    }

    size_t size() const {
        check_for_map();
        return map_.size();
    }

    typename vtr::flat_map<LookupKey, t_metadata_dict>::const_iterator begin() const {
        check_for_map();
        return map_.begin();
    }

    typename vtr::flat_map<LookupKey, t_metadata_dict>::const_iterator end() const {
        check_for_map();
        return map_.end();
    }

  private:
    // Check to see if the map has been built yet, builds it if it hasn't been
    // built.
    void check_for_map() const {
        if (map_.empty() && !data_.empty()) {
            build_map();
        }
    }

    // Constructs the metadata lookup map from the flat data.
    void build_map() const {
        VTR_ASSERT(!data_.empty());
        VTR_ASSERT(map_.empty());

        std::sort(data_.begin(), data_.end(), [](const std::tuple<LookupKey, vtr::interned_string, vtr::interned_string>& lhs, const std::tuple<LookupKey, vtr::interned_string, vtr::interned_string>& rhs) {
            return std::get<0>(lhs) < std::get<0>(rhs);
        });

        std::vector<typename vtr::flat_map<LookupKey, t_metadata_dict>::value_type> storage;
        storage.push_back(std::make_pair(std::get<0>(data_[0]), t_metadata_dict()));
        for (const auto& value : data_) {
            if (storage.back().first != std::get<0>(value)) {
                storage.push_back(std::make_pair(std::get<0>(value), t_metadata_dict()));
            }

            storage.back().second.add(std::get<1>(value), std::get<2>(value));
        }

        map_.assign_sorted(std::move(storage));

        data_.clear();
        data_.shrink_to_fit();
    }

    mutable std::vector<std::tuple<LookupKey, vtr::interned_string, vtr::interned_string>> data_;
    mutable vtr::flat_map<LookupKey, t_metadata_dict> map_;
};

#endif /* _METADATA_STORAGE_H_ */
