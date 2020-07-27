#include "binary_heap.h"
#include "vtr_log.h"

static size_t parent(size_t i) { return i >> 1; }
// child indices of a heap
static size_t left(size_t i) { return i << 1; }
static size_t right(size_t i) { return (i << 1) + 1; }

BinaryHeap::BinaryHeap()
    : heap_(nullptr)
    , heap_size_(0)
    , heap_tail_(0)
    , max_index_(std::numeric_limits<size_t>::max())
    , prune_limit_(std::numeric_limits<size_t>::max()) {}

BinaryHeap::~BinaryHeap() {
    free_all_memory();
}

t_heap* BinaryHeap::alloc() {
    return storage_.alloc();
}
void BinaryHeap::free(t_heap* hptr) {
    storage_.free(hptr);
}

// Note: malloc()/free() must be used for the heap,
//       or realloc() must be eliminated from add_to_heap()
//       because there is no C++ equivalent.
void BinaryHeap::init_heap(const DeviceGrid& grid) {
    size_t target_heap_size = (grid.width() - 1) * (grid.height() - 1);
    if (heap_ == nullptr || heap_size_ < target_heap_size) {
        if (heap_ != nullptr) {
            // coverity[offset_free : Intentional]
            vtr::free(heap_ + 1);
            heap_ = nullptr;
        }
        heap_size_ = (grid.width() - 1) * (grid.height() - 1);
        heap_ = (t_heap**)vtr::malloc(heap_size_ * sizeof(t_heap*));
        heap_--; /* heap stores from [1..heap_size] */
    }
    heap_tail_ = 1;
}

void BinaryHeap::add_to_heap(t_heap* hptr) {
    expand_heap_if_full();
    // start with undefined hole
    ++heap_tail_;
    sift_up(heap_tail_ - 1, hptr);

    // If we have pruned, rebuild the heap now.
    if (check_prune_limit()) {
        build_heap();
    }
}

bool BinaryHeap::is_empty_heap() const {
    return (bool)(heap_tail_ == 1);
}

t_heap* BinaryHeap::get_heap_head() {
    /* Returns a pointer to the smallest element on the heap, or NULL if the     *
     * heap is empty.  Invalid (index == OPEN) entries on the heap are never     *
     * returned -- they are just skipped over.                                   */

    t_heap* cheapest;
    size_t hole, child;

    do {
        if (heap_tail_ == 1) { /* Empty heap. */
            VTR_LOG_WARN("Empty heap occurred in get_heap_head.\n");
            return (nullptr);
        }

        cheapest = heap_[1];

        hole = 1;
        child = 2;
        --heap_tail_;
        while (child < heap_tail_) {
            if (heap_[child + 1]->cost < heap_[child]->cost)
                ++child; // become right child
            heap_[hole] = heap_[child];
            hole = child;
            child = left(child);
        }
        sift_up(hole, heap_[heap_tail_]);

    } while (cheapest->index == OPEN); /* Get another one if invalid entry. */

    return (cheapest);
}

void BinaryHeap::empty_heap() {
    for (size_t i = 1; i < heap_tail_; i++)
        free(heap_[i]);

    heap_tail_ = 1;
}

size_t BinaryHeap::size() const { return heap_tail_ - 1; } // heap[0] is not valid element

// make a heap rooted at index hole by **sifting down** in O(lgn) time
void BinaryHeap::sift_down(size_t hole) {
    t_heap* head{heap_[hole]};
    size_t child{left(hole)};
    while (child < heap_tail_) {
        if (child + 1 < heap_tail_ && heap_[child + 1]->cost < heap_[child]->cost)
            ++child;
        if (heap_[child]->cost < head->cost) {
            heap_[hole] = heap_[child];
            hole = child;
            child = left(child);
        } else
            break;
    }
    heap_[hole] = head;
}

// runs in O(n) time by sifting down; the least work is done on the most elements: 1 swap for bottom layer, 2 swap for 2nd, ... lgn swap for top
// 1*(n/2) + 2*(n/4) + 3*(n/8) + ... + lgn*1 = 2n (sum of i/2^i)
void BinaryHeap::build_heap() {
    // second half of heap are leaves
    for (size_t i = heap_tail_ >> 1; i != 0; --i)
        sift_down(i);
}

void BinaryHeap::set_prune_limit(size_t max_index, size_t prune_limit) {
    if (prune_limit != std::numeric_limits<size_t>::max()) {
        VTR_ASSERT(max_index < prune_limit);
    }
    max_index_ = max_index;
    prune_limit_ = prune_limit;
}

// O(lgn) sifting up to maintain heap property after insertion (should sift down when building heap)
void BinaryHeap::sift_up(size_t leaf, t_heap* const node) {
    while ((leaf > 1) && (node->cost < heap_[parent(leaf)]->cost)) {
        // sift hole up
        heap_[leaf] = heap_[parent(leaf)];
        leaf = parent(leaf);
    }
    heap_[leaf] = node;
}

void BinaryHeap::expand_heap_if_full() {
    if (heap_tail_ > heap_size_) { /* Heap is full */
        heap_size_ *= 2;
        heap_ = (t_heap**)vtr::realloc((void*)(heap_ + 1),
                                       heap_size_ * sizeof(t_heap*));
        heap_--; /* heap goes from [1..heap_size] */
    }
}

// adds an element to the back of heap and expand if necessary, but does not maintain heap property
void BinaryHeap::push_back(t_heap* const hptr) {
    expand_heap_if_full();
    heap_[heap_tail_] = hptr;
    ++heap_tail_;

    check_prune_limit();
}

bool BinaryHeap::is_valid() const {
    if (heap_ == nullptr) {
        return false;
    }

    for (size_t i = 1; i <= heap_tail_ >> 1; ++i) {
        if (left(i) < heap_tail_ && heap_[left(i)]->cost < heap_[i]->cost) return false;
        if (right(i) < heap_tail_ && heap_[right(i)]->cost < heap_[i]->cost) return false;
    }
    return true;
}

void BinaryHeap::invalidate_heap_entries(int sink_node, int ipin_node) {
    /* marks all the heap entries consisting of sink_node, where it was reached
     * via ipin_node, as invalid (open).  used only by the breadth_first router
     * and even then only in rare circumstances.
     *
     * This function enables forcing the breadth-first router to route to a
     * sink more than once, using multiple ipins, which is useful in some
     * architectures.
     * */

    for (size_t i = 1; i < heap_tail_; i++) {
        if (heap_[i]->index == sink_node) {
            if (heap_[i]->prev_node() == ipin_node) {
                heap_[i]->index = OPEN; /* Invalid. */
                break;
            }
        }
    }
}

void BinaryHeap::free_all_memory() {
    if (heap_ != nullptr) {
        empty_heap();

        // coverity[offset_free : Intentional]
        vtr::free(heap_ + 1);
    }

    heap_ = nullptr; /* Defensive coding:  crash hard if I use these. */

    storage_.free_all_memory();
}

bool BinaryHeap::check_prune_limit() {
    if (heap_tail_ > prune_limit_) {
        prune_heap();
        return true;
    }

    return false;
}

void BinaryHeap::prune_heap() {
    VTR_ASSERT(max_index_ < prune_limit_);

    std::vector<t_heap*> best_heap_item(max_index_, nullptr);

    // Find the cheapest instance of each index and store it.
    for (size_t i = 1; i < heap_tail_; i++) {
        if (heap_[i] == nullptr) {
            continue;
        }

        if (heap_[i]->index == OPEN) {
            free(heap_[i]);
            heap_[i] = nullptr;
            continue;
        }

        VTR_ASSERT(static_cast<size_t>(heap_[i]->index) < max_index_);

        if (best_heap_item[heap_[i]->index] == nullptr || best_heap_item[heap_[i]->index]->cost > heap_[i]->cost) {
            best_heap_item[heap_[i]->index] = heap_[i];
        }
    }

    // Free unused nodes.
    for (size_t i = 1; i < heap_tail_; i++) {
        if (heap_[i] == nullptr) {
            continue;
        }

        if (best_heap_item[heap_[i]->index] != heap_[i]) {
            free(heap_[i]);
            heap_[i] = nullptr;
        }
    }

    heap_tail_ = 1;

    for (size_t i = 0; i < max_index_; ++i) {
        if (best_heap_item[i] != nullptr) {
            heap_[heap_tail_++] = best_heap_item[i];
        }
    }
}
