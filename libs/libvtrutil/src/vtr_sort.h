#pragma once
/**
 * @file
 * @brief Stable counting sort and LSD radix sort for sequences keyed by small integers.
 *
 * These run in linear time with sequential memory access, unlike comparison
 * sorts, and are a good fit when the sort keys are dense ids (node ids,
 * switch ids, flags). Every sort here is stable, so elements with equal keys
 * keep their relative order.
 */

#include <cstddef>
#include <iterator>
#include <tuple>
#include <utility>
#include <vector>

#include "vtr_assert.h"

namespace vtr {

/**
 * @brief Stable counting sort of [first, last) into out.
 *
 * Writes exactly (last - first) elements to out, in ascending order of
 * key_of(element). Elements with equal keys keep their relative order.
 * The input and output ranges must not overlap.
 *
 * @tparam InIt   Forward iterator over the input elements.
 * @tparam OutIt  Random access iterator to the output range.
 * @tparam KeyFn  Callable mapping an element to its key. The key may be any
 *                type explicitly convertible to size_t (integers, vtr::StrongId).
 * @param num_keys  Exclusive upper bound on the keys. Every key must be smaller than this.
 */
template<typename InIt, typename OutIt, typename KeyFn>
void stable_counting_sort(InIt first, InIt last, OutIt out, size_t num_keys, KeyFn key_of) {
    // offsets[k + 1] first holds the number of elements with key k, then after
    // the prefix sum offsets[k] is the output position of the first element with key k.
    std::vector<size_t> offsets(num_keys + 1, 0);

    for (InIt it = first; it != last; ++it) {
        size_t key = static_cast<size_t>(key_of(*it));
        VTR_ASSERT_DEBUG_MSG(key < num_keys, "Sort key must be smaller than num_keys");
        offsets[key + 1]++;
    }

    for (size_t key = 1; key <= num_keys; ++key) {
        offsets[key] += offsets[key - 1];
    }

    for (InIt it = first; it != last; ++it) {
        size_t key = static_cast<size_t>(key_of(*it));
        out[offsets[key]++] = *it;
    }
}

/**
 * @brief Stable counting sort of a container in place.
 *
 * scratch must be a different container of the same size as items. It is used
 * as the destination buffer and then swapped with items, so after the call
 * scratch holds the old (unsorted) contents.
 *
 * @tparam Container  Random access, swappable container (std::vector, vtr::vector, ...).
 * @param num_keys  Exclusive upper bound on the keys. Every key must be smaller than this.
 */
template<typename Container, typename KeyFn>
void stable_counting_sort(Container& items, Container& scratch, size_t num_keys, KeyFn key_of) {
    VTR_ASSERT(std::size(items) == std::size(scratch));
    // The sort reads items while writing scratch, so the two must not be the same container.
    VTR_ASSERT(&items != &scratch);
    stable_counting_sort(std::begin(items), std::end(items), std::begin(scratch), num_keys, key_of);
    std::swap(items, scratch);
}

/**
 * @brief One key of a multi-key sort for stable_radix_sort().
 *
 * Construct with vtr::sort_key(num_keys, key_of). num_keys is the exclusive
 * upper bound on the values key_of returns.
 */
template<typename KeyFn>
struct sort_key {
    /// Exclusive upper bound on the keys
    size_t num_keys;
    /// Callable mapping an element to its key
    KeyFn key_of;
};

template<typename KeyFn>
sort_key(size_t, KeyFn) -> sort_key<KeyFn>;

namespace detail {
/// Runs one stable counting sort pass per key, from the last key in the tuple to the first
template<typename Container, typename KeyTuple, size_t... I>
void stable_radix_sort_passes(Container& items, Container& scratch, const KeyTuple& keys, std::index_sequence<I...>) {
    constexpr size_t num_passes = sizeof...(I);
    (stable_counting_sort(items, scratch,
                          std::get<num_passes - 1 - I>(keys).num_keys,
                          std::get<num_passes - 1 - I>(keys).key_of),
     ...);
}
} // namespace detail

/**
 * @brief Stable LSD radix sort of a container in place by several keys.
 *
 * keys are listed from the most significant to the least significant. The
 * result is the same as a std::stable_sort by the tuple of keys, but it is
 * computed as one stable counting sort pass per key, least significant
 * key first.
 *
 * scratch must have the same size as items and is left holding unspecified contents.
 *
 * Example, sorting edges by (source node, destination node):
 *
 *     vtr::stable_radix_sort(edges, scratch,
 *                            vtr::sort_key(num_nodes, [&](RREdgeId e) { return src_node[e]; }),
 *                            vtr::sort_key(num_nodes, [&](RREdgeId e) { return dest_node[e]; }));
 */
template<typename Container, typename... KeyFns>
void stable_radix_sort(Container& items, Container& scratch, const sort_key<KeyFns>&... keys) {
    detail::stable_radix_sort_passes(items, scratch, std::tie(keys...), std::index_sequence_for<KeyFns...>{});
}

/**
 * @brief Stable LSD radix sort of a container in place, allocating the scratch buffer internally.
 */
template<typename Container, typename... KeyFns>
void stable_radix_sort(Container& items, const sort_key<KeyFns>&... keys) {
    Container scratch(items);
    stable_radix_sort(items, scratch, keys...);
}

} // namespace vtr
