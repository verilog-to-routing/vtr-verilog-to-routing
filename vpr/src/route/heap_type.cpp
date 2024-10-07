#include "heap_type.h"

#include "vpr_error.h"
#include "d_ary_heap.h"

std::unique_ptr<HeapInterface> make_heap(e_heap_type heap_type) {
    switch (heap_type) {
        case e_heap_type::BINARY_HEAP:
            return std::make_unique<BinaryHeap>();
        case e_heap_type::FOUR_ARY_HEAP:
            return std::make_unique<FourAryHeap>();
        default:
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Unknown heap_type %d", heap_type);
    }
}
