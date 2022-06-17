#include <queue>
#include <random>
#include <algorithm>

#include "vtr_memory.h"
#include "vtr_time.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "rr_graph_util.h"

int seg_index_of_cblock(t_rr_type from_rr_type, int to_node) {
    /* Returns the segment number (distance along the channel) of the connection *
     * box from from_rr_type (CHANX or CHANY) to to_node (IPIN).                 */

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    if (from_rr_type == CHANX)
        return (rr_graph.node_xlow(RRNodeId(to_node)));
    else
        /* CHANY */
        return (rr_graph.node_ylow(RRNodeId(to_node)));
}

int seg_index_of_sblock(int from_node, int to_node) {
    /* Returns the segment number (distance along the channel) of the switch box *
     * box from from_node (CHANX or CHANY) to to_node (CHANX or CHANY).  The     *
     * switch box on the left side of a CHANX segment at (i,j) has seg_index =   *
     * i-1, while the switch box on the right side of that segment has seg_index *
     * = i.  CHANY stuff works similarly.  Hence the range of values returned is *
     * 0 to device_ctx.grid.width()-1 (if from_node is a CHANX) or 0 to device_ctx.grid.height()-1 (if from_node is a CHANY).   */

    t_rr_type from_rr_type, to_rr_type;

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    from_rr_type = rr_graph.node_type(RRNodeId(from_node));
    to_rr_type = rr_graph.node_type(RRNodeId(to_node));

    if (from_rr_type == CHANX) {
        if (to_rr_type == CHANY) {
            return (rr_graph.node_xlow(RRNodeId(to_node)));
        } else if (to_rr_type == CHANX) {
            if (rr_graph.node_xlow(RRNodeId(to_node)) > rr_graph.node_xlow(RRNodeId(from_node))) { /* Going right */
                return (rr_graph.node_xhigh(RRNodeId(from_node)));
            } else { /* Going left */
                return (rr_graph.node_xhigh(RRNodeId(to_node)));
            }
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "in seg_index_of_sblock: to_node %d is of type %d.\n",
                            to_node, to_rr_type);
            return OPEN; //Should not reach here once thrown
        }
    }
    /* End from_rr_type is CHANX */
    else if (from_rr_type == CHANY) {
        if (to_rr_type == CHANX) {
            return (rr_graph.node_ylow(RRNodeId(to_node)));
        } else if (to_rr_type == CHANY) {
            if (rr_graph.node_ylow(RRNodeId(to_node)) > rr_graph.node_ylow(RRNodeId(from_node))) { /* Going up */
                return (rr_graph.node_yhigh(RRNodeId(from_node)));
            } else { /* Going down */
                return (rr_graph.node_yhigh(RRNodeId(to_node)));
            }
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "in seg_index_of_sblock: to_node %d is of type %d.\n",
                            to_node, to_rr_type);
            return OPEN; //Should not reach here once thrown
        }
    }
    /* End from_rr_type is CHANY */
    else {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "in seg_index_of_sblock: from_node %d is of type %d.\n",
                        from_node, from_rr_type);
        return OPEN; //Should not reach here once thrown
    }
}

bool is_intra_blk_node(RRNodeId node_id) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_graph;

    t_physical_tile_type_ptr tile_type = device_ctx.grid[rr_graph.node_xlow(node_id)][rr_graph.node_ylow(node_id)].type;

    e_rr_type node_type = rr_graph.node_type(node_id);
    VTR_ASSERT(node_type != CHANX && node_type != CHANY);

    int pin_physical_num = rr_graph.node_ptc_num(node_id);

    if(node_type == IPIN || node_type == OPIN) {
        if(is_class_on_tile(tile_type, pin_physical_num)) {
            return false;
        } else {
            return true;
        }

    } else {
        VTR_ASSERT(node_type == SINK || node_type == SOURCE);
        if(is_pin_on_tile(tile_type, pin_physical_num)) {
            return false;
        } else {
            return true;
        }
    }
}

int get_net_list_pin_rr_node_num(const AtomLookup& atom_lookup,
                                 const ClusteredPinAtomPinsLookup& cluster_pin_lookup,
                                 const Netlist<>& net_list,
                                 const ClusteredNetlist& cluster_net_list,
                                 const vtr::vector<ClusterNetId, std::vector<int>>& cluster_net_terminals,
                                 const ParentNetId net_id,
                                 int pin_index,
                                 bool is_flat) {
    VTR_ASSERT(!net_list.net_is_ignored(net_id));
    int rr_num;
    int cluster_pin_index;
    // For the absorbed nets, we assume that the lookahead return 0.
    ClusterNetId cluster_net_id;
    if(is_flat) {
        cluster_net_id = atom_lookup.clb_net(convert_to_atom_net_id(net_id));
        VTR_ASSERT(!(cluster_net_id == ClusterNetId::INVALID()));
        auto atom_pin_id = net_list.net_pin(net_id, pin_index);
        auto cluster_pin_id = cluster_pin_lookup.connected_clb_pin(convert_to_atom_pin_id(atom_pin_id));
        cluster_pin_index = cluster_net_list.pin_net_index(cluster_pin_id);
    } else {
        cluster_net_id = convert_to_cluster_net_id(net_id);
        cluster_pin_index = pin_index;
    }

    rr_num = cluster_net_terminals[cluster_net_id][cluster_pin_index];

    return rr_num;
}

std::vector<RRNodeId> get_nodes_on_tile(const RRSpatialLookup& rr_spatial_lookup,
                                        t_rr_type node_type,
                                        int root_x,
                                        int root_y) {
    VTR_ASSERT(node_type == IPIN || node_type == SINK || node_type == OPIN || node_type == SOURCE);
    std::vector<RRNodeId> nodes_on_tile;
    auto& device_ctx = g_vpr_ctx.device();
    t_physical_tile_type_ptr tile_type = device_ctx.grid[root_x][root_y].type;
    // TODO: A helper function should be written to return the range of indices related to the pins on the tile
    int num_pin;
    if (node_type == IPIN || node_type == OPIN) {
        num_pin = tile_type->num_pins;
    } else {
        VTR_ASSERT(node_type == SINK || node_type == SOURCE);
        num_pin = tile_type->class_inf.size();
    }
    nodes_on_tile.reserve(num_pin);
    for(int pin_num = 0; pin_num < num_pin; pin_num++) {
        for(auto side: SIDES) {
            auto tmp_node = rr_spatial_lookup.find_node(root_x, root_y, node_type, pin_num, side);
            if(tmp_node != RRNodeId::INVALID()) {
                nodes_on_tile.push_back(tmp_node);
                break;
            }
        }
    }

    return nodes_on_tile;
}
bool is_node_on_tile(t_rr_type node_type,
                     int root_x,
                     int root_y,
                     int node_ptc) {
    VTR_ASSERT(node_type == IPIN || node_type == SINK || node_type == OPIN || node_type == SOURCE);
    auto& device_ctx = g_vpr_ctx.device();
    t_physical_tile_type_ptr tile_type = device_ctx.grid[root_x][root_y].type;
    // TODO: to get the range of ptc numbers related to pins on the cluster, in contrast to the pins inside the cluster, a helper function
    // needs to be written.
    if(node_type == IPIN || node_type == OPIN) {
        return (node_ptc < tile_type->num_pins);
    } else {
        VTR_ASSERT(node_type == SINK || node_type == SOURCE);
        return (node_ptc < (int)tile_type->class_inf.size());
    }

}

vtr::vector<RRNodeId, std::vector<RREdgeId>> get_fan_in_list() {
    auto& rr_graph = g_vpr_ctx.device().rr_graph;

    vtr::vector<RRNodeId, std::vector<RREdgeId>> node_fan_in_list;

    node_fan_in_list.resize(rr_graph.num_nodes(), std::vector<RREdgeId>(0));
    node_fan_in_list.shrink_to_fit();

    //Walk the graph and increment fanin on all dwnstream nodes
    rr_graph.rr_nodes().for_each_edge(
        [&](RREdgeId edge, __attribute__((unused)) RRNodeId src, RRNodeId sink) {
            node_fan_in_list[sink].push_back(edge);
        });

    return node_fan_in_list;
}
