#pragma once
/**
 * @file
 * @brief   Loops over an index range that run in parallel when VPR is built
 *          with TBB and serially otherwise.
 */

#include <cstddef>

#ifdef VPR_USE_TBB
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#endif

/**
 * @brief Call the given function on chunks that together cover the indices
 *        [0, count). Each call receives the first index of its chunk and one
 *        past its last index.
 *
 * The chunks run in parallel when VPR is built with TBB, so the function must
 * not write to state shared between indices. Use this form when each chunk
 * needs its own scratch storage.
 */
template<typename F>
void for_each_index_chunk(size_t count, const F& func) {
#ifdef VPR_USE_TBB
    tbb::parallel_for(tbb::blocked_range<size_t>(0, count), [&](const tbb::blocked_range<size_t>& range) {
        func(range.begin(), range.end());
    });
#else
    func(0, count);
#endif
}

/**
 * @brief Call the given function for every index in [0, count).
 *
 * The calls run in parallel when VPR is built with TBB, so the function must
 * not write to state shared between indices.
 */
template<typename F>
void for_each_index(size_t count, const F& func) {
    for_each_index_chunk(count, [&](size_t begin, size_t end) {
        for (size_t i = begin; i < end; i++) {
            func(i);
        }
    });
}
