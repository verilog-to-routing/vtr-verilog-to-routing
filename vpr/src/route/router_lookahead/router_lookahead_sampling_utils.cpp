/**
 * @file This file contains functions for the dijkstra expansion algorithm
 * used for generating the lookahead tables in the map and separable lookaheads.
 */

#include "router_lookahead_sampling_utils.h"
#include "globals.h"
#include "rr_graph_view.h"
#include "vpr_context.h"
#include "vpr_types.h"
#include "vpr_utils.h"

void util::expand_dijkstra_neighbours(util::PQ_Entry parent_entry,
                                      vtr::vector<RRNodeId, float>& node_visited_costs,
                                      vtr::vector<RRNodeId, bool>& node_expanded,
                                      std::priority_queue<util::PQ_Entry>& pq,
                                      const t_bb& bb) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;

    RRNodeId parent = parent_entry.rr_node;
    RRSegmentId parent_segment_id = rr_graph.node_segment(parent);

    for (t_edge_size edge : rr_graph.edges(parent)) {
        RRNodeId child_node = rr_graph.edge_sink_node(parent, edge);
        // Don't expand the nodes inside the clusters since the intra-cluster lookahead
        // is computed separately.
        if (!is_inter_cluster_node(rr_graph, child_node)) {
            continue;
        }

        // If the edge connects between a general segment and clock segment, do not expand.
        // This dijkstra expansion is used to populate cost maps, which assume that the routes
        // stay within the general or clock networks.
        // NOTE: IPINs/OPINs/SOURCEs/SINKs do not have valid segments. This check only cuts
        //       muxes that connect a GENERAL segment to a GCLK segment or vice versa.
        RRSegmentId child_segment_id = rr_graph.node_segment(child_node);
        if (parent_segment_id.is_valid() && child_segment_id.is_valid()) {
            SegResType parent_res_type = rr_graph.rr_segments(parent_segment_id).res_type;
            SegResType child_res_type = rr_graph.rr_segments(child_segment_id).res_type;
            VTR_ASSERT_SAFE(parent_res_type == SegResType::GENERAL || parent_res_type == SegResType::GCLK);
            VTR_ASSERT_SAFE(child_res_type == SegResType::GENERAL || child_res_type == SegResType::GCLK);
            if ((parent_res_type == SegResType::GCLK) ^ (child_res_type == SegResType::GCLK))
                continue;
        }

        // Don't expand nodes whose adjusted position falls outside of the bounding box.
        auto [child_x, child_y] = util::get_adjusted_rr_position(child_node);
        if (child_x < bb.xmin || child_x > bb.xmax || child_y < bb.ymin || child_y > bb.ymax) {
            continue;
        }

        int switch_ind = size_t(rr_graph.edge_switch(parent, edge));

        if (rr_graph.node_type(child_node) == e_rr_type::SINK) return;

        // Skip this child if it has already been expanded from
        if (node_expanded[child_node]) {
            continue;
        }

        util::PQ_Entry child_entry(child_node, switch_ind, parent_entry.delay,
                                   parent_entry.R_upstream, parent_entry.congestion_upstream, false);

        // Skip this child if it has been visited with smaller cost
        if (node_visited_costs[child_node] >= 0 && node_visited_costs[child_node] < child_entry.cost) {
            continue;
        }

        // Finally, record the cost with which the child was visited and put the child entry on the queue
        node_visited_costs[child_node] = child_entry.cost;
        pq.push(child_entry);
    }
}
