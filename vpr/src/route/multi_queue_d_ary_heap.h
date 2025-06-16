#pragma once
/********************************************************************
 * MultiQueue Implementation
 *
 * Originally authored by Guozheng Zhang, Gilead Posluns, and Mark C. Jeffrey
 * Published at the 36th ACM Symposium on Parallelism in Algorithms and
 * Architectures (SPAA), June 2024
 *
 * Original source: https://github.com/mcj-group/cps
 *
 * This implementation and interface has been modified from the original to:
 *   - Support queue draining functionality
 *   - Enable integration with the VTR project
 *
 * The MultiQueue data structure provides an efficient concurrent priority
 * queue implementation designed for parallel processing applications.
 *
 * Modified: February 2025
 ********************************************************************/

#include "device_grid.h"
#include "heap_type.h"
#include "multi_queue_d_ary_heap.tpp"
#include <tuple>
#include <memory>

// FIXME: Use unified heap node struct (HeapNodeId) and comparator (HeapNodeComparator)
// defined in heap_type.h. Currently, the MQ_IO is not compatible with them. Need a lot
// of refactoring in MQ_IO to make it work, which is left for another PR to clean it up.
using MQHeapNode = std::tuple<HeapNodePriority, uint32_t /* FIXME: Use HeapNodeId */>;

struct MQHeapNodeTupleComparator /* FIXME: Use HeapNodeComparator */ {
    bool operator()(const MQHeapNode& u, const MQHeapNode& v) {
        return std::get<0>(u) > std::get<0>(v);
    }
};

template<unsigned D>
class MultiQueueDAryHeap {
  public:
    using MQ_IO = MultiQueueIO<D, MQHeapNode, MQHeapNodeTupleComparator, HeapNodePriority>;

    MultiQueueDAryHeap() {
        set_num_threads_and_queues(1, 2); // Serial (#threads=1, #queues=2) by default
    }

    MultiQueueDAryHeap(size_t num_threads, size_t num_queues) {
        set_num_threads_and_queues(num_threads, num_queues);
    }

    ~MultiQueueDAryHeap() {}

    void set_num_threads_and_queues(size_t num_threads, size_t num_queues) {
        pq_.reset();
        // Note: BE AWARE that in MQ_IO interface, `num_queues` comes first, then `num_threads`!
        pq_ = std::make_unique<MQ_IO>(num_queues, num_threads, 0 /*Dont care (batch size for only popBatch)*/);
    }

    void init_heap(const DeviceGrid& grid) {
        (void)grid;
        // TODO: Reserve storage for MQ_IO
        // Note: This function could be called before setting num_threads/num_queues
    }

    bool try_pop(HeapNode& heap_node) {
        auto tmp = pq_->tryPop();
        if (!tmp.has_value()) {
            return false;
        } else {
            uint32_t node_id;
            std::tie(heap_node.prio, node_id) = tmp.value(); // FIXME: eliminate type cast by modifying MQ_IO
            heap_node.node = RRNodeId(node_id);
            return true;
        }
    }

    void add_to_heap(const HeapNode& heap_node) {
        HeapNodePriority prio = heap_node.prio;
        uint32_t node = size_t(heap_node.node);
        pq_->push({prio, node});
    }

    void push_back(const HeapNode& heap_node) {
        HeapNodePriority prio = heap_node.prio;
        uint32_t node = size_t(heap_node.node);
        pq_->push({prio, node}); // FIXME: add to heap without maintaining the heap property
    }

    void build_heap() {
        // FIXME: restore the heap property after pushing back nodes
    }

    bool is_valid() const {
        return true; // FIXME: checking if the heap property is maintained or not
    }

    void empty_heap() {
        pq_->reset(); // TODO: check if adding clear function for MQ_IO is necessary
    }

    bool is_empty_heap() const {
        return (bool)(pq_->empty());
    }

    uint64_t get_num_pushes() const {
        return pq_->getNumPushes();
    }

    uint64_t get_num_pops() const {
        return pq_->getNumPops();
    }

    uint64_t get_heap_occupancy() const {
        return pq_->getQueueOccupancy();
    }

    void reset() {
        pq_->reset();
    }

#ifdef MQ_IO_ENABLE_CLEAR_FOR_POP
    void set_min_priority_for_pop(const HeapNodePriority& minPrio) {
        pq_->setMinPrioForPop(minPrio);
    }
#endif

  private:
    std::unique_ptr<MQ_IO> pq_;
};
