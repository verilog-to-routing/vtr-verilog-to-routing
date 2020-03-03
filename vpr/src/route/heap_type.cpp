#include "heap_type.h"

HeapStorage::HeapStorage()
    : heap_free_head_(nullptr)
    , num_heap_allocated_(0) {}

t_heap*
HeapStorage::alloc() {
    if (heap_free_head_ == nullptr) { /* No elements on the free list */
        heap_free_head_ = vtr::chunk_new<t_heap>(&heap_ch_);
    }

    //Extract the head
    t_heap* temp_ptr = heap_free_head_;
    heap_free_head_ = heap_free_head_->u.next;

    num_heap_allocated_++;

    //Reset
    temp_ptr->u.next = nullptr;
    temp_ptr->cost = 0.;
    temp_ptr->backward_path_cost = 0.;
    temp_ptr->R_upstream = 0.;
    temp_ptr->index = OPEN;
    temp_ptr->u.prev.node = NO_PREVIOUS;
    temp_ptr->u.prev.edge = NO_PREVIOUS;
    return (temp_ptr);
}

void HeapStorage::free(t_heap* hptr) {
    hptr->u.next = heap_free_head_;
    heap_free_head_ = hptr;
    num_heap_allocated_--;
}

void HeapStorage::free_all_memory() {
    VTR_ASSERT(num_heap_allocated_ == 0);

    if (heap_free_head_ != nullptr) {
        t_heap* curr = heap_free_head_;
        while (curr) {
            t_heap* tmp = curr;
            curr = curr->u.next;

            vtr::chunk_delete(tmp, &heap_ch_);
        }

        heap_free_head_ = nullptr;
    }

    /*free the memory chunks that were used by heap and linked f pointer */
    free_chunk_memory(&heap_ch_);
}
