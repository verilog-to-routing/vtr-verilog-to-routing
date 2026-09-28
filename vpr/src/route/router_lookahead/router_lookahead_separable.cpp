#include "router_lookahead_separable.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include "connection_router_interface.h"
#include "globals.h"
#include "router_lookahead_map.h"
#include "router_lookahead_map_utils.h"
#include "router_lookahead_sampling_utils.h"
#include "rr_node_types.h"
#include "vpr_context.h"
#include "vpr_error.h"
#include "vpr_utils.h"
#include "vtr_time.h"

/**
 * @brief Find the first channel node with the requested segment and exact direction.
 * Unidirectional wires must be driven at the sample position; BIDIR wires may span it.
 */
static RRNodeId get_chanxy_start_node_sep(int layer, int start_x, int start_y, Direction direction, e_rr_type rr_type, int seg_index) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    const auto& node_lookup = rr_graph.node_lookup();

    VTR_ASSERT(rr_type == e_rr_type::CHANX || rr_type == e_rr_type::CHANY);

    for (const RRNodeId node_id : node_lookup.find_channel_nodes(layer, start_x, start_y, rr_type)) {
        VTR_ASSERT(rr_graph.node_type(node_id) == rr_type);

        Direction node_direction = rr_graph.node_direction(node_id);
        RRIndexedDataId node_cost_ind = rr_graph.node_cost_index(node_id);
        int node_seg_ind = device_ctx.rr_indexed_data[node_cost_ind].seg_index;
        auto [driver_x, driver_y] = util::get_adjusted_rr_position(node_id);
        if (node_direction == direction && node_seg_ind == seg_index
            && (node_direction == Direction::BIDIR || (driver_x == start_x && driver_y == start_y))) {
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
    const auto& grid = device_ctx.grid;
    const auto& rr_graph = device_ctx.rr_graph;

    const bool profile_x = (axis == e_profile_axis::X);

    const size_t num_layers = grid.get_num_layers();
    // The wire look-ahead table has a dimension for channel type (CHANX/CHANY/CHANZ). In 2-d
    // architectures there is no CHANZ, so that dimension is dropped down to CHANX/CHANY only.
    const size_t chan_type_dim_size = (num_layers == 1) ? 2 : 3;
    // INC/DEC/BIDIR
    constexpr size_t direction_dim_size = 3;

    // Size of the profiling-axis dimension: device width when profiling x, height when profiling y.
    const size_t axis_dim_size = profile_x ? grid.width() : grid.height();

    wire_cost_map = vtr::NdMatrix<util::Cost_Entry, 7>({num_layers,
                                                        num_layers,
                                                        chan_type_dim_size,
                                                        segment_infs.size(),
                                                        direction_dim_size,
                                                        axis_dim_size,
                                                        axis_dim_size});
    wire_cost_map.fill(util::Cost_Entry());

    const int fixed_coord = profile_x ? (int)(2 * grid.height() / 3) : (int)grid.width() / 2;

    for (size_t from_layer_num = 0; from_layer_num < num_layers; from_layer_num++) {
        // Skip layers without inter-cluster routing resources.
        if (!device_ctx.inter_cluster_prog_routing_resources[from_layer_num]) {
            continue;
        }

        // Restrict the Dijkstra flood to a few rows/columns around the sample line (to keep the flood
        // cheap), but the full extent of the profiling axis.
        t_bb bb;
        if (profile_x) {
            bb = t_bb(0, grid.width() - 1,
                      std::max(0, fixed_coord - 2), std::min<int>(grid.height() - 1, fixed_coord + 2),
                      (int)from_layer_num, (int)from_layer_num);
        } else {
            bb = t_bb(std::max(0, fixed_coord - 2), std::min<int>(grid.width() - 1, fixed_coord + 2),
                      0, grid.height() - 1,
                      (int)from_layer_num, (int)from_layer_num);
        }

        for (const t_segment_inf& segment_inf : segment_infs) {
            std::vector<e_rr_type> chan_types;
            if (segment_inf.parallel_axis == e_parallel_axis::X_AXIS) {
                chan_types.push_back(e_rr_type::CHANX);
            } else if (segment_inf.parallel_axis == e_parallel_axis::Y_AXIS) {
                chan_types.push_back(e_rr_type::CHANY);
            } else if (segment_inf.parallel_axis == e_parallel_axis::Z_AXIS) {
                chan_types.push_back(e_rr_type::CHANZ);
            } else {
                VTR_ASSERT(segment_inf.parallel_axis == e_parallel_axis::BOTH_AXIS);
                // Both for BOTH_AXIS segments and special segments such as clock_networks we want to search in both directions.
                chan_types.insert(chan_types.end(), {e_rr_type::CHANX, e_rr_type::CHANY});
            }

            for (e_rr_type chan_type : chan_types) {
                const int chan_index = util::chan_type_to_index(chan_type);

                for (Direction direction : {Direction::INC, Direction::DEC, Direction::BIDIR}) {
                    const int direction_index = static_cast<int>(direction);

                    // get sample points at each position along the profiling axis on the sample line
                    std::vector<RRNodeId> sample_nodes;
                    for (int coord = 1; coord < (int)axis_dim_size; coord++) {
                        const int sample_x = profile_x ? coord : fixed_coord;
                        const int sample_y = profile_x ? fixed_coord : coord;
                        RRNodeId start_node;
                        if (is_chanxy(chan_type)) {
                            start_node = get_chanxy_start_node_sep(from_layer_num, sample_x, sample_y,
                                                                   direction, chan_type, segment_inf.seg_index);
                        } else {
                            VTR_ASSERT(is_chanz(chan_type));
                            start_node = util::get_chanz_start_node(sample_x, sample_y, segment_inf.seg_index, 0, direction);
                        }

                        if (start_node) {
                            sample_nodes.emplace_back(start_node);
                        }
                    }

                    if (sample_nodes.empty()) {
                        continue;
                    }

                    // Reuse the Dijkstra workspace across sample nodes.
                    util::t_dijkstra_data dijkstra_data;
                    for (RRNodeId sample_node : sample_nodes) {
                        // Use the driver end of the wire along the profiling axis as the start coordinate.
                        const bool dec = rr_graph.node_direction(sample_node) == Direction::DEC;
                        int start_coord;
                        if (profile_x) {
                            start_coord = dec ? rr_graph.node_xhigh(sample_node) : rr_graph.node_xlow(sample_node);
                        } else {
                            start_coord = dec ? rr_graph.node_yhigh(sample_node) : rr_graph.node_ylow(sample_node);
                        }

                        auto record_cost = [&](util::PQ_Entry current) {
                            RRNodeId curr_node = current.rr_node;
                            auto [ipin_x, ipin_y] = util::get_adjusted_rr_position(curr_node);
                            const int ipin_coord = profile_x ? ipin_x : ipin_y;
                            int ipin_layer = rr_graph.node_layer_low(curr_node);

                            util::Cost_Entry& cost_entry = wire_cost_map[from_layer_num][ipin_layer][chan_index][segment_inf.seg_index][direction_index][start_coord][ipin_coord];
                            if (!cost_entry.valid() || current.delay < cost_entry.delay) {
                                cost_entry = util::Cost_Entry(current.delay, current.congestion_upstream);
                            }
                        };

                        run_dijkstra(sample_node, dijkstra_data, bb, record_cost);
                    }
                }
            }
        }
    }
}

/**
 * @brief Minimum OPIN delay between two coordinates along one axis.
 * Reachable wires lack direction information, so minimize over all directions.
 * Include OPIN access delay only for x to count it once when summing the axes.
 * Fall back to MapLookahead for each OPIN without a usable wire cost.
 */
static float min_opin_axis_delay(const util::t_src_opin_delays& src_opin_delays,
                                 const vtr::NdMatrix<util::Cost_Entry, 7>& wire_cost_map,
                                 e_profile_axis axis,
                                 const RouterLookahead& map_lookahead,
                                 int physical_tile_idx,
                                 int from_layer,
                                 int to_layer,
                                 int c1,
                                 int c2,
                                 bool include_opin_access_delay) {
    const bool profile_x = (axis == e_profile_axis::X);
    float min_delay = std::numeric_limits<float>::infinity();

    // An entry is unusable if it was never profiled, or profiled as unreachable.
    auto is_usable = [](float delay) {
        return std::isfinite(delay) && delay != ROUTER_LOOKAHEAD_NO_PATH_SENTINEL;
    };

    for (const auto& tile_opin_map : src_opin_delays[from_layer][physical_tile_idx]) {
        float expected_delay = std::numeric_limits<float>::infinity();

        for (const auto& layer_src_opin_delay_map : tile_opin_map) {
            for (const auto& kv : layer_src_opin_delay_map) {
                const util::t_reachable_wire_inf& reachable_wire_inf = kv.second;
                if (reachable_wire_inf.wire_rr_type == e_rr_type::SINK) {
                    continue;
                }

                const int chan_index = util::chan_type_to_index(reachable_wire_inf.wire_rr_type);
                if (chan_index >= (int)wire_cost_map.dim_size(2)) {
                    continue;
                }

                // The reachable wire info does not record the wire's direction, so consider all of them.
                float wire_delay = std::numeric_limits<float>::infinity();
                for (size_t dir_index = 0; dir_index < wire_cost_map.dim_size(4); ++dir_index) {
                    const util::Cost_Entry& entry = wire_cost_map[reachable_wire_inf.layer_number][to_layer][chan_index][reachable_wire_inf.wire_seg_index][dir_index][c1][c2];
                    if (entry.valid() && is_usable(entry.delay)) {
                        wire_delay = std::min(wire_delay, entry.delay);
                    }
                }

                const float access_delay = include_opin_access_delay ? reachable_wire_inf.delay : 0.f;
                expected_delay = std::min(expected_delay, access_delay + wire_delay);
            }
        }

        if (!is_usable(expected_delay)) {
            int delta_x = profile_x ? std::abs(c1 - c2) : 0;
            int delta_y = profile_x ? 0 : std::abs(c1 - c2);
            expected_delay = map_lookahead.get_opin_distance_min_delay(physical_tile_idx, from_layer, to_layer, delta_x, delta_y);
        }

        min_delay = std::min(min_delay, expected_delay);
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
                                    bool include_opin_access_delay,
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
            for (int to_layer_num = 0; to_layer_num < num_layers; to_layer_num++) {
                for (int c1 = 0; c1 < axis_dim_size; c1++) {
                    for (int c2 = 0; c2 < axis_dim_size; c2++) {
                        axis_min_delay[tile_type_idx][from_layer_num][to_layer_num][c1][c2] = min_opin_axis_delay(src_opin_delays,
                                                                                                                  wire_cost_map,
                                                                                                                  axis,
                                                                                                                  map_lookahead,
                                                                                                                  tile_type_idx,
                                                                                                                  from_layer_num,
                                                                                                                  to_layer_num,
                                                                                                                  c1,
                                                                                                                  c2,
                                                                                                                  include_opin_access_delay);
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
}

float SeparableLookahead::get_expected_cost(RRNodeId current_node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    e_rr_type from_rr_type = rr_graph.node_type(current_node);

    VTR_ASSERT_SAFE(rr_graph.node_type(target_node) == e_rr_type::SINK);

    if (is_flat_) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "SeparableLookahead does not support flat routing");
    }

    if (is_chanxy(from_rr_type) || is_chanz(from_rr_type) || from_rr_type == e_rr_type::SOURCE || from_rr_type == e_rr_type::OPIN) {
        auto [delay_cost, cong_cost] = get_expected_delay_and_cong(current_node, target_node, params, R_upstream);
        return delay_cost + cong_cost;
    }
    if (from_rr_type == e_rr_type::IPIN) {
        return device_ctx.rr_indexed_data[RRIndexedDataId(SINK_COST_INDEX)].base_cost;
    }
    return 0.f;
}

std::pair<float, float> SeparableLookahead::get_expected_delay_and_cong(RRNodeId from_node, RRNodeId to_node, const t_conn_cost_params& params, float R_upstream) const {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    int from_layer_num = rr_graph.node_layer_low(from_node);
    int to_layer_num = rr_graph.node_layer_low(to_node);

    float expected_delay_cost = std::numeric_limits<float>::infinity();
    float expected_cong_cost = std::numeric_limits<float>::infinity();

    e_rr_type from_type = rr_graph.node_type(from_node);
    if (from_type == e_rr_type::SOURCE || from_type == e_rr_type::OPIN) {
        return map_lookahead_->get_expected_delay_and_cong(from_node, to_node, params, R_upstream);
    } else if (is_chanxy(from_type) || is_chanz(from_type)) {
        Direction from_dir = rr_graph.node_direction(from_node);

        // CHANZ drives from its destination layer; BIDIR can use the target layer.
        if (from_type == e_rr_type::CHANZ) {
            if (from_dir == Direction::INC) {
                from_layer_num = rr_graph.node_layer_high(from_node);
            } else if (from_dir == Direction::BIDIR) {
                from_layer_num = to_layer_num;
            }
        }

        RRIndexedDataId from_cost_index = rr_graph.node_cost_index(from_node);
        int from_seg_index = device_ctx.rr_indexed_data[from_cost_index].seg_index;
        VTR_ASSERT(from_seg_index >= 0);

        const int chan_index = util::chan_type_to_index(from_type);
        const int dir_index = static_cast<int>(from_dir);

        // Match the driver coordinates used during profiling.
        const bool dec = (from_dir == Direction::DEC);
        const int from_x = dec ? rr_graph.node_xhigh(from_node) : rr_graph.node_xlow(from_node);
        const int from_y = dec ? rr_graph.node_yhigh(from_node) : rr_graph.node_ylow(from_node);

        auto [to_x, to_y] = util::get_adjusted_rr_position(to_node);

        const util::Cost_Entry& x_cost = x_wire_cost_map_[from_layer_num][to_layer_num][chan_index][from_seg_index][dir_index][from_x][to_x];
        const util::Cost_Entry& y_cost = y_wire_cost_map_[from_layer_num][to_layer_num][chan_index][from_seg_index][dir_index][from_y][to_y];

        expected_delay_cost = x_cost.delay + y_cost.delay;
        expected_cong_cost = x_cost.congestion + y_cost.congestion;

    } else if (from_type == e_rr_type::IPIN) {
        return std::make_pair(0., device_ctx.rr_indexed_data[RRIndexedDataId(SINK_COST_INDEX)].base_cost);
    } else {
        return std::make_pair(0., 0.);
    }

    // Use the map lookahead when the combined delay or congestion cost is unusable.
    if (!std::isfinite(expected_delay_cost) || !std::isfinite(expected_cong_cost)
        || expected_delay_cost == ROUTER_LOOKAHEAD_NO_PATH_SENTINEL
        || expected_cong_cost == ROUTER_LOOKAHEAD_NO_PATH_SENTINEL) {
        return map_lookahead_->get_expected_delay_and_cong(from_node, to_node, params, R_upstream);
    }
    expected_delay_cost *= params.criticality;
    expected_cong_cost *= (1.0f - params.criticality);

    return std::make_pair(expected_delay_cost, expected_cong_cost);
}

void SeparableLookahead::compute(const std::vector<t_segment_inf>& segment_inf) {
    vtr::ScopedStartFinishTimer timer("Computing router lookahead separable");

    compute_wire_cost_map_for_axis(segment_inf, e_profile_axis::X, x_wire_cost_map_);
    compute_wire_cost_map_for_axis(segment_inf, e_profile_axis::Y, y_wire_cost_map_);

    const util::t_src_opin_delays src_opin_delays = util::compute_router_src_opin_lookahead(is_flat_, route_verbosity_, device_model_warnings_);

    // OPIN reduction uses the map lookahead when a separable cost is unavailable.
    map_lookahead_->compute(segment_inf);

    min_opin_axis_delay_map(src_opin_delays, x_wire_cost_map_, e_profile_axis::X, *map_lookahead_,
                            true, opin_x_min_delay_);
    min_opin_axis_delay_map(src_opin_delays, y_wire_cost_map_, e_profile_axis::Y, *map_lookahead_,
                            false, opin_y_min_delay_);
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

float SeparableLookahead::get_opin_distance_min_delay(int physical_tile_idx, int from_layer, int to_layer, int dx, int dy) const {
    return map_lookahead_->get_opin_distance_min_delay(physical_tile_idx, from_layer, to_layer, dx, dy);
}

float SeparableLookahead::get_opin_min_delay_x(int physical_tile_idx, int from_layer, int to_layer, int x1, int x2) const {
    return opin_x_min_delay_[physical_tile_idx][from_layer][to_layer][x1][x2];
}

float SeparableLookahead::get_opin_min_delay_y(int physical_tile_idx, int from_layer, int to_layer, int y1, int y2) const {
    return opin_y_min_delay_[physical_tile_idx][from_layer][to_layer][y1][y2];
}
