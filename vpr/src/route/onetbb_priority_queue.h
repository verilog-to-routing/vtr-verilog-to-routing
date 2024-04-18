#ifndef _ONE_TBB_CONCURRENT_PRIORITY_QUEUE_H
#define _ONE_TBB_CONCURRENT_PRIORITY_QUEUE_H

#include "heap_type.h"
#include "rr_graph_fwd.h"

#include <atomic>

#include "oneapi/tbb/concurrent_priority_queue.h"

struct pq_node_t {
    float cost = 0.;
    float backward_path_cost = 0.;
    float R_upstream = 0.;

    RRNodeId index = RRNodeId::INVALID(); // TODO: to be optimized

    uint32_t prev_edge_id;

    pq_node_t() {}

    constexpr RREdgeId prev_edge() const {
        static_assert(sizeof(uint32_t) == sizeof(RREdgeId));
        return RREdgeId(prev_edge_id);
    }

    inline void set_prev_edge(RREdgeId edge) {
        static_assert(sizeof(uint32_t) == sizeof(RREdgeId));
        prev_edge_id = size_t(edge);
    }
};

class OneTBBConcurrentPriorityQueue {
  public:
    using pq_pair_t = std::tuple<float /*priority*/, pq_node_t>;
    struct pq_compare {
        bool operator()(const pq_pair_t& u, const pq_pair_t& v) {
            return std::get<0>(u) > std::get<0>(v);
        }
    };

    OneTBBConcurrentPriorityQueue();
    OneTBBConcurrentPriorityQueue(size_t num_threads);
    OneTBBConcurrentPriorityQueue(size_t num_threads, size_t dont_care);
    ~OneTBBConcurrentPriorityQueue();

    void init_heap(const DeviceGrid& grid);
    bool try_pop(pq_node_t &pq_top);
    void add_to_heap(const pq_node_t& hptr);
    void push_back(const pq_node_t& hptr);
    bool is_empty_heap() const;
    bool is_valid() const;
    void empty_heap();
    void build_heap();
    inline uint64_t getNumPushes() const { return num_pq_push_; }
    inline uint64_t getNumPops() const { return num_pq_pop_; }
    inline void reset() { num_idle_ = num_pq_push_ = num_pq_pop_ = 0; this->empty_heap(); }

  private:
    oneapi::tbb::concurrent_priority_queue<pq_pair_t, pq_compare> pq_;
    size_t num_threads_;
    std::atomic<size_t> num_idle_;
    std::atomic<size_t> num_pq_push_;
    std::atomic<size_t> num_pq_pop_;
};

#endif /* _ONE_TBB_CONCURRENT_PRIORITY_QUEUE_H */
