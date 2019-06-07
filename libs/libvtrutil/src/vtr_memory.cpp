#include <cstddef>
#include <cstdlib>

#include "vtr_assert.h"
#include "vtr_list.h"
#include "vtr_memory.h"
#include "vtr_error.h"
#include "vtr_util.h"

#ifndef __GLIBC__
#    include <stdlib.h>
#else
#    include <malloc.h>
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

    if ((ret = std::calloc(nelem, size)) == nullptr) {
        throw VtrError("Unable to calloc memory.", __FILE__, __LINE__);
    }
    return (ret);
}

void* malloc(size_t size) {
    void* ret;
    if (size == 0) {
        return nullptr;
    }

    if ((ret = std::malloc(size)) == nullptr && size != 0) {
        throw VtrError("Unable to malloc memory.", __FILE__, __LINE__);
    }
    return (ret);
}

void* realloc(void* ptr, size_t size) {
    void* ret;

    ret = std::realloc(ptr, size);
    if (nullptr == ret && size != 0) {
        throw VtrError(string_fmt("Unable to realloc memory (ptr=%p, size=%d).", ptr, size),
                       __FILE__, __LINE__);
    }
    return (ret);
}

void* chunk_malloc(size_t size, t_chunk* chunk_info) {
    /* This routine should be used for allocating fairly small data             *
     * structures where memory-efficiency is crucial.  This routine allocates   *
     * large "chunks" of data, and parcels them out as requested.  Whenever     *
     * it mallocs a new chunk it adds it to the linked list pointed to by       *
     * chunk_info->chunk_ptr_head.  This list can be used to free the	    *
     * chunked memory.							    *
     * Information about the currently open "chunk" must be stored by the       *
     * user program.  chunk_info->mem_avail_ptr points to an int storing	    *
     * how many bytes are left in the current chunk, while			    *
     * chunk_info->next_mem_loc_ptr is the address of a pointer to the	    *
     * next free bytes in the chunk.  To start a new chunk, simply set	    *
     * chunk_info->mem_avail_ptr = 0.  Each independent set of data		    *
     * structures should use a new chunk.                                       */

    /* To make sure the memory passed back is properly aligned, I must *
     * only send back chunks in multiples of the worst-case alignment  *
     * restriction of the machine.  On most machines this should be    *
     * a long, but on 64-bit machines it might be a long long or a     *
     * double.  Change the typedef below if this is the case.          */

    typedef size_t Align;

    constexpr int CHUNK_SIZE = 32768;
    constexpr int FRAGMENT_THRESHOLD = 100;

    char* tmp_ptr;
    int aligned_size;

    VTR_ASSERT(chunk_info->mem_avail >= 0);

    if ((size_t)(chunk_info->mem_avail) < size) { /* Need to malloc more memory. */
        if (size > CHUNK_SIZE) {                  /* Too big, use standard routine. */
            tmp_ptr = (char*)vtr::malloc(size);

            /* When debugging, uncomment the code below to see if memory allocation size */
            /* makes sense */
            //#ifdef DEBUG
            // vtr_printf("NB: my_chunk_malloc got a request for %d bytes.\n", size);
            // vtr_printf("You should consider using vtr::malloc for such big requests.\n");
            // #endif

            VTR_ASSERT(chunk_info != nullptr);
            chunk_info->chunk_ptr_head = insert_in_vptr_list(chunk_info->chunk_ptr_head, tmp_ptr);
            return (tmp_ptr);
        }

        if (chunk_info->mem_avail < FRAGMENT_THRESHOLD) { /* Only a small scrap left. */
            chunk_info->next_mem_loc_ptr = (char*)vtr::malloc(CHUNK_SIZE);
            chunk_info->mem_avail = CHUNK_SIZE;
            VTR_ASSERT(chunk_info != nullptr);
            chunk_info->chunk_ptr_head = insert_in_vptr_list(chunk_info->chunk_ptr_head, chunk_info->next_mem_loc_ptr);
        }

        /* Execute else clause only when the chunk we want is pretty big,  *
         * and would leave too big an unused fragment.  Then we use malloc *
         * to allocate normally.                                           */

        else {
            tmp_ptr = (char*)vtr::malloc(size);
            VTR_ASSERT(chunk_info != nullptr);
            chunk_info->chunk_ptr_head = insert_in_vptr_list(chunk_info->chunk_ptr_head, tmp_ptr);
            return (tmp_ptr);
        }
    }

    /* Find the smallest distance to advance the memory pointer and keep *
     * everything aligned.                                               */

    if (size % sizeof(Align) == 0) {
        aligned_size = size;
    } else {
        aligned_size = size + sizeof(Align) - size % sizeof(Align);
    }

    tmp_ptr = chunk_info->next_mem_loc_ptr;
    chunk_info->next_mem_loc_ptr += aligned_size;
    chunk_info->mem_avail -= aligned_size;
    return (tmp_ptr);
}

void free_chunk_memory(t_chunk* chunk_info) {
    /* Frees the memory allocated by a sequence of calls to my_chunk_malloc. */

    t_linked_vptr *curr_ptr, *prev_ptr;

    curr_ptr = chunk_info->chunk_ptr_head;

    while (curr_ptr != nullptr) {
        free(curr_ptr->data_vptr); /* Free memory "chunk". */
        prev_ptr = curr_ptr;
        curr_ptr = curr_ptr->next;
        free(prev_ptr); /* Free memory used to track "chunk". */
    }
    chunk_info->chunk_ptr_head = nullptr;
    chunk_info->mem_avail = 0;
    chunk_info->next_mem_loc_ptr = nullptr;
}

} // namespace vtr
