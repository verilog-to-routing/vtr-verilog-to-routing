#ifndef VTR_MEMORY_H
#define VTR_MEMORY_H
#include <cstddef>
#include <cstdlib>
#include <new>

namespace vtr {
struct t_linked_vptr; //Forward declaration

/* This structure is to keep track of chunks of memory that is being	*
 * allocated to save overhead when allocating very small memory pieces. *
 * For a complete description, please see the comment in chunk_malloc*/
struct t_chunk {
    t_linked_vptr* chunk_ptr_head = nullptr;
    /* chunk_ptr_head->data_vptr: head of the entire linked
     * list of allocated "chunk" memory;
     * chunk_ptr_head->next: pointer to the next chunk on the linked list*/
    int mem_avail = 0;                /* number of bytes left in the current chunk */
    char* next_mem_loc_ptr = nullptr; /* pointer to the first available (free) *
                                       * byte in the current chunk		*/
};

void* free(void* some);
void* calloc(size_t nelem, size_t size);
void* malloc(size_t size);
void* realloc(void* ptr, size_t size);

void* chunk_malloc(size_t size, t_chunk* chunk_info);
void free_chunk_memory(t_chunk* chunk_info);

//Like chunk_malloc, but with proper C++ object initialization
template<typename T>
T* chunk_new(t_chunk* chunk_info) {
    void* block = chunk_malloc(sizeof(T), chunk_info);

    return new (block) T(); //Placement new
}

//Call the destructor of an obj which must have been allocated in the specified chunk
template<typename T>
void chunk_delete(T* obj, t_chunk* /*chunk_info*/) {
    if (obj) {
        obj->~T(); //Manually call destructor
        //Currently we don't mark the unused memory as free
    }
}

//Cross platform wrapper around GNU's malloc_trim()
// TODO: This is only used in one place within VPR, consider removing it
int malloc_trim(size_t pad);

inline int memalign(void** ptr_out, size_t align, size_t size) {
    return posix_memalign(ptr_out, align, size);
}

// This is a macro because it has to be.  rw and locality must be constants,
// not just constexpr.
//
// This generates a prefetch instruction on all architectures that include it.
// This is all modern x86 and ARM64 platforms.
//
// rw = 0, locality = 0 is the least intrusive software prefetch.  Higher
// locality results in more CPU effort, and needs evidence for higher locality.
#define VTR_PREFETCH(addr, rw, locality) __builtin_prefetch(addr, rw, locality)

// aligned_allocator is a STL allocator that allocates memory in an aligned
// fashion (if supported by the platform).
//
// It is worth noting the C++20 std::allocator does aligned allocations, but
// C++20 has poor support.
template<class T>
struct aligned_allocator {
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    pointer allocate(size_type n, const void* /*hint*/ = 0) {
        void* data;
        int ret = vtr::memalign(&data, alignof(T), sizeof(T) * n);
        if (ret != 0) {
            throw std::bad_alloc();
        }
        return static_cast<pointer>(data);
    }

    void deallocate(T* p, size_type /*n*/) {
        vtr::free(p);
    }
};

} // namespace vtr

#endif
