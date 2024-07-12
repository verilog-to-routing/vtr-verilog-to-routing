#ifndef _PARALLEL_CONNECTION_ROUTER_H
#define _PARALLEL_CONNECTION_ROUTER_H

#include <chrono>
#include "connection_router_interface.h"
#include "rr_graph_storage.h"
#include "route_common.h"
#include "router_lookahead.h"
#include "route_tree.h"
#include "rr_rc_data.h"
#include "router_stats.h"
#include "spatial_route_tree_lookup.h"

// #define PROFILE_HEAP_OCCUPANCY
#ifdef PROFILE_HEAP_OCCUPANCY
#include <fstream>
#endif

#define VPR_PARALLEL_CONNECTION_ROUTER_USE_MULTI_QUEUE
// #define VPR_PARALLEL_CONNECTION_ROUTER_USE_ONE_TBB

#if defined(VPR_PARALLEL_CONNECTION_ROUTER_USE_MULTI_QUEUE)
#include "multi_queue_priority_queue.h"
using ParallelPriorityQueue = MultiQueuePriorityQueue;
#elif defined(VPR_PARALLEL_CONNECTION_ROUTER_USE_ONE_TBB)
#include "onetbb_priority_queue.h"
using ParallelPriorityQueue = OneTBBConcurrentPriorityQueue;
#else
#error Please define VPR_PARALLEL_CONNECTION_ROUTER_USE_MULTI_QUEUE or \
       VPR_PARALLEL_CONNECTION_ROUTER_USE_ONE_TBB
#endif

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

// `node_t` is a simplified version of `t_heap`, and is used as a bundle of node
// information in the functions inside the routing loop.
struct node_t {
    float total_cost;
    float backward_path_cost;
    float R_upstream;
    RREdgeId prev_edge;
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

// Prune the heap when it contains 4x the number of nodes in the RR graph.
// constexpr size_t kHeapPruneFactor = 4;

// This class encapsolates the timing driven connection router. This class
// routes from some initial set of sources (via the input rt tree) to a
// particular sink.
//
// When the ConnectionRouter is used, it mutates the provided
// rr_node_route_inf.  The routed path can be found by tracing from the sink
// node (which is returned) through the rr_node_route_inf.  See
// update_traceback as an example of this tracing.
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
        bool multi_queue_direct_draining,
        const std::vector<int>& thread_affinity)
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
        , router_debug_(false)
        , multi_queue_direct_draining_(multi_queue_direct_draining) {
        heap_.init_heap(grid);
        only_opin_inter_layer = (grid.get_num_layers() > 1) && inter_layer_connections_limited_to_opin(*rr_graph);
        sub_threads_.resize(multi_queue_num_threads - 1);
        thread_barrier_.init();

#ifdef PROFILE_HEAP_OCCUPANCY
        heap_occ_profile_.open("occupancy.txt", std::ios::trunc);
#endif

        bool enable_thread_affinity = thread_affinity.size() > 0;
        VTR_ASSERT((!enable_thread_affinity) || (static_cast<int>(thread_affinity.size()) == multi_queue_num_threads));

        for (int i = 0 ; i < multi_queue_num_threads - 1; ++i) {
            sub_threads_[i] = std::thread(&ParallelConnectionRouter::timing_driven_route_connection_from_heap_sub_thread_wrapper, this, i + 1 /*0: main thread*/);
            // Create a cpu_set_t object representing a set of CPUs. Clear it and mark only CPU i as set.
            if (enable_thread_affinity) {
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET(thread_affinity[i + 1], &cpuset);
                int rc = pthread_setaffinity_np(sub_threads_[i].native_handle(),
                                                sizeof(cpu_set_t), &cpuset);
                if (rc != 0) {
                    VTR_LOG("Error calling pthread_setaffinity_np: %d\n", rc);
                }
            }
            sub_threads_[i].detach();
        }

        if (enable_thread_affinity) {
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(thread_affinity[0], &cpuset);
            int rc = pthread_setaffinity_np(pthread_self(),
                                            sizeof(cpu_set_t), &cpuset);
            if (rc != 0) {
                VTR_LOG("Error calling pthread_setaffinity_np: %d\n", rc);
            }
        }
    }

    ~ParallelConnectionRouter() {
        is_router_destroying_ = true;
        thread_barrier_.wait();

        VTR_LOG("Parallel Connection Router is being destroyed. Time spent computing SSSP: %g seconds\n.", this->sssp_total_time.count() / 1000000.0);

#ifdef PROFILE_HEAP_OCCUPANCY
        heap_occ_profile_.close();
#endif
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
     * t_heap: heap element of cheapest path */
    std::tuple<bool, bool, t_heap> timing_driven_route_connection_from_route_tree(
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
     * t_heap: heap element of cheapest path */
    std::tuple<bool, bool, t_heap> timing_driven_route_connection_from_route_tree_high_fanout(
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
    vtr::vector<RRNodeId, t_heap> timing_driven_find_all_shortest_paths_from_route_tree(
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

    // Update the route path to the node pointed to by cheapest.
    inline void update_cheapest(const node_t& cheapest, RRNodeId inode, size_t thread_idx) {
        update_cheapest(cheapest, inode, &rr_node_route_inf_[inode], thread_idx);
    }

    inline void update_cheapest(const node_t& cheapest, RRNodeId inode, t_rr_node_route_inf* route_inf, size_t thread_idx) {
        //Record final link to target
        add_to_mod_list(inode, thread_idx);

        route_inf->prev_edge = cheapest.prev_edge;
        route_inf->path_cost = cheapest.total_cost;
        route_inf->backward_path_cost = cheapest.backward_path_cost;
    }

    inline void obtainSpinLock(RRNodeId inode) {
        locks_[size_t(inode)].acquire();
    }

    inline void releaseLock(RRNodeId inode) {
        locks_[size_t(inode)].release();
    }

    /** Common logic from timing_driven_route_connection_from_route_tree and
     * timing_driven_route_connection_from_route_tree_high_fanout for running
     * the connection router.
     * @param[in] rt_root RouteTreeNode describing the current routing state
     * @param[in] sink_node Sink node ID to route to
     * @param[in] cost_params
     * @param[in] bounding_box Keep search confined to this bounding box
     * @return bool Signal to retry this connection with a full-device bounding box,
     * @return t_heap* Heap element describing the path found. */
    bool timing_driven_route_connection_common_setup(
        const RouteTreeNode& rt_root,
        RRNodeId sink_node,
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box);

    // Finds a path to sink_node, starting from the elements currently in the
    // heap.
    //
    // This is the core maze routing routine.
    //
    // Note: For understanding the connection router, start here.
    //
    // Returns either the last element of the path, or nullptr if no path is
    // found
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
        const size_t thread_idx);

    // Expand each neighbor of the current node.
    void timing_driven_expand_neighbours(
        const node_t& current,
        RRNodeId from_node,
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box,
        RRNodeId target_node,
        size_t thread_idx);

    // Conditionally adds to_node to the router heap (via path from from_node
    // via from_edge).
    //
    // RR nodes outside bounding box specified in bounding_box are not added
    // to the heap.
    void timing_driven_expand_neighbour(
        const node_t& current,
        RRNodeId from_node,
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
        const node_t& current,
        RRNodeId from_node,
        RRNodeId to_node,
        RREdgeId from_edge,
        RRNodeId target_node,
        size_t thread_idx);

    // Calculates the cost of reaching to_node
    void evaluate_timing_driven_node_costs(
        node_t* to,
        const t_conn_cost_params& cost_params,
        RRNodeId from_node,
        RRNodeId to_node,
        RREdgeId from_edge,
        RRNodeId target_node);

    void empty_heap_annotating_node_route_inf();

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
    ParallelPriorityQueue heap_;
    std::vector<std::thread> sub_threads_;
    barrier_t thread_barrier_;
    std::atomic<bool> is_router_destroying_;
    std::vector<spin_lock_t> locks_;

    bool router_debug_;

    bool only_opin_inter_layer;

    std::atomic<RRNodeId*> sink_node_;
    std::atomic<t_conn_cost_params*> cost_params_;
    std::atomic<t_bb*> bounding_box_;

    // Timing
    std::chrono::microseconds sssp_total_time{0};

    bool multi_queue_direct_draining_;

    // Profiling
#ifdef PROFILE_HEAP_OCCUPANCY
    std::ofstream heap_occ_profile_;
#endif
};

#endif /* _PARALLEL_CONNECTION_ROUTER_H */
