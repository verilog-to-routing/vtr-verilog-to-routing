#pragma once

#include <algorithm>

#include "globals.h"
#include "router_lookahead_map_utils.h"
#include "rr_graph_fwd.h"
#include "vpr_types.h"

namespace util {

/** @brief Expands eligible neighbours of a Dijkstra queue entry within the bounding box. */
void expand_dijkstra_neighbours(util::PQ_Entry parent_entry,
                                vtr::vector<RRNodeId, float>& node_visited_costs,
                                vtr::vector<RRNodeId, bool>& node_expanded,
                                std::priority_queue<util::PQ_Entry>& pq,
                                const t_bb& bb);

} // namespace util

/**
 * @brief runs Dijkstra's algorithm from specified node until all nodes in bb
 * have been visited. Each time an IPIN is visited, the delay/congestion
 * information from start_node to that pin is passed on to find_ipin_callback,
 * which is passed on by lookaheads. Typically a lookahead would then save this
 * information in an internal table/map.
 */
template<typename FindIpinCallback>
void run_dijkstra(RRNodeId start_node,
                  util::t_dijkstra_data& data,
                  const t_bb& bb,
                  FindIpinCallback find_ipin_callback) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;

    // TODO: Might not need 'rr_graph.num_nodes()' elements for cases with
    // a smaller bounding box. Investigate possible fixes.
    vtr::vector<RRNodeId, bool>& node_expanded = data.node_expanded;
    node_expanded.resize(rr_graph.num_nodes());
    std::fill(node_expanded.begin(), node_expanded.end(), false);

    vtr::vector<RRNodeId, float>& node_visited_costs = data.node_visited_costs;
    node_visited_costs.resize(rr_graph.num_nodes());
    std::fill(node_visited_costs.begin(), node_visited_costs.end(), -1.0);

    // A priority queue for expansion
    std::priority_queue<util::PQ_Entry>& pq = data.pq;

    // Clear priority queue if non-empty
    while (!pq.empty()) {
        pq.pop();
    }

    // First entry has no upstream delay or congestion
    pq.emplace(start_node, UNDEFINED, 0, 0, 0, true);

    // Now do routing
    while (!pq.empty()) {
        util::PQ_Entry current = pq.top();
        pq.pop();

        RRNodeId curr_node = current.rr_node;

        // Check that we haven't already expanded from this node
        if (node_expanded[curr_node]) {
            continue;
        }

        // If this node is an ipin, pass it to find_ipin_callback. Typically `find_ipin_callback` will store
        // the congestion/delay data in an internal map/table.
        if (rr_graph.node_type(curr_node) == e_rr_type::IPIN) {
            VTR_ASSERT_SAFE(rr_graph.node_xlow(curr_node) == rr_graph.node_xhigh(curr_node));
            VTR_ASSERT_SAFE(rr_graph.node_ylow(curr_node) == rr_graph.node_yhigh(curr_node));

            find_ipin_callback(current);
        }

        util::expand_dijkstra_neighbours(current, node_visited_costs, node_expanded, pq, bb);
        node_expanded[curr_node] = true;
    }
}
