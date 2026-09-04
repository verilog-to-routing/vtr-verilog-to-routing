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
#include <cstdint>
#include <iterator>
#include <limits>
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
 * Runs in O(num_elements + num_keys) time and allocates O(num_keys) temporary
 * storage for the key counts. This beats a comparison sort, which takes
 * O(num_elements log num_elements), as long as num_keys is not much larger
 * than the number of elements. A very sparse key space (num_keys far larger
 * than the element count) wastes both time and memory on empty counts, so
 * such inputs are a poor match for this function.
 *
 * The input may hold at most 2^32 - 1 elements. Output positions are stored
 * as uint32_t to halve the memory traffic of the counts array, which is
 * accessed randomly in the placement pass.
 *
 * @tparam InIt   Forward iterator over the input elements.
 * @tparam OutIt  Random access iterator to the output range.
 * @tparam KeyFn  Callable mapping an element to its key. The key may be any
 *                type explicitly convertible to size_t (integers, vtr::StrongId).
 * @param num_keys  Exclusive upper bound on the keys. Every key must be smaller than this.
 */
template<typename InIt, typename OutIt, typename KeyFn>
    requires std::forward_iterator<InIt> && std::random_access_iterator<OutIt>
void stable_counting_sort(InIt first, InIt last, OutIt out, size_t num_keys, KeyFn key_of) {
    // The sort runs in three passes:
    //   1. Count how many elements have each key.
    //   2. Prefix sum the counts so that each key maps to the output position
    //      of its first element.
    //   3. Walk the input in order and write each element to the next free
    //      slot for its key. Walking in input order is what makes the sort stable.
    //
    // offsets[k + 1] first holds the number of elements with key k, then after
    // the prefix sum offsets[k] is the output position of the first element with key k.
    std::vector<uint32_t> offsets(num_keys + 1, 0);

    // Pass 1: count the elements with each key
    size_t num_elements = 0;
    for (InIt it = first; it != last; ++it) {
        size_t key = static_cast<size_t>(key_of(*it));
        VTR_ASSERT_MSG(key < num_keys, "Sort key must be smaller than num_keys");
        offsets[key + 1]++;
        num_elements++;
    }
    VTR_ASSERT_MSG(num_elements <= std::numeric_limits<uint32_t>::max(),
                   "Number of sorted elements must fit in the 32 bit offsets");

    // Pass 2: prefix sum the counts into output positions
    for (size_t key = 1; key <= num_keys; ++key) {
        offsets[key] += offsets[key - 1];
    }

    // Pass 3: place each element at the next free slot for its key, then advance that slot
    for (InIt it = first; it != last; ++it) {
        size_t key = static_cast<size_t>(key_of(*it));
        out[offsets[key]] = *it;
        offsets[key] += 1;
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

// Deduction guide so that vtr::sort_key(num_keys, lambda) deduces KeyFn from the
// lambda's type. Without it the caller would have to name the lambda type, which
// is not possible, or wrap the lambda in a std::function.
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
 * @brief Stable LSD radix sort of [first, last) into out by several keys.
 *
 * out must already have (last - first) elements. keys are listed from the
 * most significant to the least significant. The result is the same as a
 * std::stable_sort by the tuple of keys, but it is computed as one stable
 * counting sort pass per key, least significant key first. The input is read
 * directly, so a generated sequence such as a vtr::StrongIdRange needs no copy first.
 * As with stable_counting_sort, the input may hold at most 2^32 - 1 elements.
 *
 * Example, sorting edges by (source node, destination node):
 *
 *     vtr::stable_radix_sort(edges.begin(), edges.end(), sorted_edges,
 *                            vtr::sort_key(num_nodes, [&](RREdgeId e) { return src_node[e]; }),
 *                            vtr::sort_key(num_nodes, [&](RREdgeId e) { return dest_node[e]; }));
 */
template<typename InIt, typename Container, typename... KeyFns>
    requires std::forward_iterator<InIt>
void stable_radix_sort(InIt first, InIt last, Container& out, const sort_key<KeyFns>&... keys) {
    static_assert(sizeof...(KeyFns) > 0, "stable_radix_sort needs at least one sort key");
    constexpr size_t num_keys = sizeof...(KeyFns);
    VTR_ASSERT(static_cast<size_t>(std::distance(first, last)) == std::size(out));

    // First pass on the least significant key, from the input range into out
    auto key_tuple = std::tie(keys...);
    const auto& least_significant = std::get<num_keys - 1>(key_tuple);
    stable_counting_sort(first, last, std::begin(out), least_significant.num_keys, least_significant.key_of);

    // Remaining passes ping-pong between out and scratch, ending in out.
    // num_keys is a compile time constant, so `if constexpr` drops this block
    // entirely for a single key: no scratch buffer and no extra passes.
    if constexpr (num_keys > 1) {
        Container scratch(std::size(out));
        detail::stable_radix_sort_passes(out, scratch, key_tuple, std::make_index_sequence<num_keys - 1>{});
    }
}

} // namespace vtr
