/*
 * Copyright 2023 CAS—Atlantic (University of New Brunswick, CASA)
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef ODIN_MEMORY_H
#define ODIN_MEMORY_H

#include <cstdint>
#include <cstddef>
#include <limits>
#include <cstring>
#include <algorithm>

#include "odin_error.h"

#ifndef __GLIBC__
#include <stdlib.h>
#else
#include <malloc.h>
#endif

namespace odin {
extern uintptr_t min_address;
extern uintptr_t max_address;

char* strdup(const char* in);

template<typename T>
void free(T** ptr_ref) {
    oassert(ptr_ref != NULL && "did not pass in a valid ref");

    T* ptr = (*ptr_ref);
    (*ptr_ref) = NULL;

    if (ptr != NULL && (uintptr_t)ptr >= min_address && (uintptr_t)ptr <= max_address) { std::free(ptr); }
}

template<typename T, typename S>
void* calloc(T _n_element, S _element_size) {
    void* ret = NULL;
    size_t n_element = 0;
    size_t element_size = 0;
    size_t n_bytes = 0;

    if (std::is_unsigned<T>() || _n_element > 0) { n_element = static_cast<size_t>(_n_element); }

    if (std::is_unsigned<S>() || _element_size > 0) { element_size = static_cast<size_t>(_element_size); }

    n_bytes = n_element * element_size;
    if (n_bytes > 0) {
        ret = std::calloc(n_bytes, 1);

        oassert(ret != NULL && "odin::calloc failed, OOM?");
    }

    if (ret != NULL) {
        min_address = std::min(min_address, (uintptr_t)ret);
        max_address = std::max(max_address, (uintptr_t)ret);
    }

    return (ret);
}

// we use a template to check for valid size
template<typename T>
void* malloc(T _n_bytes) {
    return calloc(_n_bytes, 1);
}

template<typename T, typename S>
void realloc(T** ptr_ref, S _n_bytes) {
    oassert(ptr_ref != NULL && "did not pass in a valid ref");

    T* ptr = (*ptr_ref);
    (*ptr_ref) = NULL;

    size_t n_bytes = 0;
    if (std::is_unsigned<T>() || _n_bytes > 0) { n_bytes = static_cast<size_t>(_n_bytes); }

    if (ptr != NULL && (uintptr_t)ptr >= min_address && (uintptr_t)ptr <= max_address) {
        if (n_bytes > 0) {
            ptr = (T*)std::realloc(ptr, n_bytes);
            oassert(ptr != NULL && "odin::realloc failed, OOM?");
        } else {
            free(&ptr);
        }
    } else {
        if (n_bytes > 0) {
            ptr = (T*)calloc(n_bytes, 1);
        } else {
            ptr = NULL;
        }
    }

    if (ptr != NULL) {
        min_address = std::min(min_address, (uintptr_t)ptr);
        max_address = std::max(max_address, (uintptr_t)ptr);
    }

    (*ptr_ref) = ptr;
}
} // namespace odin

#endif //ODIN_MEMORY_H