#include "multi_queue_priority_queue.h"
#include "vtr_assert.h"
#include "vpr_error.h"

MultiQueuePriorityQueue::MultiQueuePriorityQueue() {
    pq_ = new MQ_IO(2, 1, 0); // Serial (#threads=1, #queues=2) by default
}

MultiQueuePriorityQueue::MultiQueuePriorityQueue(size_t num_threads, size_t num_queues) {
    pq_ = new MQ_IO(num_queues, num_threads, 0 /*Dont care (batch size for only popBatch)*/);
}

MultiQueuePriorityQueue::~MultiQueuePriorityQueue() {
    delete pq_;
}

void MultiQueuePriorityQueue::init_heap(const DeviceGrid& grid) {
    (void)grid;
    // TODO: Reserve storage for MQ_IO
}

bool MultiQueuePriorityQueue::try_pop(pq_node_t &pq_top) {
    auto tmp = pq_->tryPop();
    if (!tmp) {
        return false;
    } else {
        pq_top = std::get<1>(tmp.get());
        return true;
    }
}

void MultiQueuePriorityQueue::add_to_heap(const pq_node_t& hptr) {
    pq_->push({hptr.cost, hptr});
}

void MultiQueuePriorityQueue::push_back(const pq_node_t& hptr) {
    // push_batch_buffer_.push_back({hptr.cost, hptr});
    pq_->push({hptr.cost, hptr});
}

bool MultiQueuePriorityQueue::is_empty_heap() const {
    return (bool)(pq_->empty());
}

bool MultiQueuePriorityQueue::is_valid() const {
    return true;
}

void MultiQueuePriorityQueue::empty_heap() {
    while (pq_->tryPop()) {}
    VTR_ASSERT(is_empty_heap());
}

void MultiQueuePriorityQueue::build_heap() {
    /* const size_t buf_size = push_batch_buffer_.size();
    if (buf_size > 0) {
        // Note that `pushBatch` inserts a batch of data into the STL priority queue (PQ), resulting
        // in huge PQ maintenance overhead, which is not as efficient as the single maintenance (i.e.,
        // build_heap) demonstrated in the BinaryHeap implementation.
        // Besides, `pushBatch` may cause one queue has a large amount of data, potentially leading
        // to load imbalance issues.
        // TODO: MQ_IO improvement.
        pq_->pushBatch(buf_size, push_batch_buffer_.data());
        push_batch_buffer_.clear();
    } */
}
