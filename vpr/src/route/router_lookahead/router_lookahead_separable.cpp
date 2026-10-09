#include "router_lookahead_separable.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <ranges>
#include <vector>
#include "connection_router_interface.h"
#include "device_grid.h"
#include "globals.h"
#include "router_lookahead_map.h"
#include "router_lookahead_map_utils.h"
#include "router_lookahead_sampling_utils.h"
#include "rr_graph_builder.h"
#include "rr_graph_view.h"
#include "rr_node_types.h"
#include "rr_spatial_lookup.h"
#include "vpr_context.h"
#include "vpr_error.h"
#include "vtr_time.h"

/**
 * @brief Find the first channel node with the requested segment and exact direction.
 * Unidirectional wires must be driven at the sample position; BIDIR wires may span it.
 */
static RRNodeId get_chanxy_start_node_sep(int layer, int start_x, int start_y, Direction direction, e_rr_type rr_type, int seg_index) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;
    const RRSpatialLookup& node_lookup = rr_graph.node_lookup();

    VTR_ASSERT(rr_type == e_rr_type::CHANX || rr_type == e_rr_type::CHANY);

    for (const RRNodeId node_id : node_lookup.find_channel_nodes(layer, start_x, start_y, rr_type)) {
        VTR_ASSERT(rr_graph.node_type(node_id) == rr_type);

        Direction node_direction = rr_graph.node_direction(node_id);
        RRIndexedDataId node_cost_ind = rr_graph.node_cost_index(node_id);
        int node_seg_ind = device_ctx.rr_indexed_data[node_cost_ind].seg_index;
        auto [driver_x, driver_y] = util::get_adjusted_rr_position(node_id);
        if (node_direction == direction && node_seg_ind == seg_index
            && (driver_x == start_x && driver_y == start_y)) {
            return node_id;
        }
    }

    return RRNodeId::INVALID();
}

/// @brief The device axis (x or y) whose travel cost a separable wire cost map profiles.
enum class e_profile_axis {
    X, ///< Profiles horizontal travel; populates an x_wire_cost_map (keyed by x1/x2).
    Y  ///< Profiles vertical travel; populates a y_wire_cost_map (keyed by y1/y2).
};

/**
 * @brief Profile wire costs along one axis using Dijkstra floods in a five-tile-wide strip.
 * The sample row is at two-thirds of the device height for x; the sample column is
 * at half the device width for y. Each flood spans the full profiling axis.
 */
static void compute_wire_cost_map_for_axis(const std::vector<t_segment_inf>& segment_infs,
                                           e_profile_axis axis,
                                           vtr::NdMatrix<util::Cost_Entry, 7>& wire_cost_map) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const DeviceGrid& grid = device_ctx.grid;
    const RRGraphView& rr_graph = device_ctx.rr_graph;

    const bool profile_x = (axis == e_profile_axis::X);

    const size_t num_layers = grid.get_num_layers();
    // The wire look-ahead table has a dimension for channel type (CHANX/CHANY/CHANZ). In 2-d
    // architectures there is no CHANZ, so that dimension is dropped down to CHANX/CHANY only.
    const size_t chan_type_dim_size = (num_layers == 1) ? 2 : 3;
    // INC/DEC/BIDIR
    constexpr size_t direction_dim_size = 3;

    // Size of the profiling-axis dimension: device width when profiling x, height when profiling y.
    const size_t axis_dim_size = profile_x ? grid.width() : grid.height();

    wire_cost_map.resize({num_layers, num_layers, chan_type_dim_size, segment_infs.size(),
                          direction_dim_size, axis_dim_size, axis_dim_size});

    // TODO: Don't hardcode the fixed coord location, instead take the device edges + interposer cut lines into account
    // And pick a location far from them.
    const int fixed_coord = profile_x ? (int)(2 * grid.height() / 3) : (int)grid.width() / 2;

    util::t_dijkstra_data dijkstra_data;

    for (size_t from_layer_num = 0; from_layer_num < num_layers; from_layer_num++) {
        // Skip layers without inter-cluster routing resources.
        if (!device_ctx.inter_cluster_prog_routing_resources[from_layer_num]) {
            continue;
        }

        // Restrict the Dijkstra flood to a few rows/columns around the sample line (to keep the flood
        // cheap), but the full extent of the profiling axis.
        t_bb bb;

        // We make the boundinb box span 2 rows/columns away (on each side) from the fixed location.
        constexpr int SAMPLING_SPAN = 2;
        if (profile_x) {
            bb = t_bb(0, grid.width() - 1,
                      std::max(0, fixed_coord - SAMPLING_SPAN), std::min<int>(grid.height() - 1, fixed_coord + SAMPLING_SPAN),
                      (int)from_layer_num, (int)from_layer_num);
        } else {
            bb = t_bb(std::max(0, fixed_coord - SAMPLING_SPAN), std::min<int>(grid.width() - 1, fixed_coord + SAMPLING_SPAN),
                      0, grid.height() - 1,
                      (int)from_layer_num, (int)from_layer_num);
        }

        // Pick sample points from each segment type e.g. L4, L16
        for (const t_segment_inf& segment_inf : segment_infs) {

            // We need to figure out what channel types this segment can be found in
            std::vector<e_rr_type> chan_types;
            if (segment_inf.parallel_axis == e_parallel_axis::X_AXIS) {
                // Only CHANX
                chan_types.push_back(e_rr_type::CHANX);
            } else if (segment_inf.parallel_axis == e_parallel_axis::Y_AXIS) {
                // Only CHANY
                chan_types.push_back(e_rr_type::CHANY);
            } else if (segment_inf.parallel_axis == e_parallel_axis::Z_AXIS) {
                // Only CHANZ
                chan_types.push_back(e_rr_type::CHANZ);
            } else {
                // Both CHANY and CHANZ
                VTR_ASSERT(segment_inf.parallel_axis == e_parallel_axis::BOTH_AXIS);
                // Both for BOTH_AXIS segments and special segments such as clock_networks we want to search in both directions.
                chan_types.insert(chan_types.end(), {e_rr_type::CHANX, e_rr_type::CHANY});
            }

            // Picsk sample points from each channel type e.g. CHANX, CHANY, CHANZ
            for (e_rr_type chan_type : chan_types) {
                const int chan_index = util::chan_type_to_index(chan_type);

                for (Direction direction : {Direction::INC, Direction::DEC, Direction::BIDIR}) {
                    const int direction_index = static_cast<int>(direction);

                    // Sample each position along the profiling axis.
                    for (int coord = 1; coord < (int)axis_dim_size; coord++) {
                        const int sample_x = profile_x ? coord : fixed_coord;
                        const int sample_y = profile_x ? fixed_coord : coord;
                        RRNodeId sample_node;
                        if (is_chanxy(chan_type)) {
                            sample_node = get_chanxy_start_node_sep(from_layer_num, sample_x, sample_y,
                                                                    direction, chan_type, segment_inf.seg_index);
                        } else {
                            VTR_ASSERT(is_chanz(chan_type));
                            sample_node = util::get_chanz_start_node(sample_x, sample_y, segment_inf.seg_index, 0, direction);
                        }

                        if (!sample_node) {
                            continue;
                        }

                        // Use the driver end of the wire along the profiling axis as the start coordinate.
                        // bidirectional wires would use the midpoint instead
                        int start_coord;
                        auto [start_x, start_y] = util::get_adjusted_rr_position(sample_node);
                        if (profile_x) {
                            start_coord = start_x;
                        } else {
                            start_coord = start_y;
                        }

                        auto record_cost = [&](util::PQ_Entry current) {
                            RRNodeId curr_node = current.rr_node;

                            // If we're profiling the y axis, we want to remove the delay of going from a channel node to an IPIN
                            // to avoid double counting this delay when we sum up the x delay and y delay during routing
                            if (!profile_x) {
                                RRIndexedDataId cost_index = rr_graph.node_cost_index(curr_node);
                                current.delay -= device_ctx.rr_indexed_data[cost_index].T_linear;
                                current.congestion_upstream -= device_ctx.rr_indexed_data[cost_index].base_cost;
                            }
                            auto [ipin_x, ipin_y] = util::get_adjusted_rr_position(curr_node);
                            const int ipin_coord = profile_x ? ipin_x : ipin_y;
                            int ipin_layer = rr_graph.node_layer_low(curr_node);

                            util::Cost_Entry& cost_entry = wire_cost_map[from_layer_num][ipin_layer][chan_index][segment_inf.seg_index][direction_index][start_coord][ipin_coord];
                            if (!cost_entry.valid() || current.delay < cost_entry.delay) {
                                cost_entry = util::Cost_Entry(current.delay, current.congestion_upstream);
                            }
                        };

                        run_dijkstra<true>(sample_node, dijkstra_data, bb, record_cost);
                    }
                }
            }
        }
    }
}

/// @brief A wire type reachable from the SOURCEs/OPINs of a tile type.
struct t_opin_reachable_wire {
    int layer;          ///< Layer of the wire.
    int chan_index;     ///< Channel type index, as returned by util::chan_type_to_index().
    int seg_index;      ///< Segment type of the wire.
    float access_delay; ///< Minimum delay from any SOURCE/OPIN of the tile to the wire.
};

/**
 * @brief Collect the distinct wire types reachable from any SOURCE/OPIN of a tile type on a layer,
 *        keeping the minimum access delay to each.
 *
 * Direct connections (SINK) and channel types outside the wire cost map are skipped since they have no profiled wire cost.
 */
static std::vector<t_opin_reachable_wire> get_opin_reachable_wires(const util::t_src_opin_delays& src_opin_delays,
                                                                   const vtr::NdMatrix<util::Cost_Entry, 7>& wire_cost_map,
                                                                   int physical_tile_idx,
                                                                   int from_layer) {
    // Indexed by wire layer, channel index and segment index; infinity marks an unreachable wire type.
    vtr::NdMatrix<float, 3> min_access_delay({wire_cost_map.dim_size(0), wire_cost_map.dim_size(2), wire_cost_map.dim_size(3)},
                                             std::numeric_limits<float>::infinity());

    for (const auto& opin_delay_maps : src_opin_delays[from_layer][physical_tile_idx]) {
        for (const auto& layer_opin_delay_map : opin_delay_maps) {
            for (const util::t_reachable_wire_inf& reachable_wire_inf : layer_opin_delay_map | std::views::values) {
                if (reachable_wire_inf.wire_rr_type == e_rr_type::SINK) {
                    continue;
                }

                // CHANZ has no entry in the wire cost map of a single-layer device.
                const int chan_index = util::chan_type_to_index(reachable_wire_inf.wire_rr_type);
                if (chan_index >= (int)wire_cost_map.dim_size(2)) {
                    continue;
                }

                VTR_ASSERT_SAFE(reachable_wire_inf.layer_number < (int)min_access_delay.dim_size(0));
                VTR_ASSERT_SAFE(reachable_wire_inf.wire_seg_index < (int)min_access_delay.dim_size(2));
                float& access_delay = min_access_delay[reachable_wire_inf.layer_number][chan_index][reachable_wire_inf.wire_seg_index];
                access_delay = std::min(access_delay, reachable_wire_inf.delay);
            }
        }
    }

    std::vector<t_opin_reachable_wire> reachable_wires;
    for (size_t layer = 0; layer < min_access_delay.dim_size(0); layer++) {
        for (size_t chan_index = 0; chan_index < min_access_delay.dim_size(1); chan_index++) {
            for (size_t seg_index = 0; seg_index < min_access_delay.dim_size(2); seg_index++) {
                const float access_delay = min_access_delay[layer][chan_index][seg_index];
                if (std::isfinite(access_delay)) {
                    reachable_wires.push_back({(int)layer, (int)chan_index, (int)seg_index, access_delay});
                }
            }
        }
    }

    return reachable_wires;
}

/**
 * @brief Minimum OPIN delay between two coordinates along one axis, over all wires reachable from the tile's OPINs.
 *
 * Reachable wires lack direction information, so minimize over all directions.
 * Include OPIN access delay only for x to count it once when summing the axes.
 * Fall back to MapLookahead when no reachable wire has a usable profiled cost; that estimate
 * includes OPIN access delay on either axis.
 */
static float min_opin_axis_delay(const std::vector<t_opin_reachable_wire>& reachable_wires,
                                 const vtr::NdMatrix<util::Cost_Entry, 7>& wire_cost_map,
                                 e_profile_axis axis,
                                 const RouterLookahead& map_lookahead,
                                 int physical_tile_idx,
                                 int from_layer,
                                 int to_layer,
                                 int c1,
                                 int c2) {
    const bool profile_x = (axis == e_profile_axis::X);

    // An entry is unusable if it was never profiled (NaN), or profiled as unreachable.
    auto is_usable = [](float delay) {
        return std::isfinite(delay) && delay != ROUTER_LOOKAHEAD_NO_PATH_SENTINEL;
    };

    float min_delay = std::numeric_limits<float>::infinity();
    for (const t_opin_reachable_wire& wire : reachable_wires) {
        const float access_delay = profile_x ? wire.access_delay : 0.f;

        for (size_t dir_index = 0; dir_index < wire_cost_map.dim_size(4); dir_index++) {
            const float wire_delay = wire_cost_map[wire.layer][to_layer][wire.chan_index][wire.seg_index][dir_index][c1][c2].delay;
            if (is_usable(wire_delay)) {
                min_delay = std::min(min_delay, access_delay + wire_delay);
            }
        }
    }

    if (!is_usable(min_delay)) {
        // Query zero distance along the other axis.
        if (profile_x) {
            min_delay = map_lookahead.get_opin_distance_min_delay(physical_tile_idx, from_layer, to_layer, c1, c2, 0, 0);
        } else {
            min_delay = map_lookahead.get_opin_distance_min_delay(physical_tile_idx, from_layer, to_layer, 0, 0, c1, c2);
        }
    }

    // Keep unreachable pairs finite for placement cost calculations.
    if (!std::isfinite(min_delay)) {
        min_delay = ROUTER_LOOKAHEAD_NO_PATH_SENTINEL;
    }

    return min_delay;
}

/** @brief Precompute per-axis OPIN delays for every tile type, layer pair and coordinate pair. */
static void min_opin_axis_delay_map(const util::t_src_opin_delays& src_opin_delays,
                                    const vtr::NdMatrix<util::Cost_Entry, 7>& wire_cost_map,
                                    e_profile_axis axis,
                                    const RouterLookahead& map_lookahead,
                                    vtr::NdMatrix<float, 5>& axis_min_delay) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const int num_tile_types = (int)device_ctx.physical_tile_types.size();
    const int num_layers = device_ctx.grid.get_num_layers();
    // The wire cost map is square in its last two (coordinate) dimensions.
    const int axis_dim_size = (int)wire_cost_map.dim_size(5);

    axis_min_delay.resize({static_cast<size_t>(num_tile_types),
                           static_cast<size_t>(num_layers),
                           static_cast<size_t>(num_layers),
                           static_cast<size_t>(axis_dim_size),
                           static_cast<size_t>(axis_dim_size)});

    for (int tile_type_idx = 0; tile_type_idx < num_tile_types; tile_type_idx++) {
        for (int from_layer_num = 0; from_layer_num < num_layers; from_layer_num++) {
            const std::vector<t_opin_reachable_wire> reachable_wires = get_opin_reachable_wires(src_opin_delays, wire_cost_map, tile_type_idx, from_layer_num);

            for (int to_layer_num = 0; to_layer_num < num_layers; to_layer_num++) {
                for (int c1 = 0; c1 < axis_dim_size; c1++) {
                    for (int c2 = 0; c2 < axis_dim_size; c2++) {
                        axis_min_delay[tile_type_idx][from_layer_num][to_layer_num][c1][c2] =
                            min_opin_axis_delay(reachable_wires, wire_cost_map, axis, map_lookahead,
                                                tile_type_idx, from_layer_num, to_layer_num, c1, c2);
                    }
                }
            }
        }
    }
}

SeparableLookahead::SeparableLookahead(const t_det_routing_arch& det_routing_arch, bool is_flat, int route_verbosity, bool device_model_warnings, float interposer_base_cost_multiplier)
    : map_lookahead_(std::make_unique<MapLookahead>(det_routing_arch, is_flat, route_verbosity, device_model_warnings, interposer_base_cost_multiplier))
    , is_flat_(is_flat)
    , route_verbosity_(route_verbosity)
    , device_model_warnings_(device_model_warnings) {
    if (is_flat_) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "SeparableLookahead does not support flat routing");
    }
}

float SeparableLookahead::get_expected_cost(RRNodeId current_node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const {
    VTR_ASSERT_SAFE(g_vpr_ctx.device().rr_graph.node_type(target_node) == e_rr_type::SINK);
    auto [delay_cost, cong_cost] = get_expected_delay_and_cong(current_node, target_node, params, R_upstream);
    return delay_cost + cong_cost;
}

std::pair<float, float> SeparableLookahead::get_expected_delay_and_cong(RRNodeId from_node, RRNodeId to_node, const t_conn_cost_params& params, float R_upstream) const {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;

    e_rr_type from_type = rr_graph.node_type(from_node);
    if (from_type == e_rr_type::SOURCE || from_type == e_rr_type::OPIN) {
        // TODO: remove dependency on map lookahead
        return map_lookahead_->get_expected_delay_and_cong(from_node, to_node, params, R_upstream);
    }
    if (from_type == e_rr_type::IPIN) {
        return {0.f, device_ctx.rr_indexed_data[RRIndexedDataId(SINK_COST_INDEX)].base_cost};
    }
    if (!is_chanxy(from_type) && !is_chanz(from_type)) {
        // In case of SINK nodes or MUX nodes, return zero
        return {0.f, 0.f};
    }

    int from_layer_num = rr_graph.node_layer_low(from_node);
    int to_layer_num = rr_graph.node_layer_low(to_node);
    Direction from_dir = rr_graph.node_direction(from_node);

    // CHANZ drives from its destination layer; BIDIR uses the endpoint closest to the target.
    // We previously set from_layer to layer_low, which is correct for DEC CHANZ nodes. Correct for other caes:
    if (from_type == e_rr_type::CHANZ) {
        if (from_dir == Direction::INC) {
            from_layer_num = rr_graph.node_layer_high(from_node);
        } else if (from_dir == Direction::BIDIR) {
            int high_layer = rr_graph.node_layer_high(from_node);
            if (std::abs(high_layer - to_layer_num) < std::abs(from_layer_num - to_layer_num)) {
                from_layer_num = high_layer;
            }
        }
    }

    RRIndexedDataId from_cost_index = rr_graph.node_cost_index(from_node);
    int from_seg_index = device_ctx.rr_indexed_data[from_cost_index].seg_index;
    VTR_ASSERT(from_seg_index >= 0);

    const int chan_index = util::chan_type_to_index(from_type);
    const int dir_index = static_cast<int>(from_dir);

    auto [from_x, from_y] = util::get_adjusted_rr_position(from_node);
    auto [to_x, to_y] = util::get_adjusted_rr_position(to_node);

    const util::Cost_Entry& x_cost = x_wire_cost_map_[from_layer_num][to_layer_num][chan_index][from_seg_index][dir_index][from_x][to_x];
    const util::Cost_Entry& y_cost = y_wire_cost_map_[from_layer_num][to_layer_num][chan_index][from_seg_index][dir_index][from_y][to_y];

    float expected_delay_cost = x_cost.delay + y_cost.delay;
    float expected_cong_cost = x_cost.congestion + y_cost.congestion;

    // Use the map lookahead when the combined delay or congestion cost is unusable.
    if (!std::isfinite(expected_delay_cost) || !std::isfinite(expected_cong_cost)
        || expected_delay_cost == ROUTER_LOOKAHEAD_NO_PATH_SENTINEL
        || expected_cong_cost == ROUTER_LOOKAHEAD_NO_PATH_SENTINEL) {
        // TODO: remove dependency on map lookahead
        return map_lookahead_->get_expected_delay_and_cong(from_node, to_node, params, R_upstream);
    }
    expected_delay_cost *= params.criticality;
    expected_cong_cost *= (1.0f - params.criticality);

    return {expected_delay_cost, expected_cong_cost};
}

void SeparableLookahead::compute(const std::vector<t_segment_inf>& segment_inf) {
    vtr::ScopedStartFinishTimer timer("Computing router lookahead separable");

    compute_wire_cost_map_for_axis(segment_inf, e_profile_axis::X, x_wire_cost_map_);
    compute_wire_cost_map_for_axis(segment_inf, e_profile_axis::Y, y_wire_cost_map_);

    const util::t_src_opin_delays src_opin_delays = util::compute_router_src_opin_lookahead(is_flat_, route_verbosity_, device_model_warnings_);

    // TODO: remove dependency on map lookahead
    // min_opin_axis_delay_map uses the map lookahead when a separable cost is unavailable.
    map_lookahead_->compute(segment_inf);

    min_opin_axis_delay_map(src_opin_delays, x_wire_cost_map_, e_profile_axis::X, *map_lookahead_, opin_x_min_delay_);
    min_opin_axis_delay_map(src_opin_delays, y_wire_cost_map_, e_profile_axis::Y, *map_lookahead_, opin_y_min_delay_);
}

void SeparableLookahead::compute_intra_tile() {
    VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "SeparableLookahead::compute_intra_tile is not implemented yet");
}

void SeparableLookahead::read(const std::string& /*file*/) {
    VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "SeparableLookahead::read is not implemented yet");
}

void SeparableLookahead::read_intra_cluster(const std::string& /*file*/) {
    VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "SeparableLookahead::read_intra_cluster is not implemented yet");
}

void SeparableLookahead::write(const std::string& /*file_name*/) const {
    VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "SeparableLookahead::write is not implemented yet");
}

void SeparableLookahead::write_intra_cluster(const std::string& /*file*/) const {
    VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "SeparableLookahead::write_intra_cluster is not implemented yet");
}

float SeparableLookahead::get_opin_distance_min_delay(int physical_tile_idx, int from_layer, int to_layer, int x1, int x2, int y1, int y2) const {
    const float x_delay = opin_x_min_delay_[physical_tile_idx][from_layer][to_layer][x1][x2];
    const float y_delay = opin_y_min_delay_[physical_tile_idx][from_layer][to_layer][y1][y2];

    return x_delay + y_delay;
}
