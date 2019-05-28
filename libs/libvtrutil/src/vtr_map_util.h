#ifndef VTR_MAP_UTIL_H
#define VTR_MAP_UTIL_H

#include "vtr_pair_util.h"
#include "vtr_range.h"

namespace vtr {

//An iterator who wraps a std::map iterator to return it's key
template<typename Iter>
using map_key_iter = pair_first_iter<Iter>;

//An iterator who wraps a std::map iterator to return it's value
template<typename Iter>
using map_value_iter = pair_second_iter<Iter>;

//Returns a range iterating over a std::map's keys
template<typename T>
auto make_key_range(T b, T e) {
    using key_iter = map_key_iter<T>;
    return vtr::make_range(key_iter(b), key_iter(e));
}

//Returns a range iterating over a std::map's keys
template<typename Container>
auto make_key_range(const Container& c) {
    return make_key_range(std::begin(c), std::end(c));
}

//Returns a range iterating over a std::map's values
template<typename T>
auto make_value_range(T b, T e) {
    using value_iter = map_value_iter<T>;
    return vtr::make_range(value_iter(b), value_iter(e));
}

//Returns a range iterating over a std::map's values
template<typename Container>
auto make_value_range(const Container& c) {
    return make_value_range(std::begin(c), std::end(c));
}

} // namespace vtr

#endif
