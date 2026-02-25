#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <utility>
#include <vector>
#include "device_grid.h"
#include "physical_types.h"
#include "route_common.h"
#include "rr_graph_fwd.h"
#include "rr_graph_view.h"
#include "rr_node_types.h"
#include "vpr_context.h"
#include "vtr_assert.h"
#include "vtr_ndmatrix.h"
#include "vtr_time.h"
#include "vtr_vector.h"
#include "router_lookahead_map_utils.h"
#include "router_lookahead_interposer.h"

/**
 * @brief Compute the delay matrix for interposer delays. End result is a 2D matrix with an entry for each pair of DeviceDieIds.
 * Each element of the matrix represents the interposer delay of going from one die to another die.
 * For example, in a device with 2 interposer cuts with a delay of 5ns each, this function returns the following 3x3 matrix:
 *                      DeviceDieId
 *                    0      1      2
 *                 +------+------+------+
 *               0 |   0  |   5  |  10  |
 * DeviceDieId     +------+------+------+
 *               1 |   5  |   0  |   5  |
 *                 +------+------+------+
 *               2 |  10  |   5  |   0  |
 *                 +------+------+------+
 * @note Currently devices that are 3D and have interposer cuts are not supported. This function will not crash
 * but the resulting delay values will not be correct.
 */
static std::pair<vtr::NdMatrix<float, 2>, vtr::NdMatrix<float, 2>> compute_interposer_delay_matrix(const DeviceGrid& grid, const RRGraphView& rr_graph, const DeviceContext& device_ctx);

/**
 * @brief Get all input edges of a given list of nodes. This function is expensive and works in O(n.logk)
 * with n being the total number of edges in the RR Graph and k being the number of nodes you want the input edges of.
 * 
 * @param nodes List of nodes to get input edges for. This list will be mutated and sorted inside the function.
 * 
 */
static std::unordered_map<RRNodeId, std::vector<RREdgeId>> get_nodes_in_edges(const RRGraphView& rr_graph, std::vector<RRNodeId>& nodes);

InterposerLookahead::InterposerLookahead(const RRGraphView& rr_graph, const DeviceGrid& grid, const DeviceContext& device_ctx)
    : rr_graph_(rr_graph)
    , grid_(grid) {
    auto [delay_matrix, cong_matrix] = compute_interposer_delay_matrix(grid, rr_graph, device_ctx);
    die_to_die_delay_matrix_ = std::move(delay_matrix);
    die_to_die_cong_matrix_ = std::move(cong_matrix);
}

std::pair<float, float> InterposerLookahead::get_interposer_lookahead_cost(RRNodeId from_node, RRNodeId to_node) const {
    int from_layer = rr_graph_.node_layer_low(from_node);
    int to_layer = rr_graph_.node_layer_low(to_node);

    auto [from_x, from_y] = util::get_adjusted_rr_position(from_node);
    auto [to_x, to_y] = util::get_adjusted_rr_position(to_node);

    DeviceDieId to_die_id = grid_.get_loc_die_id({to_x, to_y, to_layer});
    DeviceDieId from_die_id = grid_.get_loc_die_id({from_x, from_y, from_layer});

    float interposer_delay = die_to_die_delay_matrix_[static_cast<size_t>(from_die_id)][static_cast<size_t>(to_die_id)];
    float interposer_cong = die_to_die_cong_matrix_[static_cast<size_t>(from_die_id)][static_cast<size_t>(to_die_id)];

    return {interposer_delay, interposer_cong};
}

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

static std::pair<vtr::NdMatrix<float, 2>, vtr::NdMatrix<float, 2>> compute_interposer_delay_matrix(const DeviceGrid& grid, const RRGraphView& rr_graph, const DeviceContext& device_ctx) {
    vtr::ScopedStartFinishTimer timer("Computing interposer delay lookahead");
    const auto [layer_size, x_size, y_size] = grid.dim_sizes();

    vtr::NdMatrix<float, 2> interposer_delay_matrices({grid.get_die_count(), grid.get_die_count()}, 0.0f);
    vtr::NdMatrix<float, 2> interposer_cong_matrices({grid.get_die_count(), grid.get_die_count()}, 0.0f);

    const auto& horizontal_interposer_cuts = grid.get_horizontal_interposer_cuts();
    const auto& vertical_interposer_cuts = grid.get_vertical_interposer_cuts();

    std::vector<std::vector<float>> horizontal_delay_sum(layer_size);
    std::vector<std::vector<float>> vertical_delay_sum(layer_size);

    std::vector<std::vector<float>> horizontal_cong_sum(layer_size);
    std::vector<std::vector<float>> vertical_cong_sum(layer_size);

    auto get_channel_min_delay = [&rr_graph, &device_ctx](const std::unordered_map<RRNodeId, std::vector<RREdgeId>>& node_in_edges) {
        float channel_delay = std::numeric_limits<float>::max();
        float channel_cong_cost = std::numeric_limits<float>::max();

        for (auto [node, in_edges] : node_in_edges) {
            for (RREdgeId in_edge : in_edges) {
                float node_delay = get_rr_node_delay_cost(node, in_edge);
                float node_cong_const = device_ctx.rr_indexed_data[rr_graph.node_cost_index(node)].base_cost;

                channel_delay = std::min(node_delay, channel_delay);
                channel_cong_cost = std::min(node_cong_const, channel_cong_cost);
            }
        }
        VTR_ASSERT(std::isfinite(channel_delay) && !std::isnan(channel_delay));
        return std::pair<float, float>{channel_delay, channel_cong_cost};
    };

    // Calculate the 1D delay sums
    for (size_t layer_num = 0; layer_num < layer_size; layer_num++) {
        const auto& layer_horizontal_interposer_cuts = horizontal_interposer_cuts[layer_num];
        const auto& layer_vertical_interposer_cuts = vertical_interposer_cuts[layer_num];

        horizontal_delay_sum[layer_num] = std::vector<float>(layer_vertical_interposer_cuts.size() + 1, 0);
        vertical_delay_sum[layer_num] = std::vector<float>(layer_horizontal_interposer_cuts.size() + 1, 0);

        horizontal_cong_sum[layer_num] = std::vector<float>(layer_vertical_interposer_cuts.size() + 1, 0);
        vertical_cong_sum[layer_num] = std::vector<float>(layer_horizontal_interposer_cuts.size() + 1, 0);

        for (size_t vertical_interposer_index = 0; vertical_interposer_index < layer_vertical_interposer_cuts.size(); vertical_interposer_index++) {
            int x_loc = layer_vertical_interposer_cuts[vertical_interposer_index];

            std::vector<RRNodeId> channel_nodes;
            for (size_t y_loc = 0; y_loc < y_size; y_loc++) {
                std::vector<RRNodeId> segment_nodes = rr_graph.node_lookup().find_channel_nodes(layer_num, x_loc, y_loc, e_rr_type::CHANX);
                channel_nodes.insert(channel_nodes.end(), segment_nodes.begin(), segment_nodes.end());
            }

            std::unordered_map<RRNodeId, std::vector<RREdgeId>> node_in_edges = get_nodes_in_edges(rr_graph, channel_nodes);
            auto [interposer_delay, interposer_cong_cost] = get_channel_min_delay(node_in_edges);
            horizontal_delay_sum[layer_num][vertical_interposer_index + 1] = horizontal_delay_sum[layer_num][vertical_interposer_index] + interposer_delay;
            horizontal_cong_sum[layer_num][vertical_interposer_index + 1] = horizontal_cong_sum[layer_num][vertical_interposer_index] + interposer_cong_cost;
        }

        for (size_t horizontal_interposer_index = 0; horizontal_interposer_index < layer_horizontal_interposer_cuts.size(); horizontal_interposer_index++) {
            int y_loc = layer_horizontal_interposer_cuts[horizontal_interposer_index];

            std::vector<RRNodeId> channel_nodes;
            for (size_t x_loc = 0; x_loc < y_size; x_loc++) {
                std::vector<RRNodeId> segment_nodes = rr_graph.node_lookup().find_channel_nodes(layer_num, x_loc, y_loc, e_rr_type::CHANY);
                channel_nodes.insert(channel_nodes.end(), segment_nodes.begin(), segment_nodes.end());
            }

            std::unordered_map<RRNodeId, std::vector<RREdgeId>> node_in_edges = get_nodes_in_edges(rr_graph, channel_nodes);
            auto [interposer_delay, interposer_cong_cost] = get_channel_min_delay(node_in_edges);
            vertical_delay_sum[layer_num][horizontal_interposer_index + 1] = vertical_delay_sum[layer_num][horizontal_interposer_index] + interposer_delay;
            vertical_cong_sum[layer_num][horizontal_interposer_index + 1] = vertical_cong_sum[layer_num][horizontal_interposer_index] + interposer_cong_cost;
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


            float first_horizontal_cong = horizontal_cong_sum[first_die_region.layer][first_die_region.x_die];
            float first_vertical_cong = vertical_cong_sum[first_die_region.layer][first_die_region.y_die];

            float second_horizontal_cong = horizontal_cong_sum[second_die_region.layer][second_die_region.x_die];
            float second_vertical_cong = vertical_cong_sum[second_die_region.layer][second_die_region.y_die];

            interposer_cong_matrices[(size_t)first_die_id][(size_t)second_die_id] =
                2 * (std::abs(first_horizontal_cong - second_horizontal_cong) + std::abs(first_vertical_cong - second_vertical_cong));
        }
    }

    return {std::move(interposer_delay_matrices), std::move(interposer_cong_matrices)};
}
