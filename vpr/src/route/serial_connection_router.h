#ifndef _SERIAL_CONNECTION_ROUTER_H
#define _SERIAL_CONNECTION_ROUTER_H

#include "connection_router.h"

#include "d_ary_heap.h"

/**
 * @class SerialConnectionRouter implements the AIR's serial timing-driven connection router
 * @details This class routes from some initial set of sources (via the input rt tree) to a
 * particular sink using single thread.
 */
template<typename HeapImplementation>
class SerialConnectionRouter : public ConnectionRouter<HeapImplementation> {
  public:
    SerialConnectionRouter(
        const DeviceGrid& grid,
        const RouterLookahead& router_lookahead,
        const t_rr_graph_storage& rr_nodes,
        const RRGraphView* rr_graph,
        const std::vector<t_rr_rc_data>& rr_rc_data,
        const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switch_inf,
        vtr::vector<RRNodeId, t_rr_node_route_inf>& rr_node_route_inf,
        bool is_flat)
        : ConnectionRouter<HeapImplementation>(grid, router_lookahead, rr_nodes, rr_graph, rr_rc_data, rr_switch_inf, rr_node_route_inf, is_flat) {
    }

    ~SerialConnectionRouter() {
        VTR_LOG("Serial Connection Router is being destroyed. Time spent on path search: %.3f seconds.\n",
                std::chrono::duration<float /*convert to seconds by default*/>(this->path_search_cumulative_time).count());
    }

    /**
     * @brief Clears the modified list per thread
     * @note Should be called after reset_path_costs have been called
     */
    void clear_modified_rr_node_info() final {
        this->modified_rr_node_inf_.clear();
    }

    /**
     * @brief Resets modified data in rr_node_route_inf based on modified_rr_node_inf
     */
    void reset_path_costs() final {
        // Reset the node info stored in rr_node_route_inf variable
        ::reset_path_costs(this->modified_rr_node_inf_);
        // Reset the node (RCV-related) info stored inside the connection router
        if (this->rcv_path_manager.is_enabled()) {
            for (const auto& node : this->modified_rr_node_inf_) {
                this->rcv_path_data[node] = nullptr;
            }
        }
    }

    /**
     * @brief Enables or disables RCV in connection router
     * @note Enabling this will utilize extra path structures, as well as
     * the RCV cost function. Ensure route budgets have been calculated
     * before enabling this.
     * @param enable Whether enabling RCV or not
     */
    void set_rcv_enabled(bool enable) final {
        this->rcv_path_manager.set_enabled(enable);
        if (enable) {
            this->rcv_path_data.resize(this->rr_node_route_inf_.size());
        }
    }

    /**
     * @brief Finds shortest paths from the route tree rooted at rt_root to all sinks available
     * @note Unlike timing_driven_route_connection_from_route_tree(), only part of the route tree which
     * is spatially close to the sink is added to the heap.
     * @note If cost_params.astar_fac is set to 0, this effectively becomes Dijkstra's algorithm with a
     * modified exit condition (runs until heap is empty). When using cost_params.astar_fac = 0, for
     * efficiency the RouterLookahead used should be the NoOpLookahead.
     * @note This routine is currently used only to generate information that may be helpful in debugging
     * an architecture.
     * @param rt_root RouteTreeNode describing the current routing state
     * @param cost_params Cost function parameters
     * @param bounding_box Keep search confined to this bounding box
     * @param router_stats Update router statistics
     * @param conn_params Parameters to guide the routing of the given connection
     * @return A vector where each element is a reachable sink
     */
    vtr::vector<RRNodeId, RTExploredNode> timing_driven_find_all_shortest_paths_from_route_tree(
        const RouteTreeNode& rt_root,
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box,
        RouterStats& router_stats,
        const ConnectionParameters& conn_params) final;

  protected:
    /**
     * @brief Marks that data associated with rr_node 'inode' has
     * been modified, and needs to be reset in reset_path_costs
     */
    inline void add_to_mod_list(RRNodeId inode) {
        if (std::isinf(this->rr_node_route_inf_[inode].path_cost)) {
            this->modified_rr_node_inf_.push_back(inode);
        }
    }

    /**
     * @brief Updates the route path to the node `cheapest.index`
     * via the path from `from_node` via `cheapest.prev_edge`
     */
    inline void update_cheapest(RTExploredNode& cheapest, const RRNodeId& from_node) {
        const RRNodeId& inode = cheapest.index;
        add_to_mod_list(inode);
        this->rr_node_route_inf_[inode].prev_edge = cheapest.prev_edge;
        this->rr_node_route_inf_[inode].path_cost = cheapest.total_cost;
        this->rr_node_route_inf_[inode].backward_path_cost = cheapest.backward_path_cost;

        // Use the already created next path structure pointer when RCV is enabled
        if (this->rcv_path_manager.is_enabled()) {
            this->rcv_path_manager.move(this->rcv_path_data[inode], cheapest.path_data);

            this->rcv_path_data[inode]->path_rr = this->rcv_path_data[from_node]->path_rr;
            this->rcv_path_data[inode]->edge = this->rcv_path_data[from_node]->edge;
            this->rcv_path_data[inode]->path_rr.push_back(from_node);
            this->rcv_path_data[inode]->edge.push_back(cheapest.prev_edge);
        }
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
     * @brief Expands this current node if it is a cheaper path
     * @param from_node Current node ID being explored
     * @param new_total_cost Identifier popped from the heap to detect if the element (pair)
     * (from_node, new_total_cost) was the most recently pushed element for from_node
     * @param target_node Target node ID to route to
     * @param cost_params Cost function parameters
     * @param bounding_box Keep search confined to this bounding box
     * @param target_bb Prune IPINs that lead to blocks other than the target block
     */
    void timing_driven_expand_cheapest(
        RRNodeId from_node,
        float new_total_cost,
        RRNodeId target_node,
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box,
        const t_bb& target_bb);

    /**
     * @brief Expands each neighbor of the current node in the wave expansion
     * @param current Current node being explored
     * @param cost_params Cost function parameters
     * @param bounding_box Keep search confined to this bounding box
     * @param target_node Target node ID to route to
     * @param target_bb Prune IPINs that lead to blocks other than the target block
     */
    void timing_driven_expand_neighbours(
        const RTExploredNode& current,
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box,
        RRNodeId target_node,
        const t_bb& target_bb);

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
     */
    void timing_driven_expand_neighbour(
        const RTExploredNode& current,
        RREdgeId from_edge,
        RRNodeId to_node,
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box,
        RRNodeId target_node,
        const t_bb& target_bb);

    /**
     * @brief Adds to_node to the heap, and also adds any nodes which are connected by non-configurable edges
     * @param cost_params Cost function parameters
     * @param current Current node being explored
     * @param to_node Neighbor node to be expanded
     * @param from_edge Edge between the current node and the neighbor node
     * @param target_node Target node ID to route to
     */
    void timing_driven_add_to_heap(
        const t_conn_cost_params& cost_params,
        const RTExploredNode& current,
        RRNodeId to_node,
        RREdgeId from_edge,
        RRNodeId target_node);

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
     * @brief Finds shortest paths from current heap to all nodes in the RR graph
     *
     * Since there is no single *target* node this uses Dijkstra's algorithm with
     * a modified exit condition (runs until heap is empty).
     *
     * @param cost_params Cost function parameters
     * @param bounding_box Keep search confined to this bounding box
     * @return A vector where each element contains the shortest route to a specific sink node
     */
    vtr::vector<RRNodeId, RTExploredNode> timing_driven_find_all_shortest_paths_from_heap(
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box) final;
};

/** Construct a serial connection router that uses the specified heap type.
 * This function is not used, but removing it will result in "undefined reference"
 * errors since heap type specializations won't get emitted from serial_connection_router.cpp
 * without it.
 * The alternative is moving all SerialConnectionRouter fn implementations into the header. */
std::unique_ptr<ConnectionRouterInterface> make_serial_connection_router(
    e_heap_type heap_type,
    const DeviceGrid& grid,
    const RouterLookahead& router_lookahead,
    const t_rr_graph_storage& rr_nodes,
    const RRGraphView* rr_graph,
    const std::vector<t_rr_rc_data>& rr_rc_data,
    const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switch_inf,
    vtr::vector<RRNodeId, t_rr_node_route_inf>& rr_node_route_inf,
    bool is_flat);

#endif /* _SERIAL_CONNECTION_ROUTER_H */
