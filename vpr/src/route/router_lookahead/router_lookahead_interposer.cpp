#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <vector>
#include "device_grid.h"
#include "physical_types.h"
#include "route_common.h"
#include "rr_graph_fwd.h"
#include "rr_graph_view.h"
#include "rr_node_types.h"
#include "vtr_assert.h"
#include "vtr_ndmatrix.h"
#include "vtr_time.h"
#include "vtr_vector.h"
#include "router_lookahead_map_utils.h"
#include "router_lookahead_interposer.h"

static vtr::NdMatrix<float, 2> compute_interposer_delay_matrix(const DeviceGrid& grid, const RRGraphView& rr_graph);

/**
 * @brief Get all input edges of a given list of nodes. This function is expensive and works in O(n.logk)
 * with n being the total number of edges in the RR Graph and k being the number of nodes you want the input edges of.
 * 
 * @param nodes List of nodes to get input edges for. This list will be mutated and sorted inside the function.
 * 
 */
static std::unordered_map<RRNodeId, std::vector<RREdgeId>> get_nodes_in_edges(const RRGraphView& rr_graph, std::vector<RRNodeId>& nodes) {
    auto node_id_comp = [](const RRNodeId l, const RRNodeId r) noexcept { return static_cast<size_t>(l) >= static_cast<size_t>(r); };

    std::ranges::stable_sort(nodes, node_id_comp);

    std::unordered_map<RRNodeId, std::vector<RREdgeId>> node_in_edges;
    for (RREdgeId edge_id : rr_graph.all_edges()) {
        auto sink_node_it = std::ranges::lower_bound(nodes, rr_graph.edge_sink_node(edge_id), node_id_comp);
        if (sink_node_it != nodes.end()) {
            node_in_edges[*sink_node_it].push_back(edge_id);
        }
    }

    return node_in_edges;
}

InterposerDelayLookahead::InterposerDelayLookahead(const RRGraphView& rr_graph, const DeviceGrid& grid)
    : rr_graph_(rr_graph)
    , grid_(grid) {
    die_to_die_delay_matrix_ = compute_interposer_delay_matrix(grid, rr_graph);
}

float InterposerDelayLookahead::get_interposer_lookahead_delay(RRNodeId from_node, RRNodeId to_node) const {
    int from_layer = rr_graph_.node_layer_low(from_node);
    int to_layer = rr_graph_.node_layer_low(to_node);

    auto [from_x, from_y] = util::get_adjusted_rr_position(from_node);
    auto [to_x, to_y] = util::get_adjusted_rr_position(to_node);

    DeviceDieId to_die_id = grid_.get_loc_die_id({to_x, to_y, to_layer});
    DeviceDieId from_die_id = grid_.get_loc_die_id({from_x, from_y, from_layer});

    float interposer_delay = die_to_die_delay_matrix_[static_cast<size_t>(from_die_id)][static_cast<size_t>(to_die_id)];

    return interposer_delay;
}

static vtr::NdMatrix<float, 2> compute_interposer_delay_matrix(const DeviceGrid& grid, const RRGraphView& rr_graph) {
    vtr::ScopedStartFinishTimer timer("Computing interposer delay matrix");
    const auto [layer_size, x_size, y_size] = grid.dim_sizes();

    vtr::NdMatrix<float, 2> interposer_delay_matrices({grid.get_die_count(), grid.get_die_count()}, 0.0f);

    const auto& horizontal_interposer_cuts = grid.get_horizontal_interposer_cuts();
    const auto& vertical_interposer_cuts = grid.get_vertical_interposer_cuts();

    std::vector<std::vector<float>> horizontal_delay_sum(layer_size);
    std::vector<std::vector<float>> vertical_delay_sum(layer_size);

    auto get_channel_min_delay = [](const std::unordered_map<RRNodeId, std::vector<RREdgeId>>& node_in_edges) {
        float channel_delay = std::numeric_limits<float>::max();
        for (auto [node, in_edges] : node_in_edges) {
            for (RREdgeId in_edge : in_edges) {
                float node_cost = get_rr_node_delay_cost(node, in_edge);
                channel_delay = std::min(node_cost, channel_delay);
            }
        }
        VTR_ASSERT(std::isfinite(channel_delay) && !std::isnan(channel_delay));
        return channel_delay;
    };

    // Calculate the 1D delay sums
    for (size_t layer_num = 0; layer_num < layer_size; layer_num++) {
        const auto& layer_horizontal_interposer_cuts = horizontal_interposer_cuts[layer_num];
        const auto& layer_vertical_interposer_cuts = vertical_interposer_cuts[layer_num];

        horizontal_delay_sum[layer_num] = std::vector<float>(layer_vertical_interposer_cuts.size() + 1, 0);
        vertical_delay_sum[layer_num] = std::vector<float>(layer_horizontal_interposer_cuts.size() + 1, 0);

        for (size_t vertical_interposer_index = 0; vertical_interposer_index < layer_vertical_interposer_cuts.size(); vertical_interposer_index++) {
            int x_loc = layer_vertical_interposer_cuts[vertical_interposer_index];

            std::vector<RRNodeId> channel_nodes;
            for (size_t y_loc = 0; y_loc < y_size; y_loc++) {
                std::vector<RRNodeId> segment_nodes = rr_graph.node_lookup().find_channel_nodes(layer_num, x_loc, y_loc, e_rr_type::CHANX);
                channel_nodes.insert(channel_nodes.end(), segment_nodes.begin(), segment_nodes.end());
            }

            std::unordered_map<RRNodeId, std::vector<RREdgeId>> node_in_edges = get_nodes_in_edges(rr_graph, channel_nodes);
            float interposer_delay = get_channel_min_delay(node_in_edges);
            horizontal_delay_sum[layer_num][vertical_interposer_index + 1] = horizontal_delay_sum[layer_num][vertical_interposer_index] + interposer_delay;
        }

        for (size_t horizontal_interposer_index = 0; horizontal_interposer_index < layer_horizontal_interposer_cuts.size(); horizontal_interposer_index++) {
            int y_loc = layer_horizontal_interposer_cuts[horizontal_interposer_index];

            std::vector<RRNodeId> channel_nodes;
            for (size_t x_loc = 0; x_loc < y_size; x_loc++) {
                std::vector<RRNodeId> segment_nodes = rr_graph.node_lookup().find_channel_nodes(layer_num, x_loc, y_loc, e_rr_type::CHANY);
                channel_nodes.insert(channel_nodes.end(), segment_nodes.begin(), segment_nodes.end());
            }

            std::unordered_map<RRNodeId, std::vector<RREdgeId>> node_in_edges = get_nodes_in_edges(rr_graph, channel_nodes);
            float interposer_delay = get_channel_min_delay(node_in_edges);
            vertical_delay_sum[layer_num][horizontal_interposer_index + 1] = vertical_delay_sum[layer_num][horizontal_interposer_index] + interposer_delay;
        }
    }

    for (t_die_region first_die_region : grid.all_die_regions()) {
        for (t_die_region second_die_region : grid.all_die_regions()) {
            DeviceDieId first_die_id = grid.get_die_region_id(first_die_region);
            DeviceDieId second_die_id = grid.get_die_region_id(second_die_region);

            float first_horizontal_delay = horizontal_delay_sum[first_die_region.layer][first_die_region.x_die];
            float first_vertical_delay = vertical_delay_sum[first_die_region.layer][first_die_region.y_die];

            float second_horizontal_delay = horizontal_delay_sum[second_die_region.layer][second_die_region.x_die];
            float second_vertical_delay = vertical_delay_sum[second_die_region.layer][second_die_region.y_die];

            interposer_delay_matrices[(size_t)first_die_id][(size_t)second_die_id] =
                std::abs(first_horizontal_delay - second_horizontal_delay) + std::abs(first_vertical_delay - second_vertical_delay);

        }
    }

    // Delay values in Ids that are in separate layers are completely wrong btw
    return interposer_delay_matrices;
}
