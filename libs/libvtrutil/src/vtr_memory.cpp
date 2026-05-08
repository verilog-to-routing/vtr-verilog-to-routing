#include <cstddef>
#include <cstdlib>
#include <math.h>

#include "vtr_assert.h"
#include "vtr_list.h"
#include "vtr_memory.h"
#include "vtr_error.h"
#include "vtr_util.h"

#ifndef __GLIBC__
#include <stdlib.h>
#else
#include <malloc.h>
#endif

namespace vtr {

#ifndef __GLIBC__
int malloc_trim(size_t /*pad*/) {
    return 0;
}
#else
int malloc_trim(size_t pad) {
    return ::malloc_trim(pad);
}
#endif

void* free(void* some) {
    if (some) {
        std::free(some);
        some = nullptr;
    }
    return nullptr;
}

void* calloc(size_t nelem, size_t size) {
    void* ret;
    if (nelem == 0) {
        return nullptr;
    }
    ret = std::calloc(nelem, size);
    if (ret == nullptr) {
        throw VtrError("Unable to calloc memory.", __FILE__, __LINE__);
    }
    return ret;
}

void* malloc(size_t size) {
    if (size == 0) {
        return nullptr;
    }
    void* ret = std::malloc(size);
    if (ret == nullptr && size != 0) {
        throw VtrError("Unable to malloc memory.", __FILE__, __LINE__);
    }
    return ret;
}

void* realloc(void* ptr, size_t size) {

    void* ret = std::realloc(ptr, size);
    if (nullptr == ret && size != 0) {
        throw VtrError(string_fmt("Unable to realloc memory (ptr=%p, size=%d).", ptr, size),
                       __FILE__, __LINE__);
    }
    return ret;
}

} // namespace vtr
