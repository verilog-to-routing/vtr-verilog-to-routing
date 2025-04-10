/********************************************************************
 * This file includes functions that are used to annotate routing results
 * from VPR to OpenFPGA. (i.e. create a mapping from RRNodeIds to ClusterNetIds)
 *******************************************************************/

#include "vpr_error.h"
#include "vtr_assert.h"
#include "vtr_time.h"
#include "vtr_log.h"

#include "route_utils.h"
#include "rr_graph.h"

#include "annotate_routing.h"

vtr::vector<RRNodeId, ClusterNetId> annotate_rr_node_nets(const ClusteringContext& cluster_ctx,
                                                          const DeviceContext& device_ctx,
                                                          const bool& verbose) {
    size_t counter = 0;
    vtr::ScopedStartFinishTimer timer("Annotating rr_node with routed nets");

    const auto& rr_graph = device_ctx.rr_graph;

    auto& netlist = cluster_ctx.clb_nlist;
    vtr::vector<RRNodeId, ClusterNetId> rr_node_nets;
    rr_node_nets.resize(rr_graph.num_nodes(), ClusterNetId::INVALID());

    for (auto net_id : netlist.nets()) {
        if (netlist.net_is_ignored(net_id)) {
            continue;
        }
        /* Ignore used in local cluster only, reserved one CLB pin */
        if (netlist.net_sinks(net_id).empty()) {
            continue;
        }

        auto& tree = get_route_tree_from_cluster_net_id(net_id);
        if (!tree)
            continue;

        for (auto& rt_node : tree->all_nodes()) {
            const RRNodeId rr_node = rt_node.inode;
            /* Ignore source and sink nodes, they are the common node multiple starting and ending points */
            if ((SOURCE != rr_graph.node_type(rr_node))
                && (SINK != rr_graph.node_type(rr_node))) {
                /* Sanity check: ensure we do not revoke any net mapping
                 * In some routing architectures, node capacity is more than 1
                 * which allows a node to be mapped by multiple nets
                 * Therefore, the sanity check should focus on the nodes
                 * whose capacity is 1
                 */
                if ((rr_node_nets[rr_node])
                    && (1 == rr_graph.node_capacity(rr_node))
                    && (net_id != rr_node_nets[rr_node])) {
                    VPR_FATAL_ERROR(VPR_ERROR_ANALYSIS,
                                    "Detect two nets '%s' and '%s' that are mapped to the same rr_node '%ld'!\n%s\n",
                                    netlist.net_name(net_id).c_str(),
                                    netlist.net_name(rr_node_nets[rr_node]).c_str(),
                                    size_t(rr_node),
                                    describe_rr_node(rr_graph,
                                                     device_ctx.grid,
                                                     device_ctx.rr_indexed_data,
                                                     rr_node,
                                                     false)
                                        .c_str());
                } else {
                    rr_node_nets[rr_node] = net_id;
                }
                counter++;
            }
        }
    }

    VTR_LOGV(verbose, "Done with %d nodes mapping\n", counter);

    return rr_node_nets;
}
