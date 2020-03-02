#ifndef _BUCKET_H
#define _BUCKET_H

#include <vector>

#include "heap_type.h"

struct BucketItem {
    t_heap item;
    BucketItem* next_bucket;
};

static_assert(offsetof(BucketItem, item) == 0,
              "BucketItem::item must have an offset of 0");

// Allocator for t_heap items.
//
// Supports fast clearing of the items, under the assumption that when clear
// is invoked, all outstanding references can be dropped.  This should be true
// between net routing, and avoids the need to rebuild the free list between
// nets.
class BucketItems {
  public:
    BucketItems() noexcept;

    // Returns all allocated items to be available for allocation.
    //
    // This operation is only safe if all outstanding references are discarded.
    // This is true when the router is starting on a new net, as all outstanding
    // items should be in the bucket, which is cleared at the start of routing.
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
            temp_ptr = heap_items_[alloced_items_++];
        } else {
            if (heap_free_head_ == nullptr) { /* No elements on the free list */
                heap_free_head_ = vtr::chunk_new<BucketItem>(&heap_ch_);
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
// The Bucket contains linked lists for costs at kConvFactor intervals.  Given
// that cost is approximately delay, each bucket contains ~1 picosecond (1e12)
// worth items.
//
// Items are pushed into the linked list that matches their cost [0, 1)
// picosecond.  When popping the Bucket, a random item in the cheapest bucket
// with items is returned.  This randomization exists to prevent the router
// from following identical paths when operating with identical costs.
// Consider two parallel paths to a node.
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
        return &items_.alloc_item()->item;
    }
    void free(t_heap* hptr) final {
        // Static assert ensures that BucketItem::item is at offset 0,
        // so this cast is safe.
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
    int cost_to_int(float cost) {
        return (int)(cost * conv_factor_);
    }

    // Simple fast random function used for randomizing item selection on pop.
    size_t fast_rand() {
        seed_ = (0x234ab32a1 * seed_) ^ (0x12acbade);
        return seed_;
    }

    void check_scaling();

    // Expand the number of buckets.
    //
    // Only call if insufficient bucets exist.
    void expand(size_t required_number_of_buckets);

    BucketItems items_; /* Item storage */

    size_t seed_; /* Seed for fast_rand, should be non-zero */

    BucketItem** heap_; /* Buckets for linked lists*/
    size_t heap_size_;  /* Number of buckets */
    size_t heap_head_;  /* First non-empty bucket */
    size_t heap_tail_;  /* Last non-empty bucket */
    float conv_factor_; /* Cost bucket scaling factor */

    float min_cost_; /* Smallest cost seen */
    float max_cost_; /* Large cost seen */
};

#endif /* _BUCKET_H */
