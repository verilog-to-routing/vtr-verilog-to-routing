#include "rr_graph_utils.h"
#include <numeric>
#include "vpr_error.h"
#include "rr_graph_obj.h"
#include "rr_graph_builder.h"
#include "rr_graph_view.h"

/*
 * @brief Walk backwards from origin SINK, and insert all cluster-edge IPINs to which origin is connected to sink_ipins
 *
 * @param rr_graph
 * @param fanins A vector where, at each node index, is a vector of edges which are fanins of that node
 * @param sink_ipins The set in which cluster-edge IPINs will be collected; should be empty
 * @param curr The current node in recursion; originally, should be the same as origin
 * @param origin The SINK whose cluster-edge IPINs are to be collected
 */
static void rr_walk_cluster_recursive(const RRGraphView& rr_graph,
                                      const vtr::vector<RRNodeId, std::vector<RREdgeId>>& fanins,
                                      std::unordered_set<RRNodeId>& sink_ipins,
                                      const RRNodeId curr,
                                      const RRNodeId origin);

static void rr_walk_cluster_recursive(const RRGraphView& rr_graph,
                                      const vtr::vector<RRNodeId, std::vector<RREdgeId>>& fanins,
                                      std::unordered_set<RRNodeId>& sink_ipins,
                                      const RRNodeId curr,
                                      const RRNodeId origin) {
    // Make sure SINK in the same cluster as origin
    int curr_x = rr_graph.node_xlow(curr);
    int curr_y = rr_graph.node_ylow(curr);
    if ((curr_x < rr_graph.node_xlow(origin)) || (curr_x > rr_graph.node_xhigh(origin)) || (curr_y < rr_graph.node_ylow(origin)) || (curr_y > rr_graph.node_yhigh(origin)))
        return;

    VTR_ASSERT_SAFE(rr_graph.node_type(origin) == e_rr_type::SINK);

    // We want to go "backward" to the cluster IPINs connected to the origin node
    const std::vector<RREdgeId>& incoming_edges = fanins[curr];
    for (RREdgeId edge : incoming_edges) {
        RRNodeId parent = rr_graph.edge_src_node(edge);
        VTR_ASSERT_SAFE(parent != RRNodeId::INVALID());

        if (rr_graph.node_type(parent) != e_rr_type::IPIN) {
            if (rr_graph.node_type(parent) != e_rr_type::CHANX && rr_graph.node_type(parent) != e_rr_type::CHANY)
                return;

            // If the parent node isn't in the origin's cluster, the current node is a "cluster-edge" pin,
            // so add it to sink_ipins
            sink_ipins.insert(curr);
            return;
        }

        // If the parent node is intra-cluster, keep going "backward"
        rr_walk_cluster_recursive(rr_graph, fanins, sink_ipins, parent, origin);
    }
}

std::vector<RRSwitchId> find_rr_graph_switches(const RRGraph& rr_graph,
                                               RRNodeId from_node,
                                               RRNodeId to_node) {
    std::vector<RRSwitchId> switches;
    std::vector<RREdgeId> edges = rr_graph.find_edges(from_node, to_node);
    if (edges.empty()) {
        /* edge is open, we return an empty vector of switches */
        return switches;
    }

    // Reach here, edge list is not empty, find switch id one by one
    // and update the switch list
    for (RREdgeId edge : edges) {
        switches.push_back(rr_graph.edge_switch(edge));
    }

    return switches;
}

int seg_index_of_cblock(const RRGraphView& rr_graph, e_rr_type from_rr_type, int to_node) {
    if (from_rr_type == e_rr_type::CHANX)
        return (rr_graph.node_xlow(RRNodeId(to_node)));
    else
        /* CHANY */
        return (rr_graph.node_ylow(RRNodeId(to_node)));
}

int seg_index_of_sblock(const RRGraphView& rr_graph, int from_node, int to_node) {
    e_rr_type from_rr_type = rr_graph.node_type(RRNodeId(from_node));
    e_rr_type to_rr_type = rr_graph.node_type(RRNodeId(to_node));

    if (from_rr_type == e_rr_type::CHANX) {
        if (to_rr_type == e_rr_type::CHANY) {
            return (rr_graph.node_xlow(RRNodeId(to_node)));
        } else if (to_rr_type == e_rr_type::CHANX) {
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
    else if (from_rr_type == e_rr_type::CHANY) {
        if (to_rr_type == e_rr_type::CHANX) {
            return (rr_graph.node_ylow(RRNodeId(to_node)));
        } else if (to_rr_type == e_rr_type::CHANY) {
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

vtr::vector<RRNodeId, std::vector<RREdgeId>> get_fan_in_list(const RRGraphView& rr_graph) {
    vtr::vector<RRNodeId, std::vector<RREdgeId>> node_fan_in_list;

    node_fan_in_list.resize(rr_graph.num_nodes(), std::vector<RREdgeId>(0));
    node_fan_in_list.shrink_to_fit();

    // Walk the graph and increment fanin on all downstream nodes
    rr_graph.rr_nodes().for_each_edge(
        [&](RREdgeId edge, RRNodeId src, RRNodeId sink) -> void {
            (void) src;
            node_fan_in_list[sink].push_back(edge);
        });

    return node_fan_in_list;
}

void rr_set_sink_locs(const RRGraphView& rr_graph, RRGraphBuilder& rr_graph_builder, const DeviceGrid& grid) {
    const vtr::vector<RRNodeId, std::vector<RREdgeId>> node_fanins = get_fan_in_list(rr_graph);

    // Keep track of offsets for SINKs for each tile type, to avoid repeated calculations
    std::unordered_map<t_physical_tile_type_ptr, std::unordered_map<int, vtr::Point<int>>> physical_type_offsets;

    // Iterate over all SINK nodes
    for (size_t node = 0; node < rr_graph.num_nodes(); ++node) {
        auto node_id = RRNodeId(node);

        if (rr_graph.node_type((RRNodeId)node_id) != e_rr_type::SINK)
            continue;

        int node_xlow = rr_graph.node_xlow(node_id);
        int node_ylow = rr_graph.node_ylow(node_id);
        int node_xhigh = rr_graph.node_xhigh(node_id);
        int node_yhigh = rr_graph.node_yhigh(node_id);

        // Skip "0x0" nodes; either the tile is 1x1, or we have seen the node on a previous call of this function
        // and its new location has already been set
        if ((node_xhigh - node_xlow) == 0 && (node_yhigh - node_ylow) == 0)
            continue;

        int node_layer = rr_graph.node_layer(node_id);
        int node_ptc = rr_graph.node_ptc_num(node_id);

        t_physical_tile_loc tile_loc = {node_xlow, node_ylow, node_layer};
        t_physical_tile_type_ptr tile_type = grid.get_physical_type(tile_loc);
        vtr::Rect<int> tile_bb = grid.get_tile_bb(tile_loc);

        // See if we have encountered this tile type/ptc combo before, and used saved offset if so
        vtr::Point<int> new_loc(-1, -1);
        if ((physical_type_offsets.find(tile_type) != physical_type_offsets.end()) && (physical_type_offsets[tile_type].find(node_ptc) != physical_type_offsets[tile_type].end())) {
            new_loc = tile_bb.bottom_left() + physical_type_offsets[tile_type].at(node_ptc);
        } else { /* We have not seen this tile type/ptc combo before */
            // The IPINs of the current SINK node
            std::unordered_set<RRNodeId> sink_ipins = {};
            rr_walk_cluster_recursive(rr_graph, node_fanins, sink_ipins, node_id, node_id);

            /* Set SINK locations as average of collected IPINs */

            if (sink_ipins.empty())
                continue;

            // Use float so that we can take average later
            std::vector<float> x_coords;
            std::vector<float> y_coords;

            // Add coordinates of each "cluster-edge" pin to vectors
            for (const auto& pin : sink_ipins) {
                int pin_x = rr_graph.node_xlow(pin);
                int pin_y = rr_graph.node_ylow(pin);

                VTR_ASSERT_SAFE(pin_x == rr_graph.node_xhigh(pin));
                VTR_ASSERT_SAFE(pin_y == rr_graph.node_yhigh(pin));

                x_coords.push_back((float)pin_x);
                y_coords.push_back((float)pin_y);
            }

            new_loc = {(int)round(std::accumulate(x_coords.begin(), x_coords.end(), 0.f) / (double)x_coords.size()),
                       (int)round(std::accumulate(y_coords.begin(), y_coords.end(), 0.f) / (double)y_coords.size())};

            // Save offset for this tile/ptc combo
            if (physical_type_offsets.find(tile_type) == physical_type_offsets.end())
                physical_type_offsets[tile_type] = {};

            physical_type_offsets[tile_type].insert({node_ptc, new_loc - tile_bb.bottom_left()});
        }

        // Remove old locations from lookup
        VTR_ASSERT(rr_graph_builder.node_lookup().find_node(node_layer, new_loc.x(), new_loc.y(), e_rr_type::SINK, node_ptc) != RRNodeId::INVALID());

        for (int x = tile_bb.xmin(); x <= tile_bb.xmax(); ++x) {
            for (int y = tile_bb.ymin(); y <= tile_bb.ymax(); ++y) {
                if (x == new_loc.x() && y == new_loc.y()) /* The new sink location */
                    continue;

                if (rr_graph_builder.node_lookup().find_node(node_layer, x, y, e_rr_type::SINK, node_ptc) == RRNodeId::INVALID()) /* Already removed */
                    continue;

                bool removed_successfully = rr_graph_builder.node_lookup().remove_node(node_id, node_layer, x, y, e_rr_type::SINK, node_ptc);
                VTR_ASSERT(removed_successfully);
            }
        }

        // Set new coordinates
        rr_graph_builder.set_node_coordinates(node_id, (short)new_loc.x(), (short)new_loc.y(), (short)new_loc.x(), (short)new_loc.y());
    }
}

bool inter_layer_connections_limited_to_opin(const RRGraphView& rr_graph) {
    bool limited_to_opin = true;
    for (const RRNodeId from_node : rr_graph.nodes()) {
        for (t_edge_size edge : rr_graph.edges(from_node)) {
            RRNodeId to_node = rr_graph.edge_sink_node(from_node, edge);
            int from_layer = rr_graph.node_layer(from_node);
            int to_layer = rr_graph.node_layer(to_node);

            if (from_layer != to_layer) {
                if (rr_graph.node_type(from_node) != e_rr_type::OPIN) {
                    limited_to_opin = false;
                    break;
                }
            }
        }
        if (!limited_to_opin) {
            break;
        }
    }

    return limited_to_opin;
}

bool chanx_chany_nodes_are_adjacent(const RRGraphView& rr_graph, RRNodeId node1, RRNodeId node2) {
    e_rr_type type1 = rr_graph.node_type(node1);
    e_rr_type type2 = rr_graph.node_type(node2);
    VTR_ASSERT((type1 == e_rr_type::CHANX && type2 == e_rr_type::CHANY) || (type1 == e_rr_type::CHANY && type2 == e_rr_type::CHANX));

    // Make sure node1 is CHANX for consistency
    if (type1 == e_rr_type::CHANY) {
        std::swap(node1, node2);
        std::swap(type1, type2);
    }

    RRNodeId chanx_node = node1;
    RRNodeId chany_node = node2;

    // Check vertical (Y) adjacency
    if (rr_graph.node_ylow(chany_node) > rr_graph.node_ylow(chanx_node) + 1 ||
        rr_graph.node_yhigh(chany_node) < rr_graph.node_ylow(chanx_node)) {
        return false;
    }

    // Check horizontal (X) adjacency
    if (rr_graph.node_xlow(chanx_node) > rr_graph.node_xlow(chany_node) + 1 ||
        rr_graph.node_xhigh(chanx_node) < rr_graph.node_xlow(chany_node)) {
        return false;
    }

    return true;
}

bool chanxy_chanz_adjacent(const RRGraphView& rr_graph, RRNodeId node1, RRNodeId node2) {
    e_rr_type type1 = rr_graph.node_type(node1);
    e_rr_type type2 = rr_graph.node_type(node2);

    // Ensure one is CHANZ, the other is CHANX or CHANY
    VTR_ASSERT((type1 == e_rr_type::CHANZ && (type2 == e_rr_type::CHANX || type2 == e_rr_type::CHANY))
               || (type2 == e_rr_type::CHANZ && (type1 == e_rr_type::CHANX || type1 == e_rr_type::CHANY)));

    // Make sure chanz_node is second for unified handling
    if (type1 == e_rr_type::CHANZ) {
        std::swap(node1, node2);
        std::swap(type1, type2);
    }

    // node1 is CHANX or CHANY, node2 is CHANZ
    RRNodeId chanxy_node = node1;
    RRNodeId chanz_node = node2;
    e_rr_type chanxy_node_type = type1;

    int chanz_x = rr_graph.node_xlow(chanz_node);
    int chanz_y = rr_graph.node_ylow(chanz_node);
    VTR_ASSERT_SAFE(chanz_x == rr_graph.node_xhigh(chanz_node));
    VTR_ASSERT_SAFE(chanz_y == rr_graph.node_yhigh(chanz_node));

    if (chanxy_node_type == e_rr_type::CHANX) {
        // CHANX runs horizontally: match Y, overlap X
        return chanz_y == rr_graph.node_ylow(chanxy_node)
               && chanz_x >= rr_graph.node_xlow(chanxy_node) - 1
               && chanz_x <= rr_graph.node_xhigh(chanxy_node) + 1;
    } else {
        // CHANY runs vertically: match X, overlap Y
        return chanz_x == rr_graph.node_xlow(chanxy_node)
               && chanz_y >= rr_graph.node_ylow(chanxy_node) - 1
               && chanz_y <= rr_graph.node_yhigh(chanxy_node) + 1;
    }
}

bool chan_same_type_are_adjacent(const RRGraphView& rr_graph, RRNodeId node1, RRNodeId node2) {
    e_rr_type type = rr_graph.node_type(node1);
    VTR_ASSERT(type == rr_graph.node_type(node2));

    int xlow1 = rr_graph.node_xlow(node1);
    int xhigh1 = rr_graph.node_xhigh(node1);
    int ylow1 = rr_graph.node_ylow(node1);
    int yhigh1 = rr_graph.node_yhigh(node1);
    int layer1 = rr_graph.node_layer(node1);

    int xlow2 = rr_graph.node_xlow(node2);
    int xhigh2 = rr_graph.node_xhigh(node2);
    int ylow2 = rr_graph.node_ylow(node2);
    int yhigh2 = rr_graph.node_yhigh(node2);
    int layer2 = rr_graph.node_layer(node2);

    if (type == e_rr_type::CHANX) {
        if (ylow1 != ylow2) {
            return false;
        }

        // Adjacent ends or overlapping segments
        if (xhigh1 == xlow2 - 1 || xhigh2 == xlow1 - 1) {
            return true;
        }

        for (int x = xlow1; x <= xhigh1; ++x) {
            if (x >= xlow2 && x <= xhigh2) {
                return true;
            }
        }
        return false;

    } else if (type == e_rr_type::CHANY) {
        if (xlow1 != xlow2) {
            return false;
        }

        if (yhigh1 == ylow2 - 1 || yhigh2 == ylow1 - 1) {
            return true;
        }

        for (int y = ylow1; y <= yhigh1; ++y) {
            if (y >= ylow2 && y <= yhigh2) {
                return true;
            }
        }
        return false;

    } else if (type == e_rr_type::CHANZ) {
        // Same X/Y span, adjacent layer
        bool same_xy = (xlow1 == xlow2 && xhigh1 == xhigh2 && ylow1 == ylow2 && yhigh1 == yhigh2);
        bool adjacent_layer = std::abs(layer1 - layer2) == 1;
        return same_xy && adjacent_layer;
    } else {
        VTR_ASSERT_MSG(false, "Unexpected RR node type in chan_same_type_are_adjacent().\n");
    }

    return false; // unreachable
}