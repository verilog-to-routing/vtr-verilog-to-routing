#ifndef _PARALLEL_CONNECTION_ROUTER_H
#define _PARALLEL_CONNECTION_ROUTER_H

#include "connection_router.h"

#include "d_ary_heap.h"
#include "multi_queue_d_ary_heap.h"

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

/**
 * @brief Spin lock implementation using std::atomic_flag
 *
 * It is used per RR node for protecting the update to node costs
 * to prevent data races. Since different threads rarely work on
 * the same node simultaneously, this fine-grained locking strategy
 * of one lock per node reduces contention.
 */
class spin_lock_t {
    /** Atomic flag used for the lock implementation */
    std::atomic_flag lock_ = ATOMIC_FLAG_INIT;

  public:
    /**
     * @brief Acquires the spin lock, repeatedly attempting until successful
     */
    void acquire() {
        while (std::atomic_flag_test_and_set_explicit(&lock_, std::memory_order_acquire))
            ;
    }

    /**
     * @brief Releases the spin lock, allowing other threads to acquire it
     */
    void release() {
        std::atomic_flag_clear_explicit(&lock_, std::memory_order_release);
    }
};

/**
 * @brief Thread barrier implementation using std::mutex
 *
 * It ensures all participating threads reach a synchronization point
 * before any are allowed to proceed further. It uses a mutex and
 * condition variable to coordinate thread synchronization.
 */
class barrier_mutex_t {
    // FIXME: Try std::barrier (since C++20) to replace this mutex barrier
    std::mutex mutex_;
    std::condition_variable cv_;
    size_t count_;
    size_t max_count_;
    size_t generation_ = 0;

  public:
    /**
     * @brief Constructs a barrier for a specific number of threads
     * @param num_threads Number of threads that must call wait() before
     * any thread is allowed to proceed
     */
    explicit barrier_mutex_t(size_t num_threads)
        : count_(num_threads)
        , max_count_(num_threads) {}

    /**
     * @brief Blocks the calling thread until all threads have called wait()
     *
     * When the specified number of threads have called this method, all
     * threads are unblocked and the barrier is reset for the next use.
     */
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

/**
 * @brief Spin-based thread barrier implementation using std::atomic
 *
 * It ensures all participating threads reach a synchronization point
 * before any are allowed to proceed further. It uses atomic operations
 * to implement Sense-Reversing Centralized Barrier (from Section 5.2.1
 * of Michael L. Scott's textbook) without using mutex locks.
 */
class barrier_spin_t {
    /** Number of threads that must reach the barrier */
    size_t num_threads_ = 1;

    /** Atomic counter tracking the number of threads that have arrived at the barrier */
    std::atomic<size_t> count_ = 0;

    /** Global sense shared by all participating threads */
    std::atomic<bool> sense_ = false;

    /** Thread-local sense value for each participating thread */
    inline static thread_local bool local_sense_ = false;

  public:
    /**
     * @brief Constructs a barrier for a specific number of threads
     * @param num_threads Number of threads that must call wait() before
     * any thread is allowed to proceed
     */
    explicit barrier_spin_t(size_t num_threads) { num_threads_ = num_threads; }

    /**
     * @brief Initializes the thread-local sense flag
     * @note Should be called by each thread before first using the barrier.
     */
    void init() {
        local_sense_ = false;
    }

    /**
     * @brief Blocks the calling thread until all threads have called wait()
     *
     * Uses a sense-reversing algorithm to synchronize threads. The last thread
     * to arrive unblocks all waiting threads. This method avoids using locks or
     * condition variables, making it potentially more efficient for short waits.
     */
    void wait() {
        bool s = !local_sense_;
        local_sense_ = s;
        size_t num_arrivals = count_.fetch_add(1) + 1;
        if (num_arrivals == num_threads_) {
            count_.store(0);
            sense_.store(s);
        } else {
            while (sense_.load() != s)
                ; // spin until the last thread arrives
        }
    }
};

using barrier_t = barrier_spin_t; // Using the spin-based thread barrier

/**
 * @class ParallelConnectionRouter implements the MultiQueue-based parallel connection
 * router (FPT'24) based on the ConnectionRouter interface.
 * @details The details of the algorithm can be found from the conference paper:
 * A. Singer, H. Yan, G. Zhang, M. Jeffrey, M. Stojilovic and V. Betz, "MultiQueue-Based FPGA Routing:
 * Relaxed A* Priority Ordering for Improved Parallelism," Int. Conf. on Field-Programmable Technology,
 * Dec. 2024.
 */
template<typename HeapImplementation>
class ParallelConnectionRouter : public ConnectionRouter<MultiQueueDAryHeap<HeapImplementation::arg_D>> {
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
        : ConnectionRouter<MultiQueueDAryHeap<HeapImplementation::arg_D>>(grid, router_lookahead, rr_nodes, rr_graph, rr_rc_data, rr_switch_inf, rr_node_route_inf, is_flat)
        , modified_rr_node_inf_(multi_queue_num_threads)
        , thread_barrier_(multi_queue_num_threads)
        , is_router_destroying_(false)
        , locks_(rr_node_route_inf.size())
        , multi_queue_direct_draining_(multi_queue_direct_draining) {
        // Set the MultiQueue parameters
        this->heap_.set_num_threads_and_queues(multi_queue_num_threads, multi_queue_num_queues);
        // Initialize the thread barrier
        this->thread_barrier_.init();
        // Instantiate (multi_queue_num_threads - 1) helper threads
        this->sub_threads_.resize(multi_queue_num_threads - 1);
        for (int i = 0; i < multi_queue_num_threads - 1; ++i) {
            this->sub_threads_[i] = std::thread(&ParallelConnectionRouter::timing_driven_find_single_shortest_path_from_heap_sub_thread_wrapper, this, i + 1 /*0: main thread*/);
            this->sub_threads_[i].detach();
        }
    }

    ~ParallelConnectionRouter() {
        this->is_router_destroying_ = true; // signal the helper threads to exit
        this->thread_barrier_.wait();       // wait until all threads reach the barrier

        VTR_LOG("Parallel Connection Router is being destroyed. Time spent on path search: %.3f seconds.\n",
                std::chrono::duration<float /*convert to seconds by default*/>(this->path_search_cumulative_time).count());
    }

    /**
     * @brief Clears the modified list per thread
     * @note Should be called after reset_path_costs have been called
     */
    void clear_modified_rr_node_info() final {
        for (auto& thread_visited_rr_nodes : this->modified_rr_node_inf_) {
            thread_visited_rr_nodes.clear();
        }
    }

    /**
     * @brief Resets modified data in rr_node_route_inf based on modified_rr_node_inf
     */
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

    /**
     * @brief [Not supported] Enables RCV feature
     * @note RCV for parallel connection router has not been implemented yet.
     * Thus this function is not expected to be called.
     */
    void set_rcv_enabled(bool) final {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "RCV for parallel connection router not yet implemented. Not expected to be called.");
    }

    /**
     * @brief [Not supported] Finds shortest paths from the route tree rooted at rt_root to all sinks available
     * @note This function has not been implemented yet and is not the focus of parallel connection router.
     * Thus this function is not expected to be called.
     */
    vtr::vector<RRNodeId, RTExploredNode> timing_driven_find_all_shortest_paths_from_route_tree(
        const RouteTreeNode&,
        const t_conn_cost_params&,
        const t_bb&,
        RouterStats&,
        const ConnectionParameters&) final {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "timing_driven_find_all_shortest_paths_from_route_tree not yet implemented (nor is the focus of the parallel connection router). Not expected to be called.");
    }

  protected:
    /**
     * @brief Marks that data associated with rr_node 'inode' has
     * been modified, and needs to be reset in reset_path_costs
     */
    inline void add_to_mod_list(RRNodeId inode, size_t thread_idx) {
        if (std::isinf(this->rr_node_route_inf_[inode].path_cost)) {
            this->modified_rr_node_inf_[thread_idx].push_back(inode);
        }
    }

    /**
     * @brief Updates the route path to the node `cheapest.index`
     * via the path from `from_node` via `cheapest.prev_edge`
     */
    inline void update_cheapest(RTExploredNode& cheapest, size_t thread_idx) {
        const RRNodeId& inode = cheapest.index;
        add_to_mod_list(inode, thread_idx);
        this->rr_node_route_inf_[inode].prev_edge = cheapest.prev_edge;
        this->rr_node_route_inf_[inode].path_cost = cheapest.total_cost;
        this->rr_node_route_inf_[inode].backward_path_cost = cheapest.backward_path_cost;
    }

    /**
     * @brief Obtains the per-node spin locks for protecting node cost updates
     */
    inline void obtainSpinLock(const RRNodeId& inode) {
        this->locks_[size_t(inode)].acquire();
    }

    /**
     * @brief Releases the per-node spin lock, allowing other
     * threads working on the same node to obtain it
     */
    inline void releaseLock(const RRNodeId& inode) {
        this->locks_[size_t(inode)].release();
    }

    /**
     * @brief Finds the single shortest path from current heap to the sink node in the RR graph
     * @param sink_node Sink node ID to route to
     * @param cost_params Cost function parameters
     * @param bounding_box Keep search confined to this bounding box
     * @param target_bb Prune IPINs that lead to blocks other than the target block
     */
    void timing_driven_find_single_shortest_path_from_heap(RRNodeId sink_node,
                                                           const t_conn_cost_params& cost_params,
                                                           const t_bb& bounding_box,
                                                           const t_bb& target_bb) final;

    /**
     * @brief Helper thread wrapper function, passed to std::thread instantiation and running a
     * while-loop to obtain and execute new helper thread tasks until the main thread signals the
     * threads to exit
     * @param thread_idx Thread ID (0 means main thread; 1 to #threads-1 means helper threads)
     */
    void timing_driven_find_single_shortest_path_from_heap_sub_thread_wrapper(
        const size_t thread_idx);

    /**
     * @brief Helper thread task function to find the single shortest path from current heap to
     * the sink node in the RR graph
     * @param sink_node Sink node ID to route to
     * @param cost_params Cost function parameters
     * @param bounding_box Keep search confined to this bounding box
     * @param target_bb Prune IPINs that lead to blocks other than the target block
     * @param thread_idx Thread ID (0 means main thread; 1 to #threads-1 means helper threads)
     */
    void timing_driven_find_single_shortest_path_from_heap_thread_func(
        RRNodeId sink_node,
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box,
        const t_bb& target_bb,
        const size_t thread_idx);

    /**
     * @brief Expands each neighbor of the current node in the wave expansion
     * @param current Current node being explored
     * @param cost_params Cost function parameters
     * @param bounding_box Keep search confined to this bounding box
     * @param target_node Target node ID to route to
     * @param target_bb Prune IPINs that lead to blocks other than the target block
     * @param thread_idx Thread ID (0 means main thread; 1 to #threads-1 means helper threads)
     */
    void timing_driven_expand_neighbours(
        const RTExploredNode& current,
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box,
        RRNodeId target_node,
        const t_bb& target_bb,
        size_t thread_idx);

    /**
     * @brief Conditionally adds to_node to the router heap (via path from current.index via from_edge)
     * @note RR nodes outside bounding box specified in bounding_box are not added to the heap.
     * @param current Current node being explored
     * @param from_edge Edge between the current node and the neighbor node
     * @param to_node Neighbor node to be expanded
     * @param cost_params Cost function parameters
     * @param bounding_box Keep search confined to this bounding box
     * @param target_node Target node ID to route to
     * @param target_bb Prune IPINs that lead to blocks other than the target block
     * @param thread_idx Thread ID (0 means main thread; 1 to #threads-1 means helper threads)
     */
    void timing_driven_expand_neighbour(
        const RTExploredNode& current,
        RREdgeId from_edge,
        RRNodeId to_node,
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box,
        RRNodeId target_node,
        const t_bb& target_bb,
        size_t thread_idx);

    /**
     * @brief Adds to_node to the heap, and also adds any nodes which are connected by non-configurable edges
     * @param cost_params Cost function parameters
     * @param current Current node being explored
     * @param to_node Neighbor node to be expanded
     * @param from_edge Edge between the current node and the neighbor node
     * @param target_node Target node ID to route to
     * @param thread_idx Thread ID (0 means main thread; 1 to #threads-1 means helper threads)
     */
    void timing_driven_add_to_heap(
        const t_conn_cost_params& cost_params,
        const RTExploredNode& current,
        RRNodeId to_node,
        RREdgeId from_edge,
        RRNodeId target_node,
        size_t thread_idx);

    /**
     * @brief Unconditionally adds rt_node to the heap
     * @note If you want to respect rt_node->re_expand that is the caller's responsibility.
     * @todo Consider moving this function into the ConnectionRouter class after checking
     * the different prune functions of the serial and parallel connection routers.
     * @param rt_node RouteTreeNode to be added to the heap
     * @param target_node Target node ID to route to
     * @param cost_params Cost function parameters
     * @param net_bb Do not push to heap if not in bounding box
     */
    void add_route_tree_node_to_heap(
        const RouteTreeNode& rt_node,
        RRNodeId target_node,
        const t_conn_cost_params& cost_params,
        const t_bb& net_bb) final;

    /**
     * @brief [Not supported] Finds shortest paths from current heap to all nodes in the RR graph
     * @note This function has not been implemented yet and is not the focus of parallel connection router.
     * Thus this function is not expected to be called.
     */
    vtr::vector<RRNodeId, RTExploredNode> timing_driven_find_all_shortest_paths_from_heap(
        const t_conn_cost_params&,
        const t_bb&) final {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "timing_driven_find_all_shortest_paths_from_heap not yet implemented (nor is the focus of this project). Not expected to be called.");
    }

    /** Node IDs of modified nodes in rr_node_route_inf for each thread*/
    std::vector<std::vector<RRNodeId>> modified_rr_node_inf_;

    /** Helper threads */
    std::vector<std::thread> sub_threads_;

    /** Thread barrier for synchronization */
    barrier_t thread_barrier_;

    /** Signal for helper threads to exit */
    std::atomic<bool> is_router_destroying_;

    /** Fine-grained locks per RR node */
    std::vector<spin_lock_t> locks_;

    /** Is queue draining optimization enabled? */
    bool multi_queue_direct_draining_;

    //@{
    /** Atomic parameters of thread task functions to pass from main thread to helper threads */
    std::atomic<RRNodeId*> sink_node_;
    std::atomic<t_conn_cost_params*> cost_params_;
    std::atomic<t_bb*> bounding_box_;
    std::atomic<t_bb*> target_bb_;
    //@}
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
