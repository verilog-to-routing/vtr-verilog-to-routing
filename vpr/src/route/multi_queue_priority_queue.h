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

class MultiQueuePriorityQueue {
  public:
    using pq_pair_t = std::tuple<float /*priority*/, pq_node_t>;
    struct pq_compare {
        bool operator()(const pq_pair_t& u, const pq_pair_t& v) {
            return std::get<0>(u) > std::get<0>(v);
        }
    };
    using MQ_IO = MultiQueueIO<pq_compare, float, pq_node_t>;

    MultiQueuePriorityQueue();
    MultiQueuePriorityQueue(size_t num_threads, size_t num_queues);
    ~MultiQueuePriorityQueue();

    void init_heap(const DeviceGrid& grid);
    bool try_pop(pq_node_t &pq_top);
    void add_to_heap(const pq_node_t& hptr);
    void push_back(const pq_node_t& hptr);
    bool is_empty_heap() const;
    bool is_valid() const;
    void empty_heap();
    void build_heap();

  private:
    MQ_IO* pq_;
    // std::vector<pq_node_t> push_batch_buffer_;
};

#endif /* _MULTI_QUEUE_PRIORITY_QUEUE_H */
