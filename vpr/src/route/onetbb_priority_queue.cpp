#include "vtr_assert.h"
#include "vpr_error.h"

#include "onetbb_priority_queue.h"
#include <atomic>

OneTBBConcurrentPriorityQueue::OneTBBConcurrentPriorityQueue() {
    num_threads_ = 1;
    num_idle_ = num_pq_push_ = num_pq_pop_ = 0;
}

OneTBBConcurrentPriorityQueue::OneTBBConcurrentPriorityQueue(size_t num_threads) {
    num_threads_ = num_threads;
    num_idle_ = num_pq_push_ = num_pq_pop_ = 0;
}

OneTBBConcurrentPriorityQueue::OneTBBConcurrentPriorityQueue(size_t num_threads, size_t dont_care) {
    (void)dont_care;
    num_threads_ = num_threads;
    num_idle_ = num_pq_push_ = num_pq_pop_ = 0;
}

OneTBBConcurrentPriorityQueue::~OneTBBConcurrentPriorityQueue() {}

void OneTBBConcurrentPriorityQueue::init_heap(const DeviceGrid& grid) {
    (void)grid;
    // TODO: Reserve storage for the priority queue
}

bool OneTBBConcurrentPriorityQueue::try_pop(pq_node_t &pq_top) {
    pq_pair_t tmp;
    if (!pq_.try_pop(tmp)) {
        size_t num = num_idle_.fetch_add(1, std::memory_order_relaxed) + 1;
        do {
            if (pq_.try_pop(tmp)) break;
            if (num >= num_threads_) return false;
            num = num_idle_.load(std::memory_order_relaxed);
        } while (true);
        num_idle_.fetch_sub(1, std::memory_order_relaxed);
    }
    pq_top = std::get<1>(tmp);
    num_pq_pop_.fetch_add(1); // TODO
    return true;
}

void OneTBBConcurrentPriorityQueue::add_to_heap(const pq_node_t& hptr) {
    this->push_back(hptr);
}

void OneTBBConcurrentPriorityQueue::push_back(const pq_node_t& hptr) {
    pq_.push({hptr.cost, hptr});
    num_pq_push_.fetch_add(1);
}

bool OneTBBConcurrentPriorityQueue::is_empty_heap() const {
    return (bool)(pq_.empty());
}

bool OneTBBConcurrentPriorityQueue::is_valid() const {
    return true;
}

void OneTBBConcurrentPriorityQueue::empty_heap() {
    pq_.clear();
    VTR_ASSERT(is_empty_heap());
}

void OneTBBConcurrentPriorityQueue::build_heap() {}
