#ifndef _PARALLEL_CONNECTION_ROUTER_H
#define _PARALLEL_CONNECTION_ROUTER_H

#include "connection_router.h"

#include "d_ary_heap.h"
#include "multi_queue_d_ary_heap.h"

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

class spin_lock_t {
    std::atomic_flag lock_ = ATOMIC_FLAG_INIT;

  public:
    void acquire() {
        while (std::atomic_flag_test_and_set_explicit(&lock_, std::memory_order_acquire))
            ;
    }

    void release() {
        std::atomic_flag_clear_explicit(&lock_, std::memory_order_release);
    }
};

class barrier_mutex_t {
    std::mutex mutex_;
    std::condition_variable cv_;
    size_t count_;
    size_t max_count_;
    size_t generation_ = 0;

  public:
    explicit barrier_mutex_t(size_t num_threads)
        : count_(num_threads)
        , max_count_(num_threads) {}

    void wait() {
        std::unique_lock<std::mutex> lock{mutex_};
        size_t gen = generation_;
        if (--count_ == 0) {
            generation_++;
            count_ = max_count_;
            cv_.notify_all();
        } else {
            cv_.wait(lock, [this, &gen] { return gen != generation_; });
        }
    }
};

class barrier_spin_t {
    size_t num_threads_ = 1;
    std::atomic<size_t> count_ = 0;
    std::atomic<bool> sense_ = false; // global sense shared by multiple threads
    inline static thread_local bool local_sense_ = false;

  public:
    explicit barrier_spin_t(size_t num_threads) { num_threads_ = num_threads; }

    void init() {
        local_sense_ = false;
    }

    void wait() {
        bool s = !local_sense_;
        local_sense_ = s;
        size_t num_arrivals = count_.fetch_add(1) + 1;
        if (num_arrivals == num_threads_) {
            count_.store(0);
            sense_.store(s);
        } else {
            while (sense_.load() != s)
                ;
        }
    }
};

using barrier_t = barrier_spin_t;

// This class encapsulates the timing driven connection router. This class
// routes from some initial set of sources (via the input rt tree) to a
// particular sink.
//
// When the ParallelConnectionRouter is used, it mutates the provided
// rr_node_route_inf.  The routed path can be found by tracing from the sink
// node (which is returned) through the rr_node_route_inf.  See
// update_traceback as an example of this tracing.
template<typename HeapImplementation>
class ParallelConnectionRouter : public ConnectionRouter<HeapImplementation> {
  public:
    ParallelConnectionRouter(
        const DeviceGrid& grid,
        const RouterLookahead& router_lookahead,
        const t_rr_graph_storage& rr_nodes,
        const RRGraphView* rr_graph,
        const std::vector<t_rr_rc_data>& rr_rc_data,
        const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switch_inf,
        vtr::vector<RRNodeId, t_rr_node_route_inf>& rr_node_route_inf,
        bool is_flat,
        int multi_queue_num_threads,
        int multi_queue_num_queues,
        bool multi_queue_direct_draining)
        : ConnectionRouter<HeapImplementation>(grid, router_lookahead, rr_nodes, rr_graph, rr_rc_data, rr_switch_inf, rr_node_route_inf, is_flat)
        , modified_rr_node_inf_(multi_queue_num_threads)
        , heap_(multi_queue_num_threads, multi_queue_num_queues)
        , thread_barrier_(multi_queue_num_threads)
        , is_router_destroying_(false)
        , locks_(rr_node_route_inf.size())
        , multi_queue_direct_draining_(multi_queue_direct_draining) {
        this->sub_threads_.resize(multi_queue_num_threads - 1);
        this->thread_barrier_.init();
        for (int i = 0; i < multi_queue_num_threads - 1; ++i) {
            this->sub_threads_[i] = std::thread(&ParallelConnectionRouter::timing_driven_find_single_shortest_path_from_heap_sub_thread_wrapper, this, i + 1 /*0: main thread*/);
            this->sub_threads_[i].detach();
        }
    }

    ~ParallelConnectionRouter() {
        this->is_router_destroying_ = true;
        this->thread_barrier_.wait();

        VTR_LOG("Parallel Connection Router is being destroyed. Time spent on path search: %.3f seconds.\n",
                std::chrono::duration<float /*convert to seconds by default*/>(this->path_search_cumulative_time).count());
    }

    void clear_modified_rr_node_info() final {
        for (auto& thread_visited_rr_nodes : this->modified_rr_node_inf_) {
            thread_visited_rr_nodes.clear();
        }
    }

    void reset_path_costs() final {
        auto& route_ctx = g_vpr_ctx.mutable_routing();
        for (const auto& thread_visited_rr_nodes : this->modified_rr_node_inf_) {
            for (const auto node : thread_visited_rr_nodes) {
                route_ctx.rr_node_route_inf[node].path_cost = std::numeric_limits<float>::infinity();
                route_ctx.rr_node_route_inf[node].backward_path_cost = std::numeric_limits<float>::infinity();
                route_ctx.rr_node_route_inf[node].prev_edge = RREdgeId::INVALID();
            }
        }
    }

    void set_rcv_enabled(bool) final {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "RCV for parallel connection router not yet implemented. Not expected to be called.");
    }

    vtr::vector<RRNodeId, RTExploredNode> timing_driven_find_all_shortest_paths_from_route_tree(
        const RouteTreeNode&,
        const t_conn_cost_params&,
        const t_bb&,
        RouterStats&,
        const ConnectionParameters&) final {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "timing_driven_find_all_shortest_paths_from_route_tree not yet implemented (nor is the focus of this project). Not expected to be called.");
    }

  protected:
    // Mark that data associated with rr_node "inode" has been modified, and
    // needs to be reset in reset_path_costs.
    inline void add_to_mod_list(RRNodeId inode, size_t thread_idx) {
        if (std::isinf(this->rr_node_route_inf_[inode].path_cost)) {
            this->modified_rr_node_inf_[thread_idx].push_back(inode);
        }
    }

    // Update the route path to the node `cheapest.index` via the path from
    // `from_node` via `cheapest.prev_edge`.
    inline void update_cheapest(RTExploredNode& cheapest, size_t thread_idx) {
        const RRNodeId& inode = cheapest.index;
        add_to_mod_list(inode, thread_idx);
        this->rr_node_route_inf_[inode].prev_edge = cheapest.prev_edge;
        this->rr_node_route_inf_[inode].path_cost = cheapest.total_cost;
        this->rr_node_route_inf_[inode].backward_path_cost = cheapest.backward_path_cost;
    }

    inline void obtainSpinLock(const RRNodeId& inode) {
        this->locks_[size_t(inode)].acquire();
    }

    inline void releaseLock(const RRNodeId& inode) {
        this->locks_[size_t(inode)].release();
    }

    // Find the shortest path from current heap to the sink node in the RR graph
    void timing_driven_find_single_shortest_path_from_heap(RRNodeId sink_node,
                                                           const t_conn_cost_params& cost_params,
                                                           const t_bb& bounding_box,
                                                           const t_bb& target_bb) final;

    void timing_driven_find_single_shortest_path_from_heap_sub_thread_wrapper(
        const size_t thread_idx);

    void timing_driven_find_single_shortest_path_from_heap_thread_func(
        RRNodeId sink_node,
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box,
        const t_bb& target_bb,
        const size_t thread_idx);

    // Expand each neighbor of the current node.
    void timing_driven_expand_neighbours(
        const RTExploredNode& current,
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box,
        RRNodeId target_node,
        const t_bb& target_bb,
        size_t thread_idx);

    // Conditionally adds to_node to the router heap (via path from current.index
    // via from_edge).
    //
    // RR nodes outside bounding box specified in bounding_box are not added
    // to the heap.
    void timing_driven_expand_neighbour(
        const RTExploredNode& current,
        RREdgeId from_edge,
        RRNodeId to_node,
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box,
        RRNodeId target_node,
        const t_bb& target_bb,
        size_t thread_idx);

    // Add to_node to the heap, and also add any nodes which are connected by
    // non-configurable edges
    void timing_driven_add_to_heap(
        const t_conn_cost_params& cost_params,
        const RTExploredNode& current,
        RRNodeId to_node,
        RREdgeId from_edge,
        RRNodeId target_node,
        size_t thread_idx);

    // TODO: move this function into ConnectionRouter class
    //Unconditionally adds rt_node to the heap
    //
    //Note that if you want to respect rt_node->re_expand that is the caller's
    //responsibility.
    void add_route_tree_node_to_heap(
        const RouteTreeNode& rt_node,
        RRNodeId target_node,
        const t_conn_cost_params& cost_params,
        const t_bb& net_bb);

    // Find paths from current heap to all nodes in the RR graph
    vtr::vector<RRNodeId, RTExploredNode> timing_driven_find_all_shortest_paths_from_heap(
        const t_conn_cost_params&,
        const t_bb&) final {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "timing_driven_find_all_shortest_paths_from_heap not yet implemented (nor is the focus of this project). Not expected to be called.");
    }

    std::vector<std::vector<RRNodeId>> modified_rr_node_inf_;
    MultiQueueDAryHeap<HeapImplementation::arg_D> heap_;
    std::vector<std::thread> sub_threads_;
    barrier_t thread_barrier_;
    std::atomic<bool> is_router_destroying_;
    std::vector<spin_lock_t> locks_;
    bool multi_queue_direct_draining_;

    std::atomic<RRNodeId*> sink_node_;
    std::atomic<t_conn_cost_params*> cost_params_;
    std::atomic<t_bb*> bounding_box_;
    std::atomic<t_bb*> target_bb_;
};

/** Construct a parallel connection router that uses the specified heap type.
 * This function is not used, but removing it will result in "undefined reference"
 * errors since heap type specializations won't get emitted from parallel_connection_router.cpp
 * without it.
 * The alternative is moving all ParallelConnectionRouter fn implementations into the header. */
std::unique_ptr<ConnectionRouterInterface> make_parallel_connection_router(
    e_heap_type heap_type,
    const DeviceGrid& grid,
    const RouterLookahead& router_lookahead,
    const t_rr_graph_storage& rr_nodes,
    const RRGraphView* rr_graph,
    const std::vector<t_rr_rc_data>& rr_rc_data,
    const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switch_inf,
    vtr::vector<RRNodeId, t_rr_node_route_inf>& rr_node_route_inf,
    bool is_flat,
    int multi_queue_num_threads,
    int multi_queue_num_queues,
    bool multi_queue_direct_draining);

#endif /* _PARALLEL_CONNECTION_ROUTER_H */
