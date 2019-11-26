/* Standard header files required go first */
#include <map>

/* EXTERNAL library header files go second*/
#include "vtr_assert.h"
#include "vtr_time.h"
#include "vtr_memory.h"

/* VPR header files go then */
#include "vpr_types.h"
#include "rr_graph_obj.h"
#include "check_rr_graph_obj.h"
#include "create_rr_graph.h"

/* Finally we include global variables */
#include "globals.h"

/********************************************************************
 * TODO: remove when this conversion (from traditional to new data structure)
 * is no longer needed
 * This function will convert an existing rr_graph in device_ctx to the RRGraph
 *object
 * This function is used to test our RRGraph if it is acceptable in downstream
 *routers
 ********************************************************************/
void convert_rr_graph(std::vector<t_segment_inf>& vpr_segments) {
    /* Release freed memory before start building rr_graph */
    vtr::malloc_trim(0);

    vtr::ScopedStartFinishTimer timer("Conversion to routing resource graph object");

    /* IMPORTANT: to build clock tree,
     * vpr added segments to the original arch segments
     * This is why to use vpr_segments as an inputs!!!
     */
    auto& device_ctx = g_vpr_ctx.mutable_device();

    /* The number of switches are in general small,
     * reserve switches may not bring significant memory efficiency
     * So, we just use create_switch to push_back each time
     */
    device_ctx.rr_graph.reserve_switches(device_ctx.rr_switch_inf.size());
    // Create the switches
    for (size_t iswitch = 0; iswitch < device_ctx.rr_switch_inf.size(); ++iswitch) {
        device_ctx.rr_graph.create_switch(device_ctx.rr_switch_inf[iswitch]);
    }

    /* The number of segments are in general small, reserve segments may not bring
     * significant memory efficiency */
    device_ctx.rr_graph.reserve_segments(vpr_segments.size());
    // Create the segments
    for (size_t iseg = 0; iseg < vpr_segments.size(); ++iseg) {
        device_ctx.rr_graph.create_segment(vpr_segments[iseg]);
    }

    /* Reserve list of nodes to be memory efficient */
    device_ctx.rr_graph.reserve_nodes((unsigned long)device_ctx.rr_nodes.size());

    // Create the nodes
    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); ++inode) {
        auto& node = device_ctx.rr_nodes[inode];
        RRNodeId rr_node = device_ctx.rr_graph.create_node(node.type());

        device_ctx.rr_graph.set_node_xlow(rr_node, node.xlow());
        device_ctx.rr_graph.set_node_ylow(rr_node, node.ylow());
        device_ctx.rr_graph.set_node_xhigh(rr_node, node.xhigh());
        device_ctx.rr_graph.set_node_yhigh(rr_node, node.yhigh());

        device_ctx.rr_graph.set_node_capacity(rr_node, node.capacity());

        device_ctx.rr_graph.set_node_ptc_num(rr_node, node.ptc_num());

        device_ctx.rr_graph.set_node_cost_index(rr_node, node.cost_index());

        if (CHANX == node.type() || CHANY == node.type()) {
            device_ctx.rr_graph.set_node_direction(rr_node, node.direction());
        }
        if (IPIN == node.type() || OPIN == node.type()) {
            device_ctx.rr_graph.set_node_side(rr_node, node.side());
        }
        device_ctx.rr_graph.set_node_R(rr_node, node.R());
        device_ctx.rr_graph.set_node_C(rr_node, node.C());

        /* Set up segment id */
        short irc_data = node.cost_index();
        short iseg = device_ctx.rr_indexed_data[irc_data].seg_index;
        device_ctx.rr_graph.set_node_segment(rr_node, RRSegmentId(iseg));
    }

    /* Reserve list of edges to be memory efficient */
    unsigned long num_edges_to_reserve = 0;
    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); ++inode) {
        const auto& node = device_ctx.rr_nodes[inode];
        num_edges_to_reserve += node.num_edges();
    }
    device_ctx.rr_graph.reserve_edges(num_edges_to_reserve);

    /* Add edges for each node */
    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); ++inode) {
        const auto& node = device_ctx.rr_nodes[inode];
        for (int iedge = 0; iedge < node.num_edges(); ++iedge) {
            size_t isink_node = node.edge_sink_node(iedge);
            int iswitch = node.edge_switch(iedge);

            device_ctx.rr_graph.create_edge(RRNodeId(inode),
                                            RRNodeId(isink_node),
                                            RRSwitchId(iswitch));
        }
    }

    /* Ensure that we reserved what we want */
    VTR_ASSERT(num_edges_to_reserve == (unsigned long)device_ctx.rr_graph.edges().size());

    /* Partition edges to be two class: configurable (1st part) and
     * non-configurable (2nd part)
     * See how the router will use the edges and determine the strategy
     * if we want to partition the edges first or depends on the routing needs
     */
    device_ctx.rr_graph.rebuild_node_edges();

    /* Essential check for rr_graph, build look-up and  */
    if (false == device_ctx.rr_graph.validate()) {
        /* Error out if built-in validator of rr_graph fails */
        vpr_throw(VPR_ERROR_ROUTE,
                  __FILE__,
                  __LINE__,
                  "Fundamental errors occurred when validating rr_graph object!\n");
    }

    /* Error out if advanced checker of rr_graph fails */
    if (false == check_rr_graph(device_ctx.rr_graph)) {
        vpr_throw(VPR_ERROR_ROUTE,
                  __FILE__,
                  __LINE__,
                  "Advanced checking rr_graph object fails! Routing may still work "
                  "but not smooth\n");
    }
}
