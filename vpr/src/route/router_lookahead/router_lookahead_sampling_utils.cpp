#include "router_lookahead_sampling_utils.h"
#include <functional>
#include "globals.h"
#include "vpr_context.h"
#include "vpr_utils.h"

static void expand_dijkstra_neighbours(util::PQ_Entry parent_entry,
                                       vtr::vector<RRNodeId, float>& node_visited_costs,
                                       vtr::vector<RRNodeId, bool>& node_expanded,
                                       std::priority_queue<util::PQ_Entry>& pq,
                                       const t_bb& bb);

void run_dijkstra(RRNodeId start_node,
                  util::t_dijkstra_data& data,
                  const t_bb& bb,
                  std::function<void(util::PQ_Entry)> found_sink_callback) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

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

        //VTR_LOG("Expanding with delay=%10.3g cong=%10.3g (%s)\n", current.delay, current.congestion_upstream, describe_rr_node(rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, curr_node).c_str());

        // If this node is an ipin record its congestion/delay in the routing_cost_map
        if (rr_graph.node_type(curr_node) == e_rr_type::IPIN) {
            VTR_ASSERT_SAFE(rr_graph.node_xlow(curr_node) == rr_graph.node_xhigh(curr_node));
            VTR_ASSERT_SAFE(rr_graph.node_ylow(curr_node) == rr_graph.node_yhigh(curr_node));

            found_sink_callback(current);
        }

        expand_dijkstra_neighbours(current, node_visited_costs, node_expanded, pq, bb);
        node_expanded[curr_node] = true;
    }
}

static void expand_dijkstra_neighbours(util::PQ_Entry parent_entry,
                                       vtr::vector<RRNodeId, float>& node_visited_costs,
                                       vtr::vector<RRNodeId, bool>& node_expanded,
                                       std::priority_queue<util::PQ_Entry>& pq,
                                       const t_bb& bb) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    RRNodeId parent = parent_entry.rr_node;

    for (t_edge_size edge : rr_graph.edges(parent)) {
        RRNodeId child_node = rr_graph.edge_sink_node(parent, edge);
        // Don't expand the nodes inside the clusters since the intra-cluster lookahead
        // is computed separately.
        if (!is_inter_cluster_node(rr_graph, child_node)) {
            continue;
        }

        // Don't expand nodes whose adjusted position falls outside of the bounding box.
        auto [child_x, child_y] = util::get_adjusted_rr_position(child_node);
        if (child_x < bb.xmin || child_x > bb.xmax || child_y < bb.ymin || child_y > bb.ymax) {
            continue;
        }

        int switch_ind = size_t(rr_graph.edge_switch(parent, edge));

        if (rr_graph.node_type(child_node) == e_rr_type::SINK) return;

        /* skip this child if it has already been expanded from */
        if (node_expanded[child_node]) {
            continue;
        }

        util::PQ_Entry child_entry(child_node, switch_ind, parent_entry.delay,
                                   parent_entry.R_upstream, parent_entry.congestion_upstream, false);

        //VTR_ASSERT(child_entry.cost >= 0); //Assertion fails in practise. TODO: debug

        /* skip this child if it has been visited with smaller cost */
        if (node_visited_costs[child_node] >= 0 && node_visited_costs[child_node] < child_entry.cost) {
            continue;
        }

        /* finally, record the cost with which the child was visited and put the child entry on the queue */
        node_visited_costs[child_node] = child_entry.cost;
        pq.push(child_entry);
    }
}
