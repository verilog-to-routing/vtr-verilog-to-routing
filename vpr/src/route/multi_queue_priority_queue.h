#ifndef _MULTI_QUEUE_PRIORITY_QUEUE_H
#define _MULTI_QUEUE_PRIORITY_QUEUE_H

// #define MQ_IO_ENABLE_CLEAR_FOR_POP

#include "heap_type.h"

#include "MultiQueueIO.h"

#include "rr_graph_fwd.h"

using pq_prio_t = float;
using pq_index_t = uint32_t;

class MultiQueuePriorityQueue {
  public:
    using pq_pair_t = std::tuple<pq_prio_t /*priority*/, pq_index_t>;
    struct pq_compare {
        bool operator()(const pq_pair_t& u, const pq_pair_t& v) {
            return std::get<0>(u) > std::get<0>(v);
        }
    };
    using MQ_IO = MultiQueueIO<pq_compare, pq_prio_t, pq_index_t>;

    MultiQueuePriorityQueue();
    MultiQueuePriorityQueue(size_t num_threads, size_t num_queues);
    ~MultiQueuePriorityQueue();

    void init_heap(const DeviceGrid& grid);
    bool try_pop(pq_prio_t &prio, RRNodeId &node);
    void add_to_heap(const pq_prio_t& prio, const RRNodeId& node);
    void push_back(const pq_prio_t& prio, const RRNodeId& node);
    bool is_empty_heap() const;
    bool is_valid() const;
    void empty_heap();
    void build_heap();
    inline uint64_t getNumPushes() const { return pq_->getNumPushes(); }
    inline uint64_t getNumPops() const { return pq_->getNumPops(); }
    inline uint64_t getHeapOccupancy() const { return pq_->getQueueOccupancy(); }
    inline void reset() { pq_->reset(); }
#ifdef MQ_IO_ENABLE_CLEAR_FOR_POP
    inline void setMinPrioForPop(const pq_prio_t& minPrio) { pq_->setMinPrioForPop(minPrio); }
#endif

  private:
    MQ_IO* pq_;
    // std::vector<pq_node_t> push_batch_buffer_; // TODO: heap memory reservation
};

#endif /* _MULTI_QUEUE_PRIORITY_QUEUE_H */
