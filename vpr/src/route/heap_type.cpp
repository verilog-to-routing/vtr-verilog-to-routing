#include "heap_type.h"

#include "binary_heap.h"
#include "bucket.h"
#include "rr_graph_fwd.h"
#include "vpr_error.h"
#include "vpr_types.h"

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
    heap_free_head_ = heap_free_head_->next_heap_item();

    num_heap_allocated_++;

    //Reset
    temp_ptr->set_next_heap_item(nullptr);
    temp_ptr->cost = 0.;
    temp_ptr->backward_path_cost = 0.;
    temp_ptr->backward_path_delay = 0.;
    temp_ptr->backward_path_congestion = 0.;
    temp_ptr->R_upstream = 0.;
    temp_ptr->index = RRNodeId::INVALID();
    temp_ptr->path_data = nullptr;
    temp_ptr->set_prev_edge(RREdgeId::INVALID());
    return (temp_ptr);
}

void HeapStorage::free(t_heap* hptr) {
    hptr->set_next_heap_item(heap_free_head_);
    heap_free_head_ = hptr;
    num_heap_allocated_--;
}

void HeapStorage::free_all_memory() {
    VTR_ASSERT(num_heap_allocated_ == 0);

    if (heap_free_head_ != nullptr) {
        t_heap* curr = heap_free_head_;
        while (curr) {
            t_heap* tmp = curr;
            curr = curr->next_heap_item();

            vtr::chunk_delete(tmp, &heap_ch_);
        }

        heap_free_head_ = nullptr;
    }

    /*free the memory chunks that were used by heap and linked f pointer */
    free_chunk_memory(&heap_ch_);
}

std::unique_ptr<HeapInterface> make_heap(e_heap_type heap_type) {
    switch (heap_type) {
        case e_heap_type::BINARY_HEAP:
            return std::make_unique<BinaryHeap>();
        case e_heap_type::BUCKET_HEAP_APPROXIMATION:
            return std::make_unique<Bucket>();
        default:
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Unknown heap_type %d", heap_type);
    }
}
