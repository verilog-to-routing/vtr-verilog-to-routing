#ifndef _BUCKET_H
#define _BUCKET_H

#include <vector>

#include "heap_type.h"
#include "vtr_log.h"

struct BucketItem {
    t_heap item;
    BucketItem* next_bucket;
};

// Allocator for t_heap items.
//
// This allocator supports fast clearing by maintaining an explicit object
// pool and a free list.
//
// The object pool maintained in heap_items_.  Whenever a new object is
// created from the chunk allocator heap_ch_ it is added to heap_items_.
//
// When a client of BucketItems requests an objet, BucketItems first checks
// if there are any objects in the object pool that have not been allocated
// to the client (alloced_items_ < heap_items_.size()).  If there are objects
// in the object pool that have not been alloced, these are use first.
//
// Once all objects from the object pool have been released, future allocations
// come from the free list (maintained in heap_free_head_).  When the free list
// is empty, only then is a new item allocated from the chunk allocator.
//
// BucketItems::clear provides a fast way to reset the object pool under the
// assumption that no live references exists.  It does this by mark the free
// list as empty and the object pool as being fully returned to BucketItems.
// This operation is extremely fast compared with putting all elements back
// onto the free list, as it only involves setting 3 values.
//
// This faster clear **requires** that all previous references to t_heap objects
// are dropped prior to calling clear, otherwise a silent use-after-free issue
// may occur. However because BucketItems is used in conjunction with Bucket,
// and the typical use case is for the heap to be fully emptied between
// routing, this optimization is safe.
//
class BucketItems {
  public:
    BucketItems() noexcept;

    // Returns all allocated items to be available for allocation.
    //
    // This operation is only safe if all outstanding references are discarded.
    // This is true when the router is starting on a new net, as all outstanding
    // items should in the bucket will be cleared at the start of routing.
    void clear() {
        heap_free_head_ = nullptr;
        num_heap_allocated_ = 0;
        alloced_items_ = 0;
    }

    // Iterators over all items ever allocated.  This is not the list of alive
    // items, but can be used for fast invalidation if needed.
    std::vector<BucketItem*>::iterator begin() {
        return heap_items_.begin();
    }
    std::vector<BucketItem*>::iterator end() {
        return heap_items_.end();
    }

    // Deallocate all items.  Outstanding references to items will become
    // invalid.
    void free() {
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
    BucketItem* alloc_item() {
        BucketItem* temp_ptr;
        if (alloced_items_ < heap_items_.size()) {
            // Return an unused object from the object pool.
            temp_ptr = heap_items_[alloced_items_++];
        } else {
            if (heap_free_head_ == nullptr) { /* No elements on the free list */
                heap_free_head_ = vtr::chunk_new<BucketItem>(&heap_ch_);
                heap_free_head_->next_bucket = nullptr;
                heap_items_.push_back(heap_free_head_);
                alloced_items_ += 1;
            }

            temp_ptr = heap_free_head_;
            heap_free_head_ = heap_free_head_->next_bucket;
        }

        num_heap_allocated_++;

        return temp_ptr;
    }

    // Return a free'd item to be reallocated.
    void free_item(BucketItem* hptr) {
        hptr->next_bucket = heap_free_head_;
        heap_free_head_ = hptr;
        num_heap_allocated_--;
    }

    // Number of outstanding allocations.
    int num_heap_allocated() {
        return num_heap_allocated_;
    }

  private:
    /* Vector of all items ever allocated. Used for full item iteration and
     * for reuse after a `clear` invocation. */
    std::vector<BucketItem*> heap_items_;

    /* Tracks how many items from heap_items_ are in use. */
    size_t alloced_items_;

    /* Number of outstanding allocated items. */
    int num_heap_allocated_;

    /* For managing my own list of currently free heap data structures. */
    BucketItem* heap_free_head_;

    /* For keeping track of the sudo malloc memory for the heap*/
    vtr::t_chunk heap_ch_;
};

// Prority queue approximation using cost buckets and randomization.
//
// The cost buckets are each a linked lists for costs at kDefaultConvFactor
// intervals. Given that cost is approximately delay, each bucket contains ~1
// picosecond (1e12) worth of items.
//
// Items are pushed into the linked list that matches their cost [0, 1)
// picosecond.  When popping the Bucket, a random item in the cheapest bucket
// with items is returned.  This randomization exists to prevent the router
// from following identical paths when operating with identical costs.
// Consider two parallel paths to a node.
//
// To ensure that number of buckets do not get too large, whenever is element
// is added to the heap, the number of buckets required is checked.  If more
// than 100k buckets are required, then the width of the buckets (conv_factor_)
// are rescaled such that ~50k buckets are required.
//
// Important node: This approximation makes some assumptions about the
// structure of costs.
//
// Assumptions:
//  1. 0 is the minimum cost
//  2. Costs that are different by 0.1 % of the maximum cost are effectively
//     equivilant
//  3. The cost function is roughly linear.
//
class Bucket : public HeapInterface {
  public:
    Bucket() noexcept;
    ~Bucket();

    t_heap* alloc() final {
        outstanding_items_ += 1;
        t_heap* hptr = &items_.alloc_item()->item;
        return hptr;
    }
    void free(t_heap* hptr) final {
        // Static assert ensures that BucketItem::item is at offset 0,
        // so this cast is safe.
        outstanding_items_ -= 1;
        items_.free_item(reinterpret_cast<BucketItem*>(hptr));
    }

    // Allocate initial buckets for items.
    void init_heap(const DeviceGrid& grid) final;

    // Deallocate memory for buckets.
    void free_all_memory() final;

    // Empties all buckets of items.
    //
    // This does NOT call BucketItems::free_item on contained items.  The
    // assumption is that when Bucket::clear is called, BucketItems::clear
    // is also called.
    void empty_heap() final;

    // Push an item onto a bucket.
    void push_back(t_heap* hptr) final;

    void add_to_heap(t_heap* hptr) final {
        push_back(hptr);
    }

    void build_heap() final {
    }

    void set_prune_limit(size_t max_index, size_t prune_limit) final;

    // Pop an item from the cheapest non-empty bucket.
    //
    // Returns nullptr if empty.
    t_heap* get_heap_head() final;

    // True if all buckets are empty.
    bool is_empty_heap() const final {
        return heap_head_ == std::numeric_limits<size_t>::max();
    }

    bool is_valid() const final {
        return true;
    }

    // Sanity check state of buckets (e.g. all items within each bucket have
    // a cost that matches their bucket index.
    void verify();

    // Print items contained in buckets.
    void print();

    void invalidate_heap_entries(int sink_node, int ipin_node) final;

  private:
    // Factor used to convert cost from float to int.  Should be scaled to
    // enable sufficent precision in bucketting.
    static constexpr float kDefaultConvFactor = 1e12;

    // Convert cost from float to integer bucket id.
    int cost_to_int(float cost) const {
        return (int)(cost * conv_factor_);
    }

    // Simple fast random function used for randomizing item selection on pop.
    size_t fast_rand() {
        seed_ = (0x234ab32a1 * seed_) ^ (0x12acbade);
        return seed_;
    }

    void check_scaling();
    void rescale();
    float rescale_func() const;
    void check_conv_factor() const;
    bool check_front_list() const;

    // Expand the number of buckets.
    //
    // Only call if insufficient buckets exist.
    void expand(size_t required_number_of_buckets);

    void prune_heap();

    BucketItems items_; /* Item storage */

    /* Number of t_heap objects alloc'd but not returned to Bucket.
     * Used to verify that clearing is safe. */
    ssize_t outstanding_items_;

    size_t seed_; /* Seed for fast_rand, should be non-zero */

    BucketItem** heap_;      /* Buckets for linked lists*/
    size_t heap_size_;       /* Number of buckets */
    size_t heap_head_;       /* First non-empty bucket */
    size_t heap_tail_;       /* Last non-empty bucket */
    float conv_factor_;      /* Cost bucket scaling factor.
                              *
                              * Larger conv_factor_ means each bucket is
                              * smaller.
                              *
                              * bucket index = cost * conv_factor_
                              *
                              */
    float division_scaling_; /* Scaling factor used during rescaling.
                              * Larger division scaling results in larger
                              * conversion factor.
                              */
    ssize_t max_buckets_;    /* Maximum number of buckets to control when to
                              * rescale.
                              */

    float min_cost_; /* Smallest cost seen */
    float max_cost_; /* Largest cost seen */

    size_t num_items_;                 /* Number of items in the bucket heap. */
    size_t max_index_;                 /* Maximum value for index. */
    size_t prune_limit_;               /* Maximum number of elements this bucket heap should
                                        * have before the heap self compacts.
                                        */
    size_t prune_count_;               /* The number of times the bucket heap has self
                                        * compacted.
                                        */
    std::vector<float> min_push_cost_; /* Lowest push cost for each index.
                                        * Only used if the bucket has
                                        * self-pruned.
                                        */

    /* In order to quickly randomly pop an element from the front bucket,
     * a list of items is made.
     *
     * front_head_ points to the heap_ index this array was constructed from.
     * If front_head_ is size_t::max or doesn't equal heap_head_, front_list_
     * needs to be re-computed.
     * */
    size_t front_head_;
    std::vector<BucketItem*> front_list_;
};

#endif /* _BUCKET_H */
