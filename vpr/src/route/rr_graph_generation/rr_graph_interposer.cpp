#include <cstddef>
#include <vector>
#include <algorithm>
#include <ranges>

#include "device_grid.h"
#include "interposer_types.h"
#include "rr_graph_builder.h"
#include "rr_graph_fwd.h"
#include "rr_graph_view.h"
#include "rr_node_types.h"
#include "rr_spatial_lookup.h"
#include "vpr_error.h"
#include "vtr_assert.h"

#include "rr_graph_interposer.h"

/**
 * @brief Takes location of a source and a sink and determines wether it crosses cut_loc or not.
 * For example, the interval (1, 4) is cut by 3, while it is not cut by 5 or 0.
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
    e_rr_type node_type = rr_graph.node_type(node);
    Direction node_direction = rr_graph.node_direction(node);
    short node_xlow = rr_graph.node_xlow(node);
    short node_xhigh = rr_graph.node_xhigh(node);

    // Return early for OPIN and IPIN types (Some BIDIR pins would trigger the assertion below)
    if (is_pin(node_type)) {
        VTR_ASSERT(node_xlow == node_xhigh);
        return node_xlow;
    }

    switch (node_direction) {
        case Direction::DEC:
            return node_xhigh;

        case Direction::INC:
            return node_xlow;

        case Direction::NONE:
            VTR_ASSERT(node_xlow == node_xhigh);
            return (node_xlow);

        case Direction::BIDIR:
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Bidir node has no starting point.");
            break;

        default:
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Invalid RR node direction.");
            break;
    }
}

/**
 * @brief Calculates the starting y point of node based on it's directionality.
 */
static short node_ystart(const RRGraphView& rr_graph, RRNodeId node) {
    e_rr_type node_type = rr_graph.node_type(node);
    Direction node_direction = rr_graph.node_direction(node);
    short node_ylow = rr_graph.node_ylow(node);
    short node_yhigh = rr_graph.node_yhigh(node);

    // Return early for OPIN and IPIN types (Some BIDIR pins would trigger the assertion below)
    if (is_pin(node_type)) {
        return node_ylow;
    }

    switch (node_direction) {
        case Direction::DEC:
            return node_yhigh;

        case Direction::INC:
            return node_ylow;

        case Direction::NONE:
            VTR_ASSERT(node_ylow == node_yhigh);
            return (node_ylow);

        case Direction::BIDIR:
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Bidir node has no starting point.");
            break;

        default:
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Invalid RR node direction.");
            break;
    }
}

std::vector<RREdgeId> mark_interposer_cut_edges_for_removal(const RRGraphView& rr_graph, const DeviceGrid& grid) {
    std::vector<RREdgeId> edges_to_be_removed;

    // Loop over all RREdgeIds and mark ones that cross a cutline to be removed

    for (RREdgeId edge_id : rr_graph.all_edges()) {
        RRNodeId src_node = rr_graph.edge_src_node(edge_id);
        RRNodeId sink_node = rr_graph.edge_sink_node(edge_id);

        // TODO: ignoring ChanZ nodes for now
        if (rr_graph.node_type(src_node) == e_rr_type::CHANZ || rr_graph.node_type(sink_node) == e_rr_type::CHANZ) {
            continue;
        }

        // Currently 3D connection block edges cross layers. We don't want to cut these edges.
        if (rr_graph.node_layer_low(src_node) != rr_graph.node_layer_low(sink_node)) {
            continue;
        }
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
 * @brief Update a CHANY node's bounding box in RRGraph and SpatialLookup entries.
 * This function assumes that the channel node actually crosses the cut location and
 * might not function correctly otherwise.
 *
 * This is a low level function, you should use cut_channel_node that wraps this up in a nicer API.
 */
static void cut_chan_y_node(RRNodeId node, int x_low, int y_low, int x_high, int y_high, int layer, int ptc_num, int cut_loc_y,
                            Direction node_direction, RRGraphBuilder& rr_graph_builder, RRSpatialLookup& spatial_lookup) {
    if (node_direction == Direction::INC) {
        // Anything above cut_loc_y shouldn't exist
        rr_graph_builder.set_node_coordinates(node, x_low, y_low, x_high, cut_loc_y);

        // Do a loop from cut_loc_y to y_high and remove node from spatial lookup
        for (int y_loc = cut_loc_y + 1; y_loc <= y_high; y_loc++) {
            spatial_lookup.remove_node(node, layer, x_low, y_loc, e_rr_type::CHANY, ptc_num);
        }
    } else if (node_direction == Direction::DEC) {
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
 * @brief Update a CHANX node's bounding box in RRGraph and SpatialLookup entries.
 * This function assumes that the channel node actually crosses the cut location and
 * might not function correctly otherwise.
 *
 * This is a low level function, you should use cut_channel_node that wraps this up in a nicer API.
 */
static void cut_chan_x_node(RRNodeId node, int x_low, int y_low, int x_high, int y_high, int layer, int ptc_num, int cut_loc_x,
                            Direction node_direction, RRGraphBuilder& rr_graph_builder, RRSpatialLookup& spatial_lookup) {
    if (node_direction == Direction::INC) {
        // Anything to the right of cut_loc_x shouldn't exist
        rr_graph_builder.set_node_coordinates(node, x_low, y_low, cut_loc_x, y_high);

        // Do a loop from cut_loc_x to x_high and remove node from spatial lookup
        for (int x_loc = cut_loc_x + 1; x_loc <= x_high; x_loc++) {
            spatial_lookup.remove_node(node, layer, x_loc, y_low, e_rr_type::CHANX, ptc_num);
        }
    } else if (node_direction == Direction::DEC) {
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

/**
 * @brief Update a CHANX or CHANY node's bounding box in RRGraph and SpatialLookup entries if it crosses cut_loc
 * 
 * @param node Channel segment RR graph node that might cross the interposer cut line
 * @param cut_loc location of vertical interposer cut line
 * @param interposer_cut_type Type of the interposer cut line (Horizontal or vertical)
 * @param sg_node_indices Sorted list of scatter-gather node IDs. We do not want to cut these nodes as they're allowed to cross an interposer cut line.
 */
static void cut_channel_node(RRNodeId node, int cut_loc, e_interposer_cut_type interposer_cut_type, const RRGraphView& rr_graph, RRGraphBuilder& rr_graph_builder,
                             RRSpatialLookup& spatial_lookup, const std::vector<std::pair<RRNodeId, int>>& sg_node_indices) {
    constexpr auto node_indice_compare = [](RRNodeId l, RRNodeId r) noexcept { return size_t(l) < size_t(r); };
    bool is_sg_node = std::ranges::binary_search(std::views::keys(sg_node_indices), node, node_indice_compare);
    if (is_sg_node) {
        return;
    }

    int x_high = rr_graph.node_xhigh(node);
    int x_low = rr_graph.node_xlow(node);

    int y_high = rr_graph.node_yhigh(node);
    int y_low = rr_graph.node_ylow(node);

    int layer = rr_graph.node_layer_low(node);
    int ptc_num = rr_graph.node_ptc_num(node);
    Direction node_direction = rr_graph.node_direction(node);
    e_rr_type node_type = rr_graph.node_type(node);

    if (interposer_cut_type == e_interposer_cut_type::HORZ) {
        VTR_ASSERT(node_type == e_rr_type::CHANY);
        if (!should_cut(y_low, y_high, cut_loc)) {
            return;
        }
    } else if (interposer_cut_type == e_interposer_cut_type::VERT) {
        VTR_ASSERT(node_type == e_rr_type::CHANX);
        if (!should_cut(x_low, x_high, cut_loc)) {
            return;
        }
    }

    if (interposer_cut_type == e_interposer_cut_type::HORZ) {
        cut_chan_y_node(node, x_low, y_low, x_high, y_high, layer, ptc_num, cut_loc, node_direction, rr_graph_builder, spatial_lookup);
    } else if (interposer_cut_type == e_interposer_cut_type::VERT) {
        cut_chan_x_node(node, x_low, y_low, x_high, y_high, layer, ptc_num, cut_loc, node_direction, rr_graph_builder, spatial_lookup);
    }
}

void update_interposer_crossing_nodes_in_spatial_lookup_and_rr_graph_storage(const RRGraphView& rr_graph, const DeviceGrid& grid, RRGraphBuilder& rr_graph_builder,
                                                                             const std::vector<std::pair<RRNodeId, int>>& sg_node_indices) {

    VTR_ASSERT(std::is_sorted(sg_node_indices.begin(), sg_node_indices.end()));

    RRSpatialLookup& spatial_lookup = rr_graph_builder.node_lookup();
    for (size_t layer = 0; layer < grid.get_num_layers(); layer++) {
        for (int cut_loc_y : grid.get_horizontal_interposer_cuts()[layer]) {
            for (size_t x_loc = 0; x_loc < grid.width(); x_loc++) {
                std::vector<RRNodeId> channel_nodes = spatial_lookup.find_channel_nodes(layer, x_loc, cut_loc_y, e_rr_type::CHANY);
                for (RRNodeId node : channel_nodes) {
                    cut_channel_node(node, cut_loc_y, e_interposer_cut_type::HORZ,
                                     rr_graph, rr_graph_builder, spatial_lookup, sg_node_indices);
                }
            }
        }

        for (int cut_loc_x : grid.get_vertical_interposer_cuts()[layer]) {
            for (size_t y_loc = 0; y_loc < grid.height(); y_loc++) {
                std::vector<RRNodeId> channel_nodes = spatial_lookup.find_channel_nodes(layer, cut_loc_x, y_loc, e_rr_type::CHANX);
                for (RRNodeId node : channel_nodes) {
                    cut_channel_node(node, cut_loc_x, e_interposer_cut_type::VERT,
                                     rr_graph, rr_graph_builder, spatial_lookup, sg_node_indices);
                }
            }
        }
    }
}
