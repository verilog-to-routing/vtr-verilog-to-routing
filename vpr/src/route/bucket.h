#ifndef _BUCKET_ITEMS_H_
#define _BUCKET_ITEMS_H_

#include <vector>

#include "netlist_fwd.h"
#include "physical_types.h"
#include "vtr_memory.h"
#include "globals.h"

/* Used by the heap as its fundamental data structure.
 * Each heap element represents a partial route.
 *
 * cost:    The cost used to sort heap.
 *          For the timing-driven router this is the backward_path_cost +
 *          expected cost to the target.
 *          For the breadth-first router it is the node cost to reach this
 *          point.
 *
 * backward_path_cost:  Used only by the timing-driven router.  The "known"
 *                      cost of the path up to and including this node.
 *                      In this case, the .cost member contains not only
 *                      the known backward cost but also an expected cost
 *                      to the target.
 *
 * R_upstream: Used only by the timing-driven router.  Stores the upstream
 *             resistance to ground from this node, including the
 *             resistance of the node itself (device_ctx.rr_nodes[index].R).
 *
 * index: The RR node index associated with the costs/R_upstream values
 *
 * u.prev.node: The previous node used to reach the current 'index' node
 * u.prev.next: The edge from u.prev.node used to reach the current 'index' node
 *
 * u.next:  pointer to the next s_heap structure in the free
 *          linked list.  Not used when on the heap.
 *
 */
struct t_heap {
    float cost = 0.;
    float backward_path_cost = 0.;
    float R_upstream = 0.;

    int index = OPEN;

    struct t_prev {
        int node;
        int edge;
    };

    union {
        t_heap* next;
        t_prev prev;
    } u;

    // Next pointer for Bucket linked list.
    t_heap* next_bucket;
};

// Allocator for t_heap items.
//
// Supports fast clearing of the items, under the assumption that when clear
// is invoked, all outstanding references can be dropped.  This should be true
// between net routing, and avoids the need to rebuild the free list between
// nets.
class BucketItems {
  public:
    // Returns all allocated items to be available for allocation.
    //
    // This operation is only safe if all outstanding references are discarded.
    // This is true when the router is starting on a new net, as all outstanding
    // items should be in the bucket, which is cleared at the start of routing.
    static void clear() {
        heap_free_head_ = nullptr;
        num_heap_allocated_ = 0;
        alloced_items_ = 0;
    }

    // Iterators over all items ever allocated.  This is not the list of alive
    // items, but can be used for fast invalidation if needed.
    static std::vector<t_heap*>::iterator begin() {
        return heap_items_.begin();
    }
    static std::vector<t_heap*>::iterator end() {
        return heap_items_.end();
    }

    // Deallocate all items.  Outstanding references to items will become
    // invalid.
    static void free() {
        // Free each individual heap item.
        for (auto* item : heap_items_) {
            vtr::chunk_delete(item, &heap_ch_);
        }
        heap_items_.clear();

        /*free the memory chunks that were used by heap and linked f pointer */
        free_chunk_memory(&heap_ch_);
    }

    // Allocate an item.  This may cause a dynamic allocation if no previously
    // allocated items are available.
    static t_heap* alloc_item() {
        t_heap* temp_ptr;
        if (alloced_items_ < heap_items_.size()) {
            temp_ptr = heap_items_[alloced_items_++];
        } else {
            if (heap_free_head_ == nullptr) { /* No elements on the free list */
                heap_free_head_ = vtr::chunk_new<t_heap>(&heap_ch_);
                heap_items_.push_back(heap_free_head_);
                alloced_items_ += 1;
            }

            temp_ptr = heap_free_head_;
            heap_free_head_ = heap_free_head_->u.next;
        }

        num_heap_allocated_++;

        return temp_ptr;
    }

    // Return a free'd item to be reallocated.
    static void free_item(t_heap* hptr) {
        hptr->u.next = heap_free_head_;
        heap_free_head_ = hptr;
        num_heap_allocated_--;
    }

    // Number of outstanding allocations.
    static int num_heap_allocated() {
        return num_heap_allocated_;
    }

  private:
    /* Vector of all items ever allocated. Used for full item iteration and
     * for reuse after a `clear` invocation. */
    static std::vector<t_heap*> heap_items_;

    /* Tracks how many items from heap_items_ are in use. */
    static size_t alloced_items_;

    /* Number of outstanding allocated items. */
    static int num_heap_allocated_;

    /* For managing my own list of currently free heap data structures. */
    static t_heap* heap_free_head_;

    /* For keeping track of the sudo malloc memory for the heap*/
    static vtr::t_chunk heap_ch_;
};

inline void free_heap_data(t_heap* hptr) {
    BucketItems::free_item(hptr);
}

inline t_heap*
alloc_heap_data() {
    //Extract the head
    t_heap* temp_ptr = BucketItems::alloc_item();

    //Reset
    temp_ptr->u.next = nullptr;
    temp_ptr->next_bucket = nullptr;
    temp_ptr->cost = 0.;
    temp_ptr->backward_path_cost = 0.;
    temp_ptr->R_upstream = 0.;
    temp_ptr->index = OPEN;
    temp_ptr->u.prev.node = NO_PREVIOUS;
    temp_ptr->u.prev.edge = NO_PREVIOUS;
    return (temp_ptr);
}

// Prority queue approximation using cost buckets and randomization.
//
// The Bucket contains linked lists for costs at kConvFactor intervals.  Given
// that cost is approximately delay, each bucket contains ~1 picosecond (1e12)
// worth items.
//
// Items are pushed into the linked list that matches their cost [0, 1)
// picosecond.  When popping the Bucket, a random item in the cheapest bucket
// with items is returned.  This randomization exists to prevent the router
// from following identical paths when operating with identical costs.
// Consider two parallel paths to a node.
class Bucket {
  public:
    Bucket() {}

    // Allocate initial buckets for items.
    static void init(const DeviceGrid& grid);

    // Deallocate memory for buckets.  This does NOT call
    // BucketItems::free_item on contained items.
    static void free();

    // Empties all buckets of items.
    //
    // This does NOT call BucketItems::free_item on contained items.  The
    // assumption is that when Bucket::clear is called, BucketItems::clear
    // is also called.
    static void clear();

    // Push an item onto a bucket.
    static void push(t_heap* hptr);

    // Pop an item from the cheapest non-empty bucket.
    //
    // Returns nullptr if empty.
    static t_heap* pop();

    // True if all buckets are empty.
    static bool empty() {
        return heap_head_ == std::numeric_limits<size_t>::max();
    }

    // Sanity check state of buckets (e.g. all items within each bucket have
    // a cost that matches their bucket index.
    static void verify();

    // Print items contained in buckets.
    static void print();

  private:
    // Factor used to convert cost from float to int.  Should be scaled to
    // enable sufficent precision in bucketting.
    static constexpr float kDefaultConvFactor = 1e12;

    // Convert cost from float to integer bucket id.
    static int cost_to_int(float cost) {
        return (int)(cost * conv_factor_);
    }

    // Simple fast random function used for randomizing item selection on pop.
    static size_t fast_rand() {
        seed_ = (0x234ab32a1 * seed_) ^ (0x12acbade);
        return seed_;
    }

    static void check_scaling();

    // Expand the number of buckets.
    //
    // Only call if insufficient bucets exist.
    static void expand(size_t required_number_of_buckets);

    static size_t seed_; /* Seed for fast_rand, should be non-zero */

    static t_heap** heap_;     /* Buckets for linked lists*/
    static size_t heap_size_;  /* Number of buckets */
    static size_t heap_head_;  /* First non-empty bucket */
    static size_t heap_tail_;  /* Last non-empty bucket */
    static float conv_factor_; /* Cost bucket scaling factor */

    static float min_cost_; /* Smallest cost seen */
    static float max_cost_; /* Large cost seen */
};

inline bool is_empty_heap() {
    return Bucket::empty();
}

inline void init_heap(const DeviceGrid& grid) {
    Bucket::init(grid);
}

inline void empty_heap() {
    BucketItems::clear();
    Bucket::clear();
}

inline void add_to_heap(t_heap* hptr) {
    Bucket::push(hptr);
}

inline t_heap* get_heap_head() {
    return Bucket::pop();
}

namespace heap_ {

inline void push_back(t_heap* const hptr) {
    add_to_heap(hptr);
}

inline bool is_valid() {
    return true;
}

// extract every element and print it
inline void pop_heap() {
    while (!is_empty_heap())
        VTR_LOG("%e ", get_heap_head()->cost);
    VTR_LOG("\n");
}

inline void build_heap() {
}

inline void verify_extract_top() {
    Bucket::verify();
}

inline void print_heap() {
    Bucket::print();
}

inline void push_back_node(int inode, float total_cost, int prev_node, int prev_edge, float backward_path_cost, float R_upstream) {
    /* Puts an rr_node on the heap with the same condition as node_to_heap,
     * but do not fix heap property yet as that is more efficiently done from
     * bottom up with build_heap    */

    auto& route_ctx = g_vpr_ctx.routing();
    if (total_cost >= route_ctx.rr_node_route_inf[inode].path_cost)
        return;

    t_heap* hptr = alloc_heap_data();
    hptr->index = inode;
    hptr->cost = total_cost;
    hptr->u.prev.node = prev_node;
    hptr->u.prev.edge = prev_edge;
    hptr->backward_path_cost = backward_path_cost;
    hptr->R_upstream = R_upstream;
    push_back(hptr);
}

} // namespace heap_

#endif /* _BUCKET_ITEMS_H_ */
