#include "router_lookahead_separable.h"

#include <memory>
#include "connection_router_interface.h"
#include "globals.h"
#include "router_lookahead_map.h"
#include "router_lookahead_map_utils.h"
#include "vpr_context.h"
#include "vpr_error.h"
#include "vpr_utils.h"
#include "vtr_time.h"

/* iterates over the children of the specified node and selectively pushes them onto the priority queue.
 * This is a copy of the (static) expand_dijkstra_neighbours() in router_lookahead_map_utils.cpp, adapted
 * to always restrict the expansion to a bounding box (no interposer-cut handling, since that is not yet
 * supported by this lookahead). */
static void expand_separable_wire_dijkstra_neighbours(util::PQ_Entry parent_entry,
                                                       vtr::vector<RRNodeId, float>& node_visited_costs,
                                                       vtr::vector<RRNodeId, bool>& node_expanded,
                                                       std::priority_queue<util::PQ_Entry>& pq,
                                                       const t_bb& bb) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    RRNodeId parent = parent_entry.rr_node;

    for (t_edge_size edge : rr_graph.edges(parent)) {
        RRNodeId child_node = rr_graph.edge_sink_node(parent, edge);
        // Don't expand the nodes inside the clusters since the intra-cluster lookahead
        // is computed separately.
        if (!is_inter_cluster_node(rr_graph, child_node)) {
            continue;
        }

        // Don't expand nodes whose adjusted position falls outside of the bounding box.
        auto [child_x, child_y] = util::get_adjusted_rr_position(child_node);
        if (child_x < bb.xmin || child_x > bb.xmax || child_y < bb.ymin || child_y > bb.ymax) {
            continue;
        }

        int switch_ind = size_t(rr_graph.edge_switch(parent, edge));

        if (rr_graph.node_type(child_node) == e_rr_type::SINK) return;

        /* skip this child if it has already been expanded from */
        if (node_expanded[child_node]) {
            continue;
        }

        util::PQ_Entry child_entry(child_node, switch_ind, parent_entry.delay,
                                   parent_entry.R_upstream, parent_entry.congestion_upstream, false);

        /* skip this child if it has been visited with smaller cost */
        if (node_visited_costs[child_node] >= 0 && node_visited_costs[child_node] < child_entry.cost) {
            continue;
        }

        /* finally, record the cost with which the child was visited and put the child entry on the queue */
        node_visited_costs[child_node] = child_entry.cost;
        pq.push(child_entry);
    }
}

/// @brief The device axis (x or y) whose travel cost a separable wire cost map profiles.
enum class e_profile_axis {
    X, ///< Profiles horizontal travel; populates an x_wire_cost_map (keyed by x1/x2).
    Y  ///< Profiles vertical travel; populates a y_wire_cost_map (keyed by y1/y2).
};

/* Runs Dijkstra's algorithm from start_node, restricted to the given bounding box, until all reachable
 * nodes within the box have been visited. Each time an IPIN is visited, the minimum delay/congestion to
 * reach the IPIN's location along the profiling axis is recorded in wire_cost_map, keyed by
 * [from_layer_num][ipin_layer][chan_index][seg_index][direction_index][start_coord][ipin_coord], where
 * start_coord/ipin_coord are the x (profile_x) or y (!profile_x) coordinates of the start node and IPIN.
 *
 * This is a copy of the (static) run_dijkstra() in router_lookahead_map_utils.cpp, adapted to record
 * entries into a separable wire_cost_map instead of the (delta_x, delta_y)-indexed routing_cost_map. */
static void run_separable_wire_dijkstra(RRNodeId start_node,
                                        int start_coord,
                                        bool profile_x,
                                        unsigned from_layer_num,
                                        int chan_index,
                                        int seg_index,
                                        int direction_index,
                                        const t_bb& bb,
                                        util::t_dijkstra_data& data,
                                        vtr::NdMatrix<util::Cost_Entry, 7>& wire_cost_map) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    vtr::vector<RRNodeId, bool>& node_expanded = data.node_expanded;
    node_expanded.resize(rr_graph.num_nodes());
    std::fill(node_expanded.begin(), node_expanded.end(), false);

    vtr::vector<RRNodeId, float>& node_visited_costs = data.node_visited_costs;
    node_visited_costs.resize(rr_graph.num_nodes());
    std::fill(node_visited_costs.begin(), node_visited_costs.end(), -1.0);

    std::priority_queue<util::PQ_Entry>& pq = data.pq;
    while (!pq.empty()) {
        pq.pop();
    }

    // First entry has no upstream delay or congestion
    pq.emplace(start_node, UNDEFINED, 0, 0, 0, true);

    while (!pq.empty()) {
        util::PQ_Entry current = pq.top();
        pq.pop();

        RRNodeId curr_node = current.rr_node;

        // Check that we haven't already expanded from this node
        if (node_expanded[curr_node]) {
            continue;
        }

        // If this node is an IPIN, record its cost to reach in wire_cost_map, keeping the smallest
        // delay seen so far for this (start_coord, ipin_coord) pair.
        if (rr_graph.node_type(curr_node) == e_rr_type::IPIN) {
            auto [ipin_x, ipin_y] = util::get_adjusted_rr_position(curr_node);
            const int ipin_coord = profile_x ? ipin_x : ipin_y;
            int ipin_layer = rr_graph.node_layer_low(curr_node);

            util::Cost_Entry& cost_entry = wire_cost_map[from_layer_num][ipin_layer][chan_index][seg_index][direction_index][start_coord][ipin_coord];
            if (!cost_entry.valid() || current.delay < cost_entry.delay) {
                cost_entry = util::Cost_Entry(current.delay, current.congestion_upstream);
            }
        }

        expand_separable_wire_dijkstra_neighbours(current, node_visited_costs, node_expanded, pq, bb);
        node_expanded[curr_node] = true;
    }
}

/* Populates wire_cost_map for the given profiling axis. Sample nodes are taken along a line through the
 * middle of the device that is perpendicular to the profiling axis (a middle-height row when profiling x,
 * a middle-width column when profiling y), and a Dijkstra flood is run from each, restricted to a few
 * rows/columns around that line but spanning the full extent of the profiling axis. */
static void compute_wire_cost_map_for_axis(const std::vector<t_segment_inf>& segment_infs,
                                           e_profile_axis axis,
                                           vtr::NdMatrix<util::Cost_Entry, 7>& wire_cost_map) {
    // look at compute_router_wire_lookahead in router_lookahead_map.cpp for reference.
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

    // Re-allocate
    wire_cost_map = vtr::NdMatrix<util::Cost_Entry, 7>({num_layers,
                                                        num_layers,
                                                        chan_type_dim_size,
                                                        segment_infs.size(),
                                                        direction_dim_size,
                                                        axis_dim_size,
                                                        axis_dim_size});
    wire_cost_map.fill(util::Cost_Entry());

    // Sample along a line through the middle of the device, perpendicular to the profiling axis:
    // a middle-height row when profiling x, a middle-width column when profiling y.
    const int fixed_coord = profile_x ? (int)(2 * grid.height() / 3) : (int)grid.width() / 2;

    for (size_t from_layer_num = 0; from_layer_num < num_layers; from_layer_num++) {
        // If arch file specifies die_number="layer_num" doesn't have inter-cluster
        // programmable routing resources, then we shouldn't profile wire segment types on
        // the current layer
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
                            start_node = util::get_chanxy_start_node(from_layer_num, sample_x, sample_y,
                                                                     direction, chan_type, segment_inf.seg_index, 0);
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

                    // Run a dijkstra flood from each sample node, recording the minimum cost to reach
                    // each IPIN's location along the profiling axis. dijkstra_data is created outside the
                    // loop and passed by reference to run_separable_wire_dijkstra() to avoid repeated
                    // memory allocation/de-allocation.
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

                        run_separable_wire_dijkstra(sample_node, start_coord, profile_x, from_layer_num,
                                                    chan_index, segment_inf.seg_index, direction_index, bb,
                                                    dijkstra_data, wire_cost_map);
                    }
                }
            }
        }
    }
}

static void compute_router_wire_lookahead(const std::vector<t_segment_inf>& segment_infs,
                                          int /*route_verbosity*/,
                                          bool /*device_model_warnings*/,
                                          t_x_wire_cost_map& x_wire_cost_map,
                                          t_y_wire_cost_map& y_wire_cost_map) {
    compute_wire_cost_map_for_axis(segment_infs, e_profile_axis::X, x_wire_cost_map);
    compute_wire_cost_map_for_axis(segment_infs, e_profile_axis::Y, y_wire_cost_map);
}

/**
 * @note This lookahead is currently a skeleton: the class compiles and can be
 *       selected via --router_lookahead separable, but none of its methods are
 *       implemented yet.
 */
#define NOT_IMPLEMENTED_ERROR(func_name) \
    VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "%s is not implemented yet", func_name)

SeparableLookahead::SeparableLookahead(const t_det_routing_arch& det_routing_arch, bool is_flat, int route_verbosity, bool device_model_warnings, float interposer_base_cost_multiplier)
    : det_routing_arch_(det_routing_arch)
    , is_flat_(is_flat)
    , route_verbosity_(route_verbosity)
    , device_model_warnings_(device_model_warnings)
    , interposer_base_cost_multiplier_(interposer_base_cost_multiplier) {
    has_interposer_cuts_ = g_vpr_ctx.device().grid.has_interposer_cuts();

    // Delegate SOURCE/OPIN distance queries to a standard MapLookahead.
    map_lookahead_ = std::make_unique<MapLookahead>(det_routing_arch, is_flat, route_verbosity, device_model_warnings, interposer_base_cost_multiplier);
}

float SeparableLookahead::get_expected_cost(RRNodeId current_node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    e_rr_type from_rr_type = rr_graph.node_type(current_node);

    VTR_ASSERT_SAFE(rr_graph.node_type(target_node) == e_rr_type::SINK);

    if (is_flat_) {
        return get_expected_cost_flat_router(current_node, target_node, params, R_upstream);
    } else {
        if (from_rr_type == e_rr_type::CHANX || from_rr_type == e_rr_type::CHANY || from_rr_type == e_rr_type::CHANZ
            || from_rr_type == e_rr_type::SOURCE || from_rr_type == e_rr_type::OPIN) {
            // Get the total cost using the combined delay and congestion costs
            auto [delay_cost, cong_cost] = get_expected_delay_and_cong(current_node, target_node, params, R_upstream);
            return delay_cost + cong_cost;
        } else if (from_rr_type == e_rr_type::IPIN) { // Change if you're allowing route-throughs
            return device_ctx.rr_indexed_data[RRIndexedDataId(SINK_COST_INDEX)].base_cost;
        } else { // Change this if you want to investigate route-throughs
            return 0.f;
        }
    }
}

float SeparableLookahead::get_expected_cost_flat_router(RRNodeId /*current_node*/, RRNodeId /*target_node*/, const t_conn_cost_params& /*params*/, float /*R_upstream*/) const {
    NOT_IMPLEMENTED_ERROR("SeparableLookahead::get_expected_cost_flat_router");
    return 0.f;
}

std::pair<float, float> SeparableLookahead::get_expected_delay_and_cong(RRNodeId from_node, RRNodeId to_node, const t_conn_cost_params& params, float R_upstream) const {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    int from_layer_num = rr_graph.node_layer_low(from_node);
    int to_layer_num = rr_graph.node_layer_low(to_node);
    auto [delta_x, delta_y] = util::get_xy_deltas(from_node, to_node);
    delta_x = std::abs(delta_x);
    delta_y = std::abs(delta_y);

    float expected_delay_cost = std::numeric_limits<float>::infinity();
    float expected_cong_cost = std::numeric_limits<float>::infinity();

    e_rr_type from_type = rr_graph.node_type(from_node);
    if (from_type == e_rr_type::SOURCE || from_type == e_rr_type::OPIN) {
        // SOURCE/OPIN cost estimation is delegated to the standard MapLookahead.
        return map_lookahead_->get_expected_delay_and_cong(from_node, to_node, params, R_upstream);
    } else if (from_type == e_rr_type::CHANX || from_type == e_rr_type::CHANY || from_type == e_rr_type::CHANZ) {
        // From a wire we look up the separable x and y travel costs directly in the absolute-coordinate
        // wire cost maps and add them. The maps are keyed by [from_layer][to_layer][chan][seg][dir]
        // [start_coord][target_coord], where start_coord is the driver end of the wire (matching how the
        // maps were profiled) and target_coord is the adjusted position of the target.

        Direction from_dir = rr_graph.node_direction(from_node);

        // For CHANZ nodes, if the direction is
        // 1) Incremental --> `chanz_node` now drives other nodes on node_layer_high(chanz_node).
        // 2) Decremental --> `chanz_node` now drives other nodes on node_layer_low(chanz_node).
        // 3) Bidirectional --> `chanz_node` now drives other nodes on both layers, so we choose the target layer
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

        // Start coordinate: the driver end of the wire (xhigh/yhigh for DEC wires, xlow/ylow otherwise),
        // matching the start_coord used when the maps were profiled.
        const bool dec = (from_dir == Direction::DEC);
        const int from_x = dec ? rr_graph.node_xhigh(from_node) : rr_graph.node_xlow(from_node);
        const int from_y = dec ? rr_graph.node_yhigh(from_node) : rr_graph.node_ylow(from_node);

        // Target coordinate: the adjusted position of the target node.
        auto [to_x, to_y] = util::get_adjusted_rr_position(to_node);

        const util::Cost_Entry& x_cost = x_wire_cost_map_[from_layer_num][to_layer_num][chan_index][from_seg_index][dir_index][from_x][to_x];
        const util::Cost_Entry& y_cost = y_wire_cost_map_[from_layer_num][to_layer_num][chan_index][from_seg_index][dir_index][from_y][to_y];

        expected_delay_cost = x_cost.delay + y_cost.delay;
        expected_cong_cost = x_cost.congestion + y_cost.congestion;

    } else if (from_type == e_rr_type::IPIN) { // Change if you're allowing route-throughs
        return std::make_pair(0., device_ctx.rr_indexed_data[RRIndexedDataId(SINK_COST_INDEX)].base_cost);
    } else { // Change this if you want to investigate route-throughs
        return std::make_pair(0., 0.);
    }

    // If we have no profiled path for this distance the cost is infinite; fall back to a large (but
    // finite) sentinel so the router de-prioritizes, but does not forbid, exploring this path.
    if (!std::isfinite(expected_delay_cost)) {
        expected_delay_cost = ROUTER_LOOKAHEAD_NO_PATH_SENTINEL;
    }
    if (!std::isfinite(expected_cong_cost)) {
        expected_cong_cost = ROUTER_LOOKAHEAD_NO_PATH_SENTINEL;
    }

    if (expected_delay_cost == ROUTER_LOOKAHEAD_NO_PATH_SENTINEL || expected_cong_cost == ROUTER_LOOKAHEAD_NO_PATH_SENTINEL) {
        return map_lookahead_->get_expected_delay_and_cong(from_node, to_node, params, R_upstream);
    }
    expected_delay_cost *= params.criticality;
    expected_cong_cost *= (1.0f - params.criticality);

    return std::make_pair(expected_delay_cost, expected_cong_cost);
}

void SeparableLookahead::compute(const std::vector<t_segment_inf>& segment_inf) {
    vtr::ScopedStartFinishTimer timer("Computing router lookahead separable");

    // First compute the delay map when starting from the various wire types
    // (CHANX/CHANY/CHANZ) in the routing architecture
    compute_router_wire_lookahead(segment_inf, route_verbosity_, device_model_warnings_, x_wire_cost_map_, y_wire_cost_map_);

    // Build the delegate MapLookahead, which provides the SOURCE/OPIN distance estimates.
    map_lookahead_->compute(segment_inf);
}

void SeparableLookahead::compute_intra_tile() {
    NOT_IMPLEMENTED_ERROR("SeparableLookahead::compute_intra_tile");
}

void SeparableLookahead::read(const std::string& /*file*/) {
    NOT_IMPLEMENTED_ERROR("SeparableLookahead::read");
}

void SeparableLookahead::read_intra_cluster(const std::string& /*file*/) {
    NOT_IMPLEMENTED_ERROR("SeparableLookahead::read_intra_cluster");
}

void SeparableLookahead::write(const std::string& /*file_name*/) const {
    NOT_IMPLEMENTED_ERROR("SeparableLookahead::write");
}

void SeparableLookahead::write_intra_cluster(const std::string& /*file*/) const {
    NOT_IMPLEMENTED_ERROR("SeparableLookahead::write_intra_cluster");
}

float SeparableLookahead::get_opin_distance_min_delay(int physical_tile_idx, int from_layer, int to_layer, int dx, int dy) const {
    return map_lookahead_->get_opin_distance_min_delay(physical_tile_idx, from_layer, to_layer, dx, dy);
}

void read_router_lookahead_separable(const std::string& /*file*/) {
    NOT_IMPLEMENTED_ERROR("read_router_lookahead_separable");
}

void write_router_lookahead_separable(const std::string& /*file*/) {
    NOT_IMPLEMENTED_ERROR("write_router_lookahead_separable");
}

#undef NOT_IMPLEMENTED_ERROR
