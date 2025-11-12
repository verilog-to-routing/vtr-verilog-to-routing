#include <cstddef>
#include <vector>
#include <algorithm>
#include <ranges>

#include "device_grid.h"
#include "rr_graph_builder.h"
#include "rr_graph_fwd.h"
#include "rr_graph_view.h"
#include "rr_node_types.h"
#include "rr_spatial_lookup.h"
#include "vtr_assert.h"
#include "vtr_log.h"

#include "interposer_cut.h"

/**
 * @brief Takes location of a source and a sink and determines wether it crosses cut_loc or not. For example, the interval (1, 4) is cut by 3, while it is not cut by 5 or 0.
 */
static bool should_cut(int src_loc, int sink_loc, int cut_loc) {
    int src_delta = src_loc - cut_loc;
    int sink_delta = sink_loc - cut_loc;

    // Same sign means that both sink and source are on the same side of this cut
    if ((src_delta <= 0 && sink_delta <= 0) || (src_delta > 0 && sink_delta > 0)) {
        return false;
    } else {
        return true;
    }
}

/**
 * @brief Calculates the starting x point of node based on it's directionality.
 */
static short node_xstart(const RRGraphView& rr_graph, RRNodeId node) {
    // Return early for OPIN and IPIN types (Some BIDIR pins would trigger the assertion below)
    if (rr_graph.node_type(node) == e_rr_type::OPIN || rr_graph.node_type(node) == e_rr_type::IPIN) {
        VTR_ASSERT(rr_graph.node_xlow(node) == rr_graph.node_xhigh(node));
        return rr_graph.node_xlow(node);
    }

    switch (rr_graph.node_direction(node)) {
        case Direction::DEC:
            return rr_graph.node_xhigh(node);
            break;

        case Direction::INC:
            return rr_graph.node_xlow(node);
            break;

        case Direction::NONE:
            VTR_ASSERT(rr_graph.node_xlow(node) == rr_graph.node_xhigh(node));
            return (rr_graph.node_xlow(node));
            break;

        case Direction::BIDIR:
            VTR_ASSERT_MSG(false, "Bidir node has no starting point");
            break;

        default:
            VTR_ASSERT(false);
            break;
    }
}

/**
 * @brief Calculates the starting y point of node based on it's directionality.
 */
static short node_ystart(const RRGraphView& rr_graph, RRNodeId node) {
    // Return early for OPIN and IPIN types (Some BIDIR pins would trigger the assertion below)
    if (rr_graph.node_type(node) == e_rr_type::OPIN || rr_graph.node_type(node) == e_rr_type::IPIN) {
        return rr_graph.node_ylow(node);
    }

    switch (rr_graph.node_direction(node)) {
        case Direction::DEC:
            return rr_graph.node_yhigh(node);
            break;

        case Direction::INC:
            return rr_graph.node_ylow(node);
            break;

        case Direction::NONE:
            VTR_ASSERT(rr_graph.node_ylow(node) == rr_graph.node_yhigh(node));
            return (rr_graph.node_ylow(node));
            break;

        case Direction::BIDIR:
            VTR_ASSERT_MSG(false, "Bidir node has no starting point");
            break;

        default:
            VTR_ASSERT(false);
            break;
    }
}

std::vector<RREdgeId> mark_interposer_cut_edges_for_removal(const RRGraphView& rr_graph, const DeviceGrid& grid) {
    std::vector<RREdgeId> edges_to_be_removed;

    // Loop over all RREdgeIds and mark ones that cross a cutline to be removed

    for (RREdgeId edge_id : rr_graph.all_edges()) {
        RRNodeId src_node = rr_graph.edge_src_node(edge_id);
        RRNodeId sink_node = rr_graph.edge_sink_node(edge_id);

        if (src_node == RRNodeId(5866) && sink_node == RRNodeId(5604)) {
            VTR_LOG("HI\n");
        }

        // TODO: ignoring chanz nodes for now
        if (rr_graph.node_type(src_node) == e_rr_type::CHANZ || rr_graph.node_type(sink_node) == e_rr_type::CHANZ) {
            continue;
        }

        VTR_ASSERT(rr_graph.node_layer_low(src_node) == rr_graph.node_layer_low(sink_node));
        VTR_ASSERT(rr_graph.node_layer_low(src_node) == rr_graph.node_layer_high(src_node));
        VTR_ASSERT(rr_graph.node_layer_low(sink_node) == rr_graph.node_layer_high(sink_node));

        int layer = rr_graph.node_layer_low(src_node);

        for (int cut_loc_y : grid.get_horizontal_interposer_cuts()[layer]) {
            int src_start_loc_y = node_ystart(rr_graph, src_node);
            int sink_start_loc_y = node_ystart(rr_graph, sink_node);

            if (should_cut(src_start_loc_y, sink_start_loc_y, cut_loc_y)) {
                edges_to_be_removed.push_back(edge_id);
            }
        }

        for (int cut_loc_x : grid.get_vertical_interposer_cuts()[layer]) {
            int src_start_loc_x = node_xstart(rr_graph, src_node);
            int sink_start_loc_x = node_xstart(rr_graph, sink_node);

            if (should_cut(src_start_loc_x, sink_start_loc_x, cut_loc_x)) {
                edges_to_be_removed.push_back(edge_id);
            }
        }
    }

    return edges_to_be_removed;
}

/**
 * @brief Update a CHANY node's bounding box in RRGraph and SpatialLookup entries if it crosses cut_loc_y
 * 
 * @param node CHANY RR graph node that might cross the interposer cut line
 * @param cut_loc_y Y location of horizontal interposer cut line
 * @param sg_node_indices List of scatter-gather node IDs. We do not want to cut these nodes as they're allowed to cross an interposer cut line.
 * @note This function is very similar to cut_chan_x_node. If you're modifying this you probably also want to modify that function too.
 */
static void cut_chan_y_node(RRNodeId node, int cut_loc_y, const RRGraphView& rr_graph, RRGraphBuilder& rr_graph_builder, RRSpatialLookup& spatial_lookup) {
    VTR_ASSERT_SAFE(rr_graph.node_type(node) == e_rr_type::CHANY);

    int x_high = rr_graph.node_xhigh(node);
    int x_low = rr_graph.node_xlow(node);

    int y_high = rr_graph.node_yhigh(node);
    int y_low = rr_graph.node_ylow(node);

    int layer = rr_graph.node_layer_low(node);

    int ptc_num = rr_graph.node_ptc_num(node);

    if (!should_cut(y_low, y_high, cut_loc_y)) {
        return;
    }

    if (rr_graph.node_direction(node) == Direction::INC) {
        // Anything above cut_loc_y shouldn't exist
        rr_graph_builder.set_node_coordinates(node, x_low, y_low, x_high, cut_loc_y);

        // Do a loop from cut_loc_y to y_high and remove node from spatial lookup
        for (int y_loc = cut_loc_y + 1; y_loc <= y_high; y_loc++) {
            spatial_lookup.remove_node(node, layer, x_low, y_loc, e_rr_type::CHANY, ptc_num);
        }
    } else if (rr_graph.node_direction(node) == Direction::DEC) {
        // Anything below cut_loc_y (inclusive) shouldn't exist
        rr_graph_builder.set_node_coordinates(node, x_low, cut_loc_y + 1, x_high, y_high);

        // Do a loop from y_low to cut_loc_y and remove node from spatial lookup
        for (int y_loc = y_low; y_loc <= cut_loc_y; y_loc++) {
            spatial_lookup.remove_node(node, layer, x_low, y_loc, e_rr_type::CHANY, ptc_num);
        }
    } else {
        VTR_ASSERT_MSG(false, "Bidirectional routing is not supported for interposer architectures.");
    }
}

/**
 * @brief Update a CHANX node's bounding box in RRGraph and SpatialLookup entries if it crosses cut_loc_x
 * 
 * @param node CHANX RR graph node that might cross the interposer cut line
 * @param cut_loc_y X location of vertical interposer cut line
 * @param sg_node_indices List of scatter-gather node IDs. We do not want to cut these nodes as they're allowed to cross an interposer cut line.
 * @note This function is very similar to cut_chan_y_node. If you're modifying this you probably also want to modify that function too.
 */
static void cut_chan_x_node(RRNodeId node, int cut_loc_x, const RRGraphView& rr_graph, RRGraphBuilder& rr_graph_builder, RRSpatialLookup& spatial_lookup) {
    VTR_ASSERT_SAFE(rr_graph.node_type(node) == e_rr_type::CHANX);

    int x_high = rr_graph.node_xhigh(node);
    int x_low = rr_graph.node_xlow(node);

    int y_high = rr_graph.node_yhigh(node);
    int y_low = rr_graph.node_ylow(node);

    int layer = rr_graph.node_layer_low(node);

    int ptc_num = rr_graph.node_ptc_num(node);

    if (!should_cut(x_low, x_high, cut_loc_x)) {
        return;
    }

    if (rr_graph.node_direction(node) == Direction::INC) {
        // Anything to the right of cut_loc_x shouldn't exist
        rr_graph_builder.set_node_coordinates(node, x_low, y_low, cut_loc_x, y_high);

        // Do a loop from cut_loc_x to x_high and remove node from spatial lookup
        for (int x_loc = cut_loc_x + 1; x_loc <= x_high; x_loc++) {
            spatial_lookup.remove_node(node, layer, x_loc, y_low, e_rr_type::CHANX, ptc_num);
        }
    } else if (rr_graph.node_direction(node) == Direction::DEC) {
        // Anything to the left of cut_loc_x (inclusive) shouldn't exist
        rr_graph_builder.set_node_coordinates(node, cut_loc_x + 1, y_low, x_high, y_high);

        // Do a loop from x_low to cut_loc_x - 1 and remove node from spatial lookup
        for (int x_loc = x_low; x_loc <= cut_loc_x; x_loc++) {
            spatial_lookup.remove_node(node, layer, x_loc, y_low, e_rr_type::CHANX, ptc_num);
        }
    } else {
        VTR_ASSERT_MSG(false, "Bidirectional routing is not supported for interposer architectures.");
    }
}

void update_interposer_crossing_nodes_in_spatial_lookup_and_rr_graph_storage(const RRGraphView& rr_graph, const DeviceGrid& grid, RRGraphBuilder& rr_graph_builder, const std::vector<std::pair<RRNodeId, int>>& sg_node_indices) {

    constexpr auto node_indice_compare = [](RRNodeId l, RRNodeId r) noexcept { return size_t(l) < size_t(r); };
    VTR_ASSERT(std::is_sorted(sg_node_indices.begin(), sg_node_indices.end()));

    RRSpatialLookup& spatial_lookup = rr_graph_builder.node_lookup();
    for (size_t layer = 0; layer < grid.get_num_layers(); layer++) {
        for (int cut_loc_y : grid.get_horizontal_interposer_cuts()[layer]) {
            for (size_t x_loc = 0; x_loc < grid.width(); x_loc++) {
                std::vector<RRNodeId> channel_nodes = spatial_lookup.find_channel_nodes(layer, x_loc, cut_loc_y, e_rr_type::CHANY);
                for (RRNodeId node : channel_nodes) {
                    bool is_sg_node = std::ranges::binary_search(std::views::keys(sg_node_indices), node, node_indice_compare);
                    if (is_sg_node) {
                        continue;
                    }

                    cut_chan_y_node(node, cut_loc_y, rr_graph, rr_graph_builder, spatial_lookup);
                }
            }
        }

        for (int cut_loc_x : grid.get_vertical_interposer_cuts()[layer]) {
            for (size_t y_loc = 0; y_loc < grid.height(); y_loc++) {
                std::vector<RRNodeId> channel_nodes = spatial_lookup.find_channel_nodes(layer, cut_loc_x, y_loc, e_rr_type::CHANX);
                for (RRNodeId node : channel_nodes) {
                    bool is_sg_node = std::ranges::binary_search(std::views::keys(sg_node_indices), node, node_indice_compare);
                    if (is_sg_node) {
                        continue;
                    }

                    cut_chan_x_node(node, cut_loc_x, rr_graph, rr_graph_builder, spatial_lookup);
                }
            }
        }
    }
}
