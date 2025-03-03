#ifndef _PARALLEL_CONNECTION_ROUTER_H
#define _PARALLEL_CONNECTION_ROUTER_H

#include "connection_router_interface.h"
#include "rr_graph_storage.h"
#include "route_common.h"
#include "router_lookahead.h"
#include "route_tree.h"
#include "rr_rc_data.h"
#include "router_stats.h"
#include "spatial_route_tree_lookup.h"

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
        while (std::atomic_flag_test_and_set_explicit(&lock_, std::memory_order_acquire));
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
    explicit barrier_mutex_t(size_t num_threads) : count_(num_threads), max_count_(num_threads) { }

    void wait() {
        std::unique_lock<std::mutex> lock{mutex_};
        size_t gen = generation_;
        if (--count_ == 0) {
            generation_ ++;
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
            while (sense_.load() != s) ;
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
class ParallelConnectionRouter : public ConnectionRouterInterface {
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
        : grid_(grid)
        , router_lookahead_(router_lookahead)
        , rr_nodes_(rr_nodes.view())
        , rr_graph_(rr_graph)
        , rr_rc_data_(rr_rc_data.data(), rr_rc_data.size())
        , rr_switch_inf_(rr_switch_inf.data(), rr_switch_inf.size())
        , net_terminal_groups(g_vpr_ctx.routing().net_terminal_groups)
        , net_terminal_group_num(g_vpr_ctx.routing().net_terminal_group_num)
        , rr_node_route_inf_(rr_node_route_inf)
        , is_flat_(is_flat)
        , modified_rr_node_inf_(multi_queue_num_threads)
        , router_stats_(nullptr)
        , heap_(multi_queue_num_threads, multi_queue_num_queues)
        , thread_barrier_(multi_queue_num_threads)
        , is_router_destroying_(false)
        , locks_(rr_node_route_inf.size())
        , multi_queue_direct_draining_(multi_queue_direct_draining)
        , router_debug_(false)
        , path_search_cumulative_time(0) {
        heap_.init_heap(grid);
        only_opin_inter_layer = (grid.get_num_layers() > 1) && inter_layer_connections_limited_to_opin(*rr_graph);

        sub_threads_.resize(multi_queue_num_threads - 1);
        thread_barrier_.init();
        for (int i = 0 ; i < multi_queue_num_threads - 1; ++i) {
            sub_threads_[i] = std::thread(&ParallelConnectionRouter::timing_driven_route_connection_from_heap_sub_thread_wrapper, this, i + 1 /*0: main thread*/);
            sub_threads_[i].detach();
        }
    }

    ~ParallelConnectionRouter() {
        is_router_destroying_ = true;
        thread_barrier_.wait();

        VTR_LOG("Parallel Connection Router is being destroyed. Time spent on path search: %.3f seconds.\n",
                std::chrono::duration<float/*convert to seconds by default*/>(path_search_cumulative_time).count());
    }

    // Clear's the modified list.  Should be called after reset_path_costs
    // have been called.
    void clear_modified_rr_node_info() final {
        for (auto& thread_visited_rr_nodes : modified_rr_node_inf_) {
            thread_visited_rr_nodes.clear();
        }
    }

    // Reset modified data in rr_node_route_inf based on modified_rr_node_inf.
    // Derived from `reset_path_costs` from route_common.cpp as a specific version
    // for the parallel connection router.
    void reset_path_costs() final {
        auto& route_ctx = g_vpr_ctx.mutable_routing();
        for (const auto& thread_visited_rr_nodes : modified_rr_node_inf_) {
            for (const auto node : thread_visited_rr_nodes) {
                route_ctx.rr_node_route_inf[node].path_cost = std::numeric_limits<float>::infinity();
                route_ctx.rr_node_route_inf[node].backward_path_cost = std::numeric_limits<float>::infinity();
                route_ctx.rr_node_route_inf[node].prev_edge = RREdgeId::INVALID();
            }
        }
    }

    /** Finds a path from the route tree rooted at rt_root to sink_node.
     * This is used when you want to allow previous routing of the same net to
     * serve as valid start locations for the current connection.
     *
     * Returns a tuple of:
     * bool: path exists? (hard failure, rr graph disconnected)
     * bool: should retry with full bounding box? (only used in parallel routing)
     * RTExploredNode: the explored sink node, from which the cheapest path can be found via back-tracing */
    std::tuple<bool, bool, RTExploredNode> timing_driven_route_connection_from_route_tree(
        const RouteTreeNode& rt_root,
        RRNodeId sink_node,
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box,
        RouterStats& router_stats,
        const ConnectionParameters& conn_params) final;

    /** Finds a path from the route tree rooted at rt_root to sink_node for a
     * high fanout net.
     *
     * Unlike timing_driven_route_connection_from_route_tree(), only part of
     * the route tree which is spatially close to the sink is added to the heap.
     *
     * Returns a tuple of:
     * bool: path exists? (hard failure, rr graph disconnected)
     * bool: should retry with full bounding box? (only used in parallel routing)
     * RTExploredNode: the explored sink node, from which the cheapest path can be found via back-tracing */
    std::tuple<bool, bool, RTExploredNode> timing_driven_route_connection_from_route_tree_high_fanout(
        const RouteTreeNode& rt_root,
        RRNodeId sink_node,
        const t_conn_cost_params& cost_params,
        const t_bb& net_bounding_box,
        const SpatialRouteTreeLookup& spatial_rt_lookup,
        RouterStats& router_stats,
        const ConnectionParameters& conn_params) final;

    // Finds a path from the route tree rooted at rt_root to all sinks
    // available.
    //
    // Each element of the returned vector is a reachable sink.
    //
    // If cost_params.astar_fac is set to 0, this effectively becomes
    // Dijkstra's algorithm with a modified exit condition (runs until heap is
    // empty).  When using cost_params.astar_fac = 0, for efficiency the
    // RouterLookahead used should be the NoOpLookahead.
    //
    // Note: This routine is currently used only to generate information that
    // may be helpful in debugging an architecture.
    vtr::vector<RRNodeId, RTExploredNode> timing_driven_find_all_shortest_paths_from_route_tree(
        const RouteTreeNode& rt_root,
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box,
        RouterStats& router_stats,
        const ConnectionParameters& conn_params) final;

    void set_router_debug(bool router_debug) final {
        router_debug_ = router_debug;
    }

    // Empty the route tree set used for RCV node detection
    // Will return if RCV is disabled
    // Called after each net is finished routing to flush the set
    void empty_rcv_route_tree_set() final;

    // Enable or disable RCV in connection router
    // Enabling this will utilize extra path structures, as well as the RCV cost function
    //
    // Ensure route budgets have been calculated before enabling this
    void set_rcv_enabled(bool enable) final;

  private:
    // Mark that data associated with rr_node "inode" has been modified, and
    // needs to be reset in reset_path_costs.
    void add_to_mod_list(RRNodeId inode, size_t thread_idx) {
        if (std::isinf(rr_node_route_inf_[inode].path_cost)) {
            modified_rr_node_inf_[thread_idx].push_back(inode);
        }
    }

    // Update the route path to the node `cheapest.index` via the path from
    // `from_node` via `cheapest.prev_edge`.
    inline void update_cheapest(RTExploredNode& cheapest, size_t thread_idx) {
        const RRNodeId& inode = cheapest.index;
        add_to_mod_list(inode, thread_idx);
        rr_node_route_inf_[inode].prev_edge = cheapest.prev_edge;
        rr_node_route_inf_[inode].path_cost = cheapest.total_cost;
        rr_node_route_inf_[inode].backward_path_cost = cheapest.backward_path_cost;
    }

    inline void obtainSpinLock(const RRNodeId& inode) {
        locks_[size_t(inode)].acquire();
    }

    inline void releaseLock(const RRNodeId& inode) {
        locks_[size_t(inode)].release();
    }

    /** Common logic from timing_driven_route_connection_from_route_tree and
     * timing_driven_route_connection_from_route_tree_high_fanout for running
     * the connection router.
     * @param[in] rt_root RouteTreeNode describing the current routing state
     * @param[in] sink_node Sink node ID to route to
     * @param[in] cost_params
     * @param[in] bounding_box Keep search confined to this bounding box
     * @return bool Signal to retry this connection with a full-device bounding box */
    bool timing_driven_route_connection_common_setup(
        const RouteTreeNode& rt_root,
        RRNodeId sink_node,
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box);

    // Finds a path to sink_node, starting from the elements currently in the
    // heap.
    //
    // If the path is not found, which means that the path_cost of sink_node in
    // RR node route info has never been updated, `rr_node_route_inf_[sink_node]
    // .path_cost` will be the initial value (i.e., float infinity). This case
    // can be detected by `std::isinf(rr_node_route_inf_[sink_node].path_cost)`.
    //
    // This is the core maze routing routine.
    //
    // Note: For understanding the connection router, start here.
    void timing_driven_route_connection_from_heap(
        RRNodeId sink_node,
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box);

    void timing_driven_route_connection_from_heap_sub_thread_wrapper(
        const size_t thread_idx);

    void timing_driven_route_connection_from_heap_thread_func(
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

    // Calculates the cost of reaching to_node
    void evaluate_timing_driven_node_costs(
        RTExploredNode* to,
        const t_conn_cost_params& cost_params,
        RRNodeId from_node,
        RRNodeId target_node);

    // Find paths from current heap to all nodes in the RR graph
    vtr::vector<RRNodeId, RTExploredNode> timing_driven_find_all_shortest_paths_from_heap(
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box);

    //Adds the route tree rooted at rt_node to the heap, preparing it to be
    //used as branch-points for further routing.
    void add_route_tree_to_heap(const RouteTreeNode& rt_node,
                                RRNodeId target_node,
                                const t_conn_cost_params& cost_params,
                                const t_bb& net_bb);

    //Unconditionally adds rt_node to the heap
    //
    //Note that if you want to respect rt_node->re_expand that is the caller's
    //responsibility.
    void add_route_tree_node_to_heap(
        const RouteTreeNode& rt_node,
        RRNodeId target_node,
        const t_conn_cost_params& cost_params,
        const t_bb& net_bb);

    t_bb add_high_fanout_route_tree_to_heap(
        const RouteTreeNode& rt_root,
        RRNodeId target_node,
        const t_conn_cost_params& cost_params,
        const SpatialRouteTreeLookup& spatial_route_tree_lookup,
        const t_bb& net_bounding_box);

    const DeviceGrid& grid_;
    const RouterLookahead& router_lookahead_;
    const t_rr_graph_view rr_nodes_;
    const RRGraphView* rr_graph_;
    vtr::array_view<const t_rr_rc_data> rr_rc_data_;
    vtr::array_view<const t_rr_switch_inf> rr_switch_inf_;
    const vtr::vector<ParentNetId, std::vector<std::vector<int>>>& net_terminal_groups;
    const vtr::vector<ParentNetId, std::vector<int>>& net_terminal_group_num;
    vtr::vector<RRNodeId, t_rr_node_route_inf>& rr_node_route_inf_;
    bool is_flat_;
    std::vector<std::vector<RRNodeId>> modified_rr_node_inf_;
    RouterStats* router_stats_;
    const ConnectionParameters* conn_params_;
    MultiQueueDAryHeap<HeapImplementation::arg_D> heap_;
    std::vector<std::thread> sub_threads_;
    barrier_t thread_barrier_;
    std::atomic<bool> is_router_destroying_;
    std::vector<spin_lock_t> locks_;
    bool multi_queue_direct_draining_;

    bool router_debug_;

    bool only_opin_inter_layer;

    std::atomic<RRNodeId*> sink_node_;
    std::atomic<t_conn_cost_params*> cost_params_;
    std::atomic<t_bb*> bounding_box_;
    std::atomic<t_bb*> target_bb_;

    // Cumulative time spent in the path search part of the connection router.
    std::chrono::microseconds path_search_cumulative_time;

    // The path manager for RCV, keeps track of the route tree as a set, also
    // manages the allocation of `rcv_path_data`.
    PathManager rcv_path_manager;
    vtr::vector<RRNodeId, t_heap_path*> rcv_path_data;
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
