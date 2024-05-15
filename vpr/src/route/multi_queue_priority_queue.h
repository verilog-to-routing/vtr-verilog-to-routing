#ifndef _MULTI_QUEUE_PRIORITY_QUEUE_H
#define _MULTI_QUEUE_PRIORITY_QUEUE_H

#include "heap_type.h"

#include "MultiQueueIO.h"

#include "rr_graph_fwd.h"

struct pq_node_t {
    float cost = 0.;
    float backward_path_cost = 0.;
    float R_upstream = 0.;

    RRNodeId index = RRNodeId::INVALID(); // TODO: to be optimized

    uint32_t prev_edge_id;

    pq_node_t() {}
    pq_node_t(size_t dont_care) { (void)dont_care; } // for MQ compatibility

    constexpr RREdgeId prev_edge() const {
        static_assert(sizeof(uint32_t) == sizeof(RREdgeId));
        return RREdgeId(prev_edge_id);
    }

    inline void set_prev_edge(RREdgeId edge) {
        static_assert(sizeof(uint32_t) == sizeof(RREdgeId));
        prev_edge_id = size_t(edge);
    }
};


using pq_prio_t = float;
using pq_index_t = RRNodeId;

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
    bool try_pop(pq_prio_t &prio, pq_index_t &node);
    void add_to_heap(const pq_prio_t& prio, const pq_index_t& node);
    void push_back(const pq_prio_t& prio, const pq_index_t& node);
    bool is_empty_heap() const;
    bool is_valid() const;
    void empty_heap();
    void build_heap();
    inline uint64_t getNumPushes() const { return pq_->getNumPushes(); }
    inline uint64_t getNumPops() const { return pq_->getNumPops(); }
    inline void reset() { pq_->reset(); }

  private:
    MQ_IO* pq_;
    // std::vector<pq_node_t> push_batch_buffer_; // TODO: heap memory reservation
};

#endif /* _MULTI_QUEUE_PRIORITY_QUEUE_H */
