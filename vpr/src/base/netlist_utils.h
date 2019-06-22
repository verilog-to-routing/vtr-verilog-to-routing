#ifndef NETLIST_UTILS_H
#define NETLIST_UTILS_H

#include "vtr_vector_map.h"
#include <set>

/*
 *
 * Templated utility functions for cleaning and reordering IdMaps
 *
 */

//Returns true if all elements are contiguously ascending values (i.e. equal to their index)
template<typename T>
bool are_contiguous(vtr::vector_map<T, T>& values) {
    size_t i = 0;
    for (T val : values) {
        if (val != T(i)) {
            return false;
        }
        ++i;
    }
    return true;
}

//Returns true if all elements in the vector 'values' evaluate true
template<typename Container>
bool all_valid(const Container& values) {
    for (auto val : values) {
        if (!val) {
            return false;
        }
    }
    return true;
}

//Builds a mapping from old to new ids by skipping values marked invalid
template<typename Id>
vtr::vector_map<Id, Id> compress_ids(const vtr::vector_map<Id, Id>& ids) {
    vtr::vector_map<Id, Id> id_map(ids.size());
    size_t i = 0;
    for (auto id : ids) {
        if (id) {
            //Valid
            id_map.insert(id, Id(i));
            ++i;
        }
    }

    return id_map;
}

//Returns a vector based on 'values', which has had entries dropped & re-ordered according according to 'id_map'.
//Each entry in id_map corresponds to the assoicated element in 'values'.
//The value of the id_map entry is the new ID of the entry in values.
//
//If it is an invalid ID, the element in values is dropped.
//Otherwise the element is moved to the new ID location.
template<typename Id, typename T>
vtr::vector_map<Id, T> clean_and_reorder_values(const vtr::vector_map<Id, T>& values, const vtr::vector_map<Id, Id>& id_map) {
    VTR_ASSERT(values.size() == id_map.size());

    //Allocate space for the values that will not be dropped
    vtr::vector_map<Id, T> result;

    //Move over the valid entries to their new locations
    for (size_t cur_idx = 0; cur_idx < values.size(); ++cur_idx) {
        Id old_id = Id(cur_idx);

        Id new_id = id_map[old_id];
        if (new_id) {
            //There is a valid mapping
            result.insert(new_id, std::move(values[old_id]));
        }
    }

    return result;
}

//Returns the set of new valid Ids defined by 'id_map'
//TODO: merge with clean_and_reorder_values
template<typename Id>
vtr::vector_map<Id, Id> clean_and_reorder_ids(const vtr::vector_map<Id, Id>& id_map) {
    //For IDs, the values are the new id's stored in the map

    //Allocate a new vector to store the values that have been not dropped
    vtr::vector_map<Id, Id> result;

    //Move over the valid entries to their new locations
    for (size_t cur_idx = 0; cur_idx < id_map.size(); ++cur_idx) {
        Id old_id = Id(cur_idx);

        Id new_id = id_map[old_id];
        if (new_id) {
            result.insert(new_id, new_id);
        }
    }

    return result;
}

//Count how many of the Id's referenced in 'range' have a valid
//new mapping in 'id_map'
template<typename R, typename Id>
size_t count_valid_refs(R range, const vtr::vector_map<Id, Id>& id_map) {
    size_t valid_count = 0;

    for (Id old_id : range) {
        if (id_map[old_id]) {
            ++valid_count;
        }
    }

    return valid_count;
}

//Updates the Ids in 'values' based on id_map, even if the original or new mapping is not valid
template<typename Container, typename ValId>
Container update_all_refs(const Container& values, const vtr::vector_map<ValId, ValId>& id_map) {
    Container updated;

    for (ValId orig_val : values) {
        //The original item was valid
        ValId new_val = id_map[orig_val];
        //The original item exists in the new mapping
        updated.emplace_back(new_val);
    }

    return updated;
}

template<typename Container, typename ValId>
Container update_valid_refs(const Container& values,
                            const vtr::vector_map<ValId, ValId>& id_map,
                            const std::set<size_t>& preserved_indices = {}) {
    Container updated;

    size_t idx = 0;
    for (ValId orig_val : values) {
        if (preserved_indices.count(idx)) {
            //Preserve the original value
            if (!orig_val) {
                updated.emplace_back(orig_val);
            } else {
                ValId new_val = id_map[orig_val];
                if (new_val) {
                    //The original item exists in the new mapping
                    updated.emplace_back(new_val);
                }
            }
        } else if (orig_val) {
            //Original item valid

            ValId new_val = id_map[orig_val];
            if (new_val) {
                //The original item exists in the new mapping
                updated.emplace_back(new_val);
            }
        }
        ++idx;
    }
    return updated;
}

#endif
