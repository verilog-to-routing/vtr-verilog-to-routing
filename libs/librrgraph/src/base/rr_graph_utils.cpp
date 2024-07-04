#include <queue>
#include <random>

#include "rr_graph_utils.h"
#include "vpr_error.h"
#include "rr_graph_obj.h"
#include "rr_graph_builder.h"

std::vector<RRSwitchId> find_rr_graph_switches(const RRGraph& rr_graph,
                                               const RRNodeId& from_node,
                                               const RRNodeId& to_node) {
    std::vector<RRSwitchId> switches;
    std::vector<RREdgeId> edges = rr_graph.find_edges(from_node, to_node);
    if (true == edges.empty()) {
        /* edge is open, we return an empty vector of switches */
        return switches;
    }

    /* Reach here, edge list is not empty, find switch id one by one
     * and update the switch list
     */
    for (auto edge : edges) {
        switches.push_back(rr_graph.edge_switch(edge));
    }

    return switches;
}

int seg_index_of_cblock(const RRGraphView& rr_graph, t_rr_type from_rr_type, int to_node) {
    if (from_rr_type == CHANX)
        return (rr_graph.node_xlow(RRNodeId(to_node)));
    else
        /* CHANY */
        return (rr_graph.node_ylow(RRNodeId(to_node)));
}

int seg_index_of_sblock(const RRGraphView& rr_graph, int from_node, int to_node) {
    t_rr_type from_rr_type, to_rr_type;

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

vtr::vector<RRNodeId, std::vector<RREdgeId>> get_fan_in_list(const RRGraphView& rr_graph) {
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

void set_sink_locs(const RRGraphView& rr_graph, RRGraphBuilder& rr_graph_builder) {
    auto node_fanins = get_fan_in_list(rr_graph);

    // We could keep track of xy-"offsets" by tile and ptc number; however, this results
    // in a ~10% increase in runtime to build the RR graph (on vtr benchmarks)

    // Iterate over all SINK nodes
    for (size_t node = 0; node < rr_graph.num_nodes(); ++node) {
        auto node_id = RRNodeId(node);

        if (rr_graph.node_type((RRNodeId)node_id) != e_rr_type::SINK)
            continue;

        int tile_xlow = rr_graph.node_xlow(node_id);
        int tile_xhigh = rr_graph.node_xhigh(node_id);
        int tile_ylow = rr_graph.node_ylow(node_id);
        int tile_yhigh = rr_graph.node_yhigh(node_id);

        int tile_width = tile_xhigh - tile_xlow;
        int tile_height = tile_yhigh - tile_ylow;

        if (tile_width == 0 && tile_height == 0)
            continue;

        // The IPINs of the current SINK node
        std::unordered_set<RRNodeId> sink_ipins = {};

        // IPINs are always one node away from the SINK. So, we just get the fanins of the SINK
        // and add them to the set
        for (auto edge : node_fanins[node_id]) {
            RRNodeId pin = rr_graph.edge_src_node(edge);

            VTR_ASSERT_SAFE(rr_graph.node_type(pin) == e_rr_type::IPIN);

            // Make sure IPIN in the same cluster as origin
            int curr_x = rr_graph.node_xlow(pin);
            int curr_y = rr_graph.node_ylow(pin);
            if ((curr_x < tile_xlow) || (curr_x > tile_xhigh) || (curr_y < tile_ylow) || (curr_y > tile_yhigh))
                continue;

            sink_ipins.insert(pin);
        }

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

        auto x_avg = (short)round(std::accumulate(x_coords.begin(), x_coords.end(), 0.f) / (double)x_coords.size());
        auto y_avg = (short)round(std::accumulate(y_coords.begin(), y_coords.end(), 0.f) / (double)y_coords.size());

        // Remove old indices from RRSpatialLookup
        int layer = rr_graph.node_layer(node_id);
        int ptc = rr_graph.node_ptc_num(node_id);

        for (int x = tile_xlow; x <= tile_xhigh; ++x) {
            for (int y = tile_ylow; y <= tile_yhigh; ++y) {
                if (x == x_avg && y == y_avg)
                    continue;

                rr_graph_builder.node_lookup().remove_node(node_id, layer, x, y, SINK, ptc);
            }
        }

        rr_graph_builder.set_node_coordinates(node_id, x_avg, y_avg, x_avg, y_avg);
    }
}

bool inter_layer_connections_limited_to_opin(const RRGraphView& rr_graph) {
    bool limited_to_opin = true;
    for (const auto& from_node : rr_graph.nodes()) {
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