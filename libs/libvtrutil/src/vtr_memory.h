#pragma once

#include <cstddef>
#include <cstdlib>
#include <new>

#ifdef _WIN32
#include <cerrno>
#include <malloc.h>
#endif

#ifdef _MSC_VER // MSVC
// MSVC does not support GCC __builtin_prefetch.
// Without this guard, GCC-specific constructs can cause syntax errors
// (e.g., in multi_queue_d_ary_heap.tpp). Since prefetch is only a
// performance hint, it is safe to make it a no-op and ignore attributes.
#define __builtin_prefetch(...) ((void)0)
#endif

namespace vtr {

/**
 * @brief This function will force the container to be cleared
 *
 * It releases its held memory.
 * For efficiency, STL containers usually don't
 * release their actual heap-allocated memory until
 * destruction (even if Container::clear() is called).
 */
template<typename Container>
void release_memory(Container& container) {
    ///@brief Force a re-allocation to happen by swapping in a new (empty) container.
    Container().swap(container);
}

void* free(void* some);
void* calloc(size_t nelem, size_t size);
void* malloc(size_t size);
void* realloc(void* ptr, size_t size);

/**
 * @brief Cross platform wrapper around GNU's malloc_trim()
 *
 * TODO: This is only used in one place within VPR, consider removing it
 */
int malloc_trim(size_t pad);

/**
 * @brief A macro generates a prefetch instruction on all architectures that include it.
 * 
 * This is all modern x86 and ARM64 platforms.
 *
 * This is a macro because it has to be.  rw and locality must be constants,
 * not just constexpr.
 */
#define VTR_PREFETCH(addr, rw, locality) __builtin_prefetch(addr, rw, locality)

} // namespace vtr
