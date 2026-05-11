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
 * @brief Internal struct used for returning the delay and congestion cost data of interposer cuts in helper functions.
 * 
 */
struct t_interposer_cost_prefix_sums {
    t_interposer_cost_prefix_sums(int num_layers) {
        horizontal_delay_sum = std::vector<std::vector<float>>(num_layers);
        horizontal_cong_sum = std::vector<std::vector<float>>(num_layers);

        vertical_delay_sum = std::vector<std::vector<float>>(num_layers);
        vertical_cong_sum = std::vector<std::vector<float>>(num_layers);
    }

    std::vector<std::vector<float>> horizontal_delay_sum;
    std::vector<std::vector<float>> vertical_delay_sum;

    std::vector<std::vector<float>> horizontal_cong_sum;
    std::vector<std::vector<float>> vertical_cong_sum;
};

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
static std::pair<vtr::NdMatrix<float, 2>, vtr::NdMatrix<float, 2>> compute_interposer_delay_matrix(const DeviceGrid& grid,
                                                                                                   const RRGraphView& rr_graph,
                                                                                                   const DeviceContext& device_ctx,
                                                                                                   float interposer_cut_base_cost_multiplier);
/**
 * @brief Computes prefix sums for delay and congestion cost of crossing interposer cuts
 * 
 * @return t_interposer_cost_prefix_sums Struct containing 4 prefix sums, for delay and congestion cost of horizontal and vertical cuts.
 */
static t_interposer_cost_prefix_sums compute_die_crossing_prefix_sums(const DeviceGrid& grid,
                                                                      const RRGraphView& rr_graph,
                                                                      const DeviceContext& device_ctx);
/**
 * @brief Get all input edges of a given list of nodes. This function is expensive and works in O(n.logk)
 * with n being the total number of edges in the RR Graph and k being the number of nodes you want the input edges of.
 * 
 * @param nodes List of nodes to get input edges for. This list will be mutated and sorted inside the function.
 */
static std::unordered_map<RRNodeId, std::vector<RREdgeId>> get_nodes_in_edges(const RRGraphView& rr_graph, std::vector<RRNodeId>& nodes);

InterposerLookahead::InterposerLookahead(const RRGraphView& rr_graph, const DeviceGrid& grid, const DeviceContext& device_ctx, float interposer_cut_base_cost_multiplier)
    : rr_graph_(rr_graph)
    , grid_(grid) {
    auto [delay_matrix, cong_matrix] = compute_interposer_delay_matrix(grid, rr_graph, device_ctx, interposer_cut_base_cost_multiplier);
    die_to_die_delay_matrix_ = std::move(delay_matrix);
    die_to_die_cong_matrix_ = std::move(cong_matrix);
}

std::pair<float, float> InterposerLookahead::get_interposer_lookahead_cost(RRNodeId from_node, RRNodeId to_node) const {
    int from_layer = rr_graph_.node_layer_low(from_node);
    int to_layer = rr_graph_.node_layer_low(to_node);

    auto [from_x, from_y] = util::get_adjusted_rr_position(from_node);
    auto [to_x, to_y] = util::get_adjusted_rr_position(to_node);

    return get_interposer_lookahead_cost({from_x, from_y, from_layer}, {to_x, to_y, to_layer});
}

std::pair<float, float> InterposerLookahead::get_interposer_lookahead_cost(t_physical_tile_loc from_loc, t_physical_tile_loc to_loc) const {
    size_t from_die_index = static_cast<size_t>(grid_.get_loc_die_id(from_loc));
    size_t to_die_index = static_cast<size_t>(grid_.get_loc_die_id(to_loc));

    float interposer_delay = die_to_die_delay_matrix_[from_die_index][to_die_index];
    float interposer_cong = die_to_die_cong_matrix_[from_die_index][to_die_index];

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

static t_interposer_cost_prefix_sums compute_die_crossing_prefix_sums(const DeviceGrid& grid,
                                                                      const RRGraphView& rr_graph,
                                                                      const DeviceContext& device_ctx) {

    const auto [layer_size, device_width, device_height] = grid.dim_sizes();
    const std::vector<std::vector<int>>& horizontal_interposer_cuts = grid.get_horizontal_interposer_cuts();
    const std::vector<std::vector<int>>& vertical_interposer_cuts = grid.get_vertical_interposer_cuts();

    /**
     * @brief Gets the minimum delay and congestion cost of a list of nodes along with their input edges.
     * Used here to calculate the cost of crossing an interposer cut.
     */
    auto get_channel_min_delay = [&rr_graph, &device_ctx](const std::unordered_map<RRNodeId, std::vector<RREdgeId>>& node_in_edges) {
        float channel_delay = std::numeric_limits<float>::max();
        float channel_cong_cost = std::numeric_limits<float>::max();

        for (auto& [node, in_edges] : node_in_edges) {
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

    // Compute prefix sums for interposer delays and congestion costs along each axis.
    // The cost of each cut is the minimum cost of all nodes that cross the cut.
    // By calculating the cumulative cost at each cut, we can determine the cost
    // between any two regions in O(1) time using the difference between their
    // prefix sum values.
    t_interposer_cost_prefix_sums interposer_sums(layer_size);

    for (size_t layer_num = 0; layer_num < layer_size; layer_num++) {
        const std::vector<int>& layer_horizontal_interposer_cuts = horizontal_interposer_cuts[layer_num];
        const std::vector<int>& layer_vertical_interposer_cuts = vertical_interposer_cuts[layer_num];

        interposer_sums.horizontal_delay_sum[layer_num] = std::vector<float>(layer_vertical_interposer_cuts.size() + 1, 0);
        interposer_sums.vertical_delay_sum[layer_num] = std::vector<float>(layer_horizontal_interposer_cuts.size() + 1, 0);

        interposer_sums.horizontal_cong_sum[layer_num] = std::vector<float>(layer_vertical_interposer_cuts.size() + 1, 0);
        interposer_sums.vertical_cong_sum[layer_num] = std::vector<float>(layer_horizontal_interposer_cuts.size() + 1, 0);

        // Calculate the vertical delay and congestion cost prefix sums
        for (size_t vertical_interposer_index = 0; vertical_interposer_index < layer_vertical_interposer_cuts.size(); vertical_interposer_index++) {
            int x_loc = layer_vertical_interposer_cuts[vertical_interposer_index];

            std::vector<RRNodeId> channel_nodes;
            for (size_t y_loc = 0; y_loc < device_height; y_loc++) {
                std::vector<RRNodeId> segment_nodes = rr_graph.node_lookup().find_channel_nodes(layer_num, x_loc, y_loc, e_rr_type::CHANX);
                channel_nodes.insert(channel_nodes.end(), segment_nodes.begin(), segment_nodes.end());
            }

            std::unordered_map<RRNodeId, std::vector<RREdgeId>> node_in_edges = get_nodes_in_edges(rr_graph, channel_nodes);
            auto [interposer_delay, interposer_cong_cost] = get_channel_min_delay(node_in_edges);
            interposer_sums.horizontal_delay_sum[layer_num][vertical_interposer_index + 1] = interposer_sums.horizontal_delay_sum[layer_num][vertical_interposer_index] + interposer_delay;
            interposer_sums.horizontal_cong_sum[layer_num][vertical_interposer_index + 1] = interposer_sums.horizontal_cong_sum[layer_num][vertical_interposer_index] + interposer_cong_cost;
        }

        // Calculate the horizontal delay and congestion cost prefix sums
        for (size_t horizontal_interposer_index = 0; horizontal_interposer_index < layer_horizontal_interposer_cuts.size(); horizontal_interposer_index++) {
            int y_loc = layer_horizontal_interposer_cuts[horizontal_interposer_index];

            std::vector<RRNodeId> channel_nodes;
            for (size_t x_loc = 0; x_loc < device_height; x_loc++) {
                std::vector<RRNodeId> segment_nodes = rr_graph.node_lookup().find_channel_nodes(layer_num, x_loc, y_loc, e_rr_type::CHANY);
                channel_nodes.insert(channel_nodes.end(), segment_nodes.begin(), segment_nodes.end());
            }

            std::unordered_map<RRNodeId, std::vector<RREdgeId>> node_in_edges = get_nodes_in_edges(rr_graph, channel_nodes);
            auto [interposer_delay, interposer_cong_cost] = get_channel_min_delay(node_in_edges);
            interposer_sums.vertical_delay_sum[layer_num][horizontal_interposer_index + 1] = interposer_sums.vertical_delay_sum[layer_num][horizontal_interposer_index] + interposer_delay;
            interposer_sums.vertical_cong_sum[layer_num][horizontal_interposer_index + 1] = interposer_sums.vertical_cong_sum[layer_num][horizontal_interposer_index] + interposer_cong_cost;
        }
    }

    return interposer_sums;
}

static std::pair<vtr::NdMatrix<float, 2>, vtr::NdMatrix<float, 2>> compute_interposer_delay_matrix(const DeviceGrid& grid,
                                                                                                   const RRGraphView& rr_graph,
                                                                                                   const DeviceContext& device_ctx,
                                                                                                   float interposer_cut_base_cost_multiplier) {
    vtr::ScopedStartFinishTimer timer("Computing interposer lookahead");

    t_interposer_cost_prefix_sums interposer_sums = compute_die_crossing_prefix_sums(grid, rr_graph, device_ctx);

    vtr::NdMatrix<float, 2> interposer_delay_matrix({grid.get_die_count(), grid.get_die_count()}, 0.0f);
    vtr::NdMatrix<float, 2> interposer_cong_matrix({grid.get_die_count(), grid.get_die_count()}, 0.0f);

    // Lambda to compute the die-crossing cost between two die regions based on horizontal and vertical cost prefix sums.
    auto get_die_crossing_cost = [](const std::vector<std::vector<float>>& horiz_sums,
                                    const std::vector<std::vector<float>>& vert_sums,
                                    const t_die_region& src,
                                    const t_die_region& dest) {
        float h_dist = std::abs(horiz_sums[src.layer][src.x_die] - horiz_sums[dest.layer][dest.x_die]);
        float v_dist = std::abs(vert_sums[src.layer][src.y_die] - vert_sums[dest.layer][dest.y_die]);
        return h_dist + v_dist;
    };

    // Calculate the final 2D die-to-die delay and congestion cost map using the prefix sums
    for (t_die_region first_die_region : grid.all_die_regions()) {
        for (t_die_region second_die_region : grid.all_die_regions()) {
            DeviceDieId first_die_id = grid.get_die_region_id(first_die_region);
            DeviceDieId second_die_id = grid.get_die_region_id(second_die_region);

            interposer_delay_matrix[(size_t)first_die_id][(size_t)second_die_id] =
                get_die_crossing_cost(interposer_sums.horizontal_delay_sum, interposer_sums.vertical_delay_sum, first_die_region, second_die_region);

            interposer_cong_matrix[(size_t)first_die_id][(size_t)second_die_id] =
                interposer_cut_base_cost_multiplier * get_die_crossing_cost(interposer_sums.horizontal_cong_sum, interposer_sums.vertical_cong_sum, first_die_region, second_die_region);
        }
    }

    return {std::move(interposer_delay_matrix), std::move(interposer_cong_matrix)};
}
