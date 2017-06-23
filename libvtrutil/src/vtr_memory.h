#ifndef VTR_MEMORY_H
#define VTR_MEMORY_H
#include <cstddef>

namespace vtr {
    struct t_linked_vptr; //Forward declaration

    /* This structure is to keep track of chunks of memory that is being	*
     * allocated to save overhead when allocating very small memory pieces. *
     * For a complete description, please see the comment in my_chunk_malloc*/
    struct t_chunk {
        t_linked_vptr *chunk_ptr_head; 
        /* chunk_ptr_head->data_vptr: head of the entire linked 
         * list of allocated "chunk" memory;
         * chunk_ptr_head->next: pointer to the next chunk on the linked list*/
        int mem_avail; /* number of bytes left in the current chunk */
        char *next_mem_loc_ptr;/* pointer to the first available (free) *
                    * byte in the current chunk		*/
    };

    void free(void *some);
    void* calloc(size_t nelem, size_t size);
    void* malloc(size_t size);
    void* realloc(void *ptr, size_t size);

    void* chunk_malloc(size_t size, t_chunk *chunk_info);
    void free_chunk_memory(t_chunk *chunk_info);

    //Cross platform wrapper around GNU's malloc_trim()
    // TODO: This is only used in one place within VPR, consider removing it
    int malloc_trim(size_t pad);
}

#endif
