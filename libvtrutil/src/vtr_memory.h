#ifndef VTR_MEMORY_H
#define VTR_MEMORY_H
#include <cstddef>

//Legacy macros
#define my_calloc(nelem, size) vtr::calloc_impl(nelem, size, __FILE__, __LINE__)
#define my_malloc(size) vtr::malloc_impl(size, __FILE__, __LINE__)
#define my_realloc(ptr, size) vtr::realloc_impl(ptr, size, __FILE__, __LINE__)

namespace vtr {

    /* This structure is to keep track of chunks of memory that is being	*
     * allocated to save overhead when allocating very small memory pieces. *
     * For a complete description, please see the comment in my_chunk_malloc*/
    typedef struct s_chunk {
        struct s_linked_vptr *chunk_ptr_head; 
        /* chunk_ptr_head->data_vptr: head of the entire linked 
         * list of allocated "chunk" memory;
         * chunk_ptr_head->next: pointer to the next chunk on the linked list*/
        int mem_avail; /* number of bytes left in the current chunk */
        char *next_mem_loc_ptr;/* pointer to the first available (free) *
                    * byte in the current chunk		*/
    } t_chunk;

    void *calloc_impl(size_t nelem, size_t size, const char* const file, const int line);
    void *malloc_impl(size_t size, const char* const file, const int line);
    void *realloc_impl(void *ptr, size_t size, const char* const file, const int line);

    void *chunk_malloc(size_t size, t_chunk *chunk_info);
    void free_chunk_memory(t_chunk *chunk_info);

}

#endif
