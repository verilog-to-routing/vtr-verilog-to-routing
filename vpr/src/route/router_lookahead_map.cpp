/*
 * The router lookahead provides an estimate of the cost from an intermediate node to the target node
 * during directed (A*-like) routing.
 *
 * The VPR 7.0 lookahead (route/route_timing.c ==> get_timing_driven_expected_cost) lower-bounds the remaining delay and
 * congestion by assuming that a minimum number of wires, of the same type as the current node being expanded, can be used
 * to complete the route. While this method is efficient, it can run into trouble with architectures that use
 * multiple interconnected wire types.
 *
 * The lookahead in this file performs undirected Dijkstra searches to evaluate many paths through the routing network,
 * starting from all the different wire types in the routing architecture. This ensures the lookahead captures the
 * effect of inter-wire connectivity. This information is then reduced into a delta_x delta_y based lookup table for
 * reach source wire type (f_cost_map). This is used for estimates from CHANX/CHANY -> SINK nodes. See Section 3.2.4
 * in Oleg Petelin's MASc thesis (2016) for more discussion.
 *
 * To handle estimates starting from SOURCE/OPIN's the lookahead also creates a small side look-up table of the wire types
 * which are reachable from each physical tile type's SOURCEs/OPINs (f_src_opin_delays). This is used for
 * SRC/OPIN -> CHANX/CHANY estimates.
 *
 * In the case of SRC/OPIN -> SINK estimates the results from the two look-ups are added together (and the minimum taken
 * if there are multiple possibilities).
 */

#include <cmath>
#include <vector>
#include "connection_router_interface.h"
#include "vpr_types.h"
#include "vpr_error.h"
#include "vpr_utils.h"
#include "globals.h"
#include "vtr_math.h"
#include "vtr_log.h"
#include "vtr_assert.h"
#include "vtr_time.h"
#include "vtr_geometry.h"
#include "router_lookahead_map.h"
#include "router_lookahead_map_utils.h"
#include "rr_graph2.h"
#include "rr_graph.h"
#include "route_common.h"

#ifdef VTR_ENABLE_CAPNPROTO
#    include "capnp/serialize.h"
#    include "map_lookahead.capnp.h"
#    include "ndmatrix_serdes.h"
#    include "intra_cluster_serdes.h"
#    include "mmap_file.h"
#    include "serdes_utils.h"
#endif /* VTR_ENABLE_CAPNPROTO */

static constexpr int VALID_NEIGHBOR_NUMBER = 3;

/* when a list of delay/congestion entries at a coordinate in Cost_Entry is boiled down to a single
 * representative entry, this enum is passed-in to specify how that representative entry should be
 * calculated */
enum e_representative_entry_method {
    FIRST = 0, //the first cost that was recorded
    SMALLEST,  //the smallest-delay cost recorded
    AVERAGE,
    GEOMEAN,
    MEDIAN
};

/******** File-Scope Variables ********/

//Look-up table from CHANX/CHANY (to SINKs) for various distances
t_wire_cost_map f_wire_cost_map;

/******** File-Scope Functions ********/

/***
 * @brief Fill f_wire_cost_map. It is a look-up table from CHANX/CHANY (to SINKs) for various distances
 * @param segment_inf
 */
static util::Cost_Entry get_wire_cost_entry(e_rr_type rr_type,
                                            int seg_index,
                                            int from_layer_num,
                                            int delta_x,
                                            int delta_y,
                                            int to_layer_num);

static void compute_router_wire_lookahead(const std::vector<t_segment_inf>& segment_inf);

/***
 * @brief Compute the cost from pin to sinks of tiles - Compute the minimum cost to get to each tile sink from pins on the cluster
 * @param intra_tile_pin_primitive_pin_delay
 * @param tile_min_cost
 * @param det_routing_arch
 * @param device_ctx
 */
static void compute_tiles_lookahead(std::unordered_map<int, util::t_ipin_primitive_sink_delays>& intra_tile_pin_primitive_pin_delay,
                                    std::unordered_map<int, std::unordered_map<int, util::Cost_Entry>>& tile_min_cost,
                                    const t_det_routing_arch& det_routing_arch,
                                    const DeviceContext& device_ctx);
/***
 * @brief Compute the cose from tile pins to tile sinks
 * @param intra_tile_pin_primitive_pin_delay [physical_tile_type_idx][from_pin_ptc_num][sink_ptc_num] -> cost
 * @param physical_tile
 * @param det_routing_arch
 * @param delayless_switch
 */
static void compute_tile_lookahead(std::unordered_map<int, util::t_ipin_primitive_sink_delays>& intra_tile_pin_primitive_pin_delay,
                                   t_physical_tile_type_ptr physical_tile,
                                   const t_det_routing_arch& det_routing_arch,
                                   const int delayless_switch);

/***
 * @brief Compute the minimum cost to get to the sinks from pins on the cluster
 * @param tile_min_cost [physical_tile_idx][sink_ptc_num] -> min_cost
 * @param physical_tile
 * @param intra_tile_pin_primitive_pin_delay [physical_tile_type_idx][from_pin_ptc_num][sink_ptc_num] -> cost
 */
static void store_min_cost_to_sinks(std::unordered_map<int, std::unordered_map<int, util::Cost_Entry>>& tile_min_cost,
                                    t_physical_tile_type_ptr physical_tile,
                                    const std::unordered_map<int, util::t_ipin_primitive_sink_delays>& intra_tile_pin_primitive_pin_delay);

/**
 * @brief Iterate over the first (channel type) and second (segment type) dimensions of f_wire_cost_map to get the minimum cost for each dx and dy_
 * @param internal_opin_global_cost_map This map is populated in this function. [dx][dy] -> cost
 */
static void min_chann_global_cost_map(vtr::NdMatrix<util::Cost_Entry, 4>& distance_min_cost);

/**
 * @brief // Given the src/opin map of each physical tile type, iterate over all OPINs/sources of a type and create
 * the minimum cost map across all of them for each tile type.
 * @param src_opin_delays
 * @param distance_min_cost
 */
static void min_opin_distance_cost_map(const util::t_src_opin_delays& src_opin_delays, vtr::NdMatrix<util::Cost_Entry, 5>& distance_min_cost);

// Read the file and fill intra_tile_pin_primitive_pin_delay and tile_min_cost
static void read_intra_cluster_router_lookahead(std::unordered_map<int, util::t_ipin_primitive_sink_delays>& intra_tile_pin_primitive_pin_delay,
                                                const std::string& file);

// Write the file with intra_tile_pin_primitive_pin_delay and tile_min_cost
static void write_intra_cluster_router_lookahead(const std::string& file,
                                                 const std::unordered_map<int, util::t_ipin_primitive_sink_delays>& intra_tile_pin_primitive_pin_delay);

/* sets the lookahead cost map entries based on representative cost entries from routing_cost_map */
static void set_lookahead_map_costs(int from_layer_num, int segment_index, e_rr_type chan_type, util::t_routing_cost_map& routing_cost_map);
/* fills in missing lookahead map entries by copying the cost of the closest valid entry */
static void fill_in_missing_lookahead_entries(int segment_index, e_rr_type chan_type);
/* returns a cost entry in the f_wire_cost_map that is near the specified coordinates (and preferably towards (0,0)) */
static util::Cost_Entry get_nearby_cost_entry(int from_layer_num, int x, int y, int to_layer_num, int segment_index, int chan_index);

/**
 * @brief Fill in the missing entry in router lookahead map
 * If there is a missing entry in the router lookahead, search among its neighbors in a 3x3 window. If there are `VALID_NEIGHBOR_NUMBER` valid entries,
 * take the average of them and fill in the missing entry.
 * @param from_layer_num The layer num of the source node
 * @param missing_dx Dx of the missing input
 * @param missing_dy Dy of the missing input
 * @param to_layer_num The layer num of the destination point
 * @param segment_index The segment index of the source node
 * @param chan_index The channel index of the source node
 * @return The cost for the missing entry
 */
static util::Cost_Entry get_nearby_cost_entry_average_neighbour(int from_layer_num,
                                                                int missing_dx,
                                                                int missing_dy,
                                                                int to_layer_num,
                                                                int segment_index,
                                                                int chan_index);

/******** Interface class member function definitions ********/
MapLookahead::MapLookahead(const t_det_routing_arch& det_routing_arch, bool is_flat)
    : det_routing_arch_(det_routing_arch)
    , is_flat_(is_flat) {}

float MapLookahead::get_expected_cost(RRNodeId current_node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    t_rr_type from_rr_type = rr_graph.node_type(current_node);

    VTR_ASSERT_SAFE(rr_graph.node_type(target_node) == t_rr_type::SINK);

    if (is_flat_) {
        return get_expected_cost_flat_router(current_node, target_node, params, R_upstream);
    } else {
        if (from_rr_type == CHANX || from_rr_type == CHANY || from_rr_type == SOURCE || from_rr_type == OPIN) {
            // Get the total cost using the combined delay and congestion costs
            auto [delay_cost, cong_cost] = get_expected_delay_and_cong(current_node, target_node, params, R_upstream);
            return delay_cost + cong_cost;
        } else if (from_rr_type == IPIN) { /* Change if you're allowing route-throughs */
            return (device_ctx.rr_indexed_data[RRIndexedDataId(SINK_COST_INDEX)].base_cost);
        } else { /* Change this if you want to investigate route-throughs */
            return (0.);
        }
    }
}

float MapLookahead::get_expected_cost_flat_router(RRNodeId current_node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    t_rr_type from_rr_type = rr_graph.node_type(current_node);

    VTR_ASSERT_SAFE(rr_graph.node_type(target_node) == t_rr_type::SINK);

    float delay_cost = 0.;
    float cong_cost = 0.;

    t_physical_tile_type_ptr from_physical_type = device_ctx.grid.get_physical_type({rr_graph.node_xlow(current_node),
                                                                                     rr_graph.node_ylow(current_node),
                                                                                     rr_graph.node_layer(current_node)});
    int from_node_ptc_num = rr_graph.node_ptc_num(current_node);
    t_physical_tile_type_ptr to_physical_type = device_ctx.grid.get_physical_type({rr_graph.node_xlow(target_node),
                                                                                   rr_graph.node_ylow(target_node),
                                                                                   rr_graph.node_layer(target_node)});
    float delay_offset_cost = 0.;
    float cong_offset_cost = 0.;
    int to_node_ptc_num = rr_graph.node_ptc_num(target_node);
    int to_layer_num = rr_graph.node_layer(target_node);
    // We have not checked the multi-layer FPGA for flat routing
    VTR_ASSERT(rr_graph.node_layer(current_node) == rr_graph.node_layer(target_node));
    if (from_rr_type == CHANX || from_rr_type == CHANY) {
        std::tie(delay_cost, cong_cost) = get_expected_delay_and_cong(current_node, target_node, params, R_upstream);

        // delay_cost and cong_cost only represent the cost to get to the root-level pins. The below offsets are used to represent the intra-cluster cost
        // of getting to a sink
        delay_offset_cost = params.criticality * tile_min_cost.at(to_physical_type->index).at(to_node_ptc_num).delay;
        cong_offset_cost = (1. - params.criticality) * tile_min_cost.at(to_physical_type->index).at(to_node_ptc_num).congestion;

        return delay_cost + cong_cost + delay_offset_cost + cong_offset_cost;
    } else if (from_rr_type == OPIN) {
        if (is_inter_cluster_node(rr_graph, current_node)) {
            // Similar to CHANX and CHANY
            std::tie(delay_cost, cong_cost) = get_expected_delay_and_cong(current_node, target_node, params, R_upstream);

            delay_offset_cost = params.criticality * tile_min_cost.at(to_physical_type->index).at(to_node_ptc_num).delay;
            cong_offset_cost = (1. - params.criticality) * tile_min_cost.at(to_physical_type->index).at(to_node_ptc_num).congestion;
            return delay_cost + cong_cost + delay_offset_cost + cong_offset_cost;
        } else {
            if (node_in_same_physical_tile(current_node, target_node)) {
                delay_offset_cost = 0.;
                cong_offset_cost = 0.;
                const auto& pin_delays = intra_tile_pin_primitive_pin_delay.at(from_physical_type->index)[from_node_ptc_num];
                auto pin_delay_itr = pin_delays.find(rr_graph.node_ptc_num(target_node));
                if (pin_delay_itr == pin_delays.end()) {
                    // There isn't any intra-cluster path to connect the current OPIN to the SINK, thus it has to outside.
                    // The best estimation we have now, it the minimum intra-cluster delay to the sink. However, this cost is incomplete,
                    // since it does not consider the cost of going outside of the cluster and, then, returning to it.
                    delay_cost = params.criticality * tile_min_cost.at(to_physical_type->index).at(to_node_ptc_num).delay;
                    cong_cost = (1. - params.criticality) * tile_min_cost.at(to_physical_type->index).at(to_node_ptc_num).congestion;
                    return delay_cost + cong_cost;
                } else {
                    delay_cost = params.criticality * pin_delay_itr->second.delay;
                    cong_cost = (1. - params.criticality) * pin_delay_itr->second.congestion;
                }
            } else {
                // Since we don't know which type of wires are accessible from an OPIN inside the cluster, we use
                // distance_based_min_cost to get an estimation of the global cost, and then, add this cost to the tile_min_cost
                // to have an estimation of the cost of getting into a cluster - We don't have any estimation of the cost to get out of the cluster
                auto [delta_x, delta_y] = util::get_xy_deltas(current_node, target_node);
                delta_x = abs(delta_x);
                delta_y = abs(delta_y);
                delay_cost = params.criticality * chann_distance_based_min_cost[rr_graph.node_layer(current_node)][to_layer_num][delta_x][delta_y].delay;
                cong_cost = (1. - params.criticality) * chann_distance_based_min_cost[rr_graph.node_layer(current_node)][to_layer_num][delta_x][delta_y].congestion;

                delay_offset_cost = params.criticality * tile_min_cost.at(to_physical_type->index).at(to_node_ptc_num).delay;
                cong_offset_cost = (1. - params.criticality) * tile_min_cost.at(to_physical_type->index).at(to_node_ptc_num).congestion;
            }
            return delay_cost + cong_cost + delay_offset_cost + cong_offset_cost;
        }
    } else if (from_rr_type == IPIN) {
        // we assume that route-through is not enabled.
        VTR_ASSERT(node_in_same_physical_tile(current_node, target_node));
        const auto& pin_delays = intra_tile_pin_primitive_pin_delay.at(from_physical_type->index)[from_node_ptc_num];
        auto pin_delay_itr = pin_delays.find(rr_graph.node_ptc_num(target_node));
        if (pin_delay_itr == pin_delays.end()) {
            delay_cost = std::numeric_limits<float>::max() / 1e12;
            cong_cost = std::numeric_limits<float>::max() / 1e12;
        } else {
            delay_cost = params.criticality * pin_delay_itr->second.delay;
            cong_cost = (1. - params.criticality) * pin_delay_itr->second.congestion;
        }
        return delay_cost + cong_cost;
    } else if (from_rr_type == SOURCE) {
        if (node_in_same_physical_tile(current_node, target_node)) {
            delay_cost = 0.;
            cong_cost = 0.;
            delay_offset_cost = 0.;
            cong_offset_cost = 0.;
        } else {
            auto [delta_x, delta_y] = util::get_xy_deltas(current_node, target_node);
            delta_x = abs(delta_x);
            delta_y = abs(delta_y);
            delay_cost = params.criticality * chann_distance_based_min_cost[rr_graph.node_layer(current_node)][to_layer_num][delta_x][delta_y].delay;
            cong_cost = (1. - params.criticality) * chann_distance_based_min_cost[rr_graph.node_layer(current_node)][to_layer_num][delta_x][delta_y].congestion;

            delay_offset_cost = params.criticality * tile_min_cost.at(to_physical_type->index).at(to_node_ptc_num).delay;
            cong_offset_cost = (1. - params.criticality) * tile_min_cost.at(to_physical_type->index).at(to_node_ptc_num).congestion;
        }
        return delay_cost + cong_cost + delay_offset_cost + cong_offset_cost;
    } else {
        VTR_ASSERT(from_rr_type == SINK);
        return (0.);
    }
}

/******** Function Definitions ********/
/* queries the lookahead_map (should have been computed prior to routing) to get the expected cost
 * from the specified source to the specified target */
std::pair<float, float> MapLookahead::get_expected_delay_and_cong(RRNodeId from_node, RRNodeId to_node, const t_conn_cost_params& params, float /*R_upstream*/) const {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_graph;

    int from_layer_num = rr_graph.node_layer(from_node);
    int to_layer_num = rr_graph.node_layer(to_node);
    auto [delta_x, delta_y] = util::get_xy_deltas(from_node, to_node);
    delta_x = abs(delta_x);
    delta_y = abs(delta_y);

    float expected_delay_cost = std::numeric_limits<float>::infinity();
    float expected_cong_cost = std::numeric_limits<float>::infinity();

    e_rr_type from_type = rr_graph.node_type(from_node);
    if (from_type == SOURCE || from_type == OPIN) {
        //When estimating costs from a SOURCE/OPIN we look-up to find which wire types (and the
        //cost to reach them) in src_opin_delays. Once we know what wire types are
        //reachable, we query the f_wire_cost_map (i.e. the wire lookahead) to get the final
        //delay to reach the sink.

        t_physical_tile_type_ptr from_tile_type = device_ctx.grid.get_physical_type({rr_graph.node_xlow(from_node),
                                                                                     rr_graph.node_ylow(from_node),
                                                                                     from_layer_num});

        auto from_tile_index = std::distance(&device_ctx.physical_tile_types[0], from_tile_type);

        auto from_ptc = rr_graph.node_ptc_num(from_node);

        std::tie(expected_delay_cost, expected_cong_cost) = util::get_cost_from_src_opin(src_opin_delays[from_layer_num][from_tile_index][from_ptc][to_layer_num],
                                                                                         delta_x,
                                                                                         delta_y,
                                                                                         to_layer_num,
                                                                                         get_wire_cost_entry);

        expected_delay_cost *= params.criticality;
        expected_cong_cost *= (1 - params.criticality);

        VTR_ASSERT_SAFE_MSG(std::isfinite(expected_delay_cost),
                            vtr::string_fmt("Lookahead failed to estimate cost from %s: %s",
                                            rr_node_arch_name(from_node, is_flat_).c_str(),
                                            describe_rr_node(rr_graph,
                                                             device_ctx.grid,
                                                             device_ctx.rr_indexed_data,
                                                             from_node,
                                                             is_flat_)
                                                .c_str())
                                .c_str());

    } else if (from_type == CHANX || from_type == CHANY) {
        //When estimating costs from a wire, we directly look-up the result in the wire lookahead (f_wire_cost_map)

        auto from_cost_index = rr_graph.node_cost_index(from_node);
        int from_seg_index = device_ctx.rr_indexed_data[from_cost_index].seg_index;

        VTR_ASSERT(from_seg_index >= 0);

        /* now get the expected cost from our lookahead map */
        util::Cost_Entry cost_entry = get_wire_cost_entry(from_type,
                                                          from_seg_index,
                                                          from_layer_num,
                                                          delta_x,
                                                          delta_y,
                                                          to_layer_num);
        expected_delay_cost = cost_entry.delay;
        expected_cong_cost = cost_entry.congestion;

        VTR_ASSERT_SAFE_MSG(std::isfinite(expected_delay_cost),
                            vtr::string_fmt("Lookahead failed to estimate cost from %s: %s",
                                            rr_node_arch_name(from_node, is_flat_).c_str(),
                                            describe_rr_node(rr_graph,
                                                             device_ctx.grid,
                                                             device_ctx.rr_indexed_data,
                                                             from_node,
                                                             is_flat_)
                                                .c_str())
                                .c_str());
        expected_delay_cost = cost_entry.delay * params.criticality;
        expected_cong_cost = cost_entry.congestion * (1 - params.criticality);
    } else if (from_type == IPIN) { /* Change if you're allowing route-throughs */
        return std::make_pair(0., device_ctx.rr_indexed_data[RRIndexedDataId(SINK_COST_INDEX)].base_cost);
    } else { /* Change this if you want to investigate route-throughs */
        return std::make_pair(0., 0.);
    }

    return std::make_pair(expected_delay_cost, expected_cong_cost);
}

void MapLookahead::compute(const std::vector<t_segment_inf>& segment_inf) {
    vtr::ScopedStartFinishTimer timer("Computing router lookahead map");

    //First compute the delay map when starting from the various wire types
    //(CHANX/CHANY)in the routing architecture
    compute_router_wire_lookahead(segment_inf);

    //Next, compute which wire types are accessible (and the cost to reach them)
    //from the different physical tile type's SOURCEs & OPINs
    this->src_opin_delays = util::compute_router_src_opin_lookahead(is_flat_);

    min_chann_global_cost_map(chann_distance_based_min_cost);
    min_opin_distance_cost_map(src_opin_delays, opin_distance_based_min_cost);
}

void MapLookahead::compute_intra_tile() {
    is_flat_ = true;
    vtr::ScopedStartFinishTimer timer("Computing tile lookahead");
    VTR_ASSERT(intra_tile_pin_primitive_pin_delay.empty());
    VTR_ASSERT(tile_min_cost.empty());

    compute_tiles_lookahead(intra_tile_pin_primitive_pin_delay,
                            tile_min_cost,
                            det_routing_arch_,
                            g_vpr_ctx.device());
}

void MapLookahead::read(const std::string& file) {
    read_router_lookahead(file);

    //Next, compute which wire types are accessible (and the cost to reach them)
    //from the different physical tile type's SOURCEs & OPINs
    this->src_opin_delays = util::compute_router_src_opin_lookahead(is_flat_);

    min_chann_global_cost_map(chann_distance_based_min_cost);
    min_opin_distance_cost_map(src_opin_delays, opin_distance_based_min_cost);
}

void MapLookahead::read_intra_cluster(const std::string& file) {
    vtr::ScopedStartFinishTimer timer("Loading router intra cluster lookahead map");
    is_flat_ = true;
    // Maps related to global resources should not be empty
    VTR_ASSERT(!f_wire_cost_map.empty());
    read_intra_cluster_router_lookahead(intra_tile_pin_primitive_pin_delay,
                                        file);

    const auto& tiles = g_vpr_ctx.device().physical_tile_types;
    for (const auto& tile : tiles) {
        if (is_empty_type(&tile)) {
            continue;
        }
        store_min_cost_to_sinks(tile_min_cost,
                                &tile,
                                intra_tile_pin_primitive_pin_delay);
    }
}

void MapLookahead::write(const std::string& file_name) const {
    if (vtr::check_file_name_extension(file_name, ".csv")) {
        std::vector<int> wire_cost_map_size(f_wire_cost_map.ndims());
        for (size_t i = 0; i < f_wire_cost_map.ndims(); ++i) {
            wire_cost_map_size[i] = static_cast<int>(f_wire_cost_map.dim_size(i));
        }
        dump_readable_router_lookahead_map(file_name, wire_cost_map_size, get_wire_cost_entry);
    } else {
        VTR_ASSERT(vtr::check_file_name_extension(file_name, ".capnp") || vtr::check_file_name_extension(file_name, ".bin"));
        write_router_lookahead(file_name);
    }
}

void MapLookahead::write_intra_cluster(const std::string& file) const {
    write_intra_cluster_router_lookahead(file,
                                         intra_tile_pin_primitive_pin_delay);
}

float MapLookahead::get_opin_distance_min_delay(int physical_tile_idx, int from_layer, int to_layer, int dx, int dy) const {
    return opin_distance_based_min_cost[physical_tile_idx][from_layer][to_layer][dx][dy].delay;
}

/******** Function Definitions ********/

static util::Cost_Entry get_wire_cost_entry(e_rr_type rr_type, int seg_index, int from_layer_num, int delta_x, int delta_y, int to_layer_num) {
    VTR_ASSERT_SAFE(rr_type == CHANX || rr_type == CHANY);

    int chan_index = 0;
    if (rr_type == CHANY) {
        chan_index = 1;
    }

    VTR_ASSERT_SAFE(from_layer_num < (int)f_wire_cost_map.dim_size(0));
    VTR_ASSERT_SAFE(to_layer_num < (int)f_wire_cost_map.dim_size(3));
    VTR_ASSERT_SAFE(delta_x < (int)f_wire_cost_map.dim_size(4));
    VTR_ASSERT_SAFE(delta_y < (int)f_wire_cost_map.dim_size(5));

    return f_wire_cost_map[from_layer_num][chan_index][seg_index][to_layer_num][delta_x][delta_y];
}

static void compute_router_wire_lookahead(const std::vector<t_segment_inf>& segment_inf_vec) {
    vtr::ScopedStartFinishTimer timer("Computing wire lookahead");

    auto& device_ctx = g_vpr_ctx.device();

    auto& grid = device_ctx.grid;

    //Re-allocate
    f_wire_cost_map = t_wire_cost_map({static_cast<unsigned long>(grid.get_num_layers()),
                                       2,
                                       segment_inf_vec.size(),
                                       static_cast<unsigned long>(grid.get_num_layers()),
                                       device_ctx.grid.width(),
                                       device_ctx.grid.height()});

    int longest_seg_length = 0;
    for (const auto& seg_inf : segment_inf_vec) {
        longest_seg_length = std::max(longest_seg_length, seg_inf.length);
    }

    //Profile each wire segment type
    for (int from_layer_num = 0; from_layer_num < grid.get_num_layers(); from_layer_num++) {
        for (const auto& segment_inf : segment_inf_vec) {
            std::vector<e_rr_type> chan_types;
            if (segment_inf.parallel_axis == X_AXIS)
                chan_types.push_back(CHANX);
            else if (segment_inf.parallel_axis == Y_AXIS)
                chan_types.push_back(CHANY);
            else //Both for BOTH_AXIS segments and special segments such as clock_networks we want to search in both directions.
                chan_types.insert(chan_types.end(), {CHANX, CHANY});

            for (e_rr_type chan_type : chan_types) {
                util::t_routing_cost_map routing_cost_map = util::get_routing_cost_map(longest_seg_length,
                                                                                       from_layer_num,
                                                                                       chan_type,
                                                                                       segment_inf,
                                                                                       std::unordered_map<int, std::unordered_set<int>>(),
                                                                                       true);
                if (routing_cost_map.empty()) {
                    continue;
                }

                /* boil down the cost list in routing_cost_map at each coordinate to a representative cost entry and store it in the lookahead
                 * cost map */
                set_lookahead_map_costs(from_layer_num, segment_inf.seg_index, chan_type, routing_cost_map);

                /* fill in missing entries in the lookahead cost map by copying the closest cost entries (cost map was computed based on
                 * a reference coordinate > (0,0) so some entries that represent a cross-chip distance have not been computed) */
                fill_in_missing_lookahead_entries(segment_inf.seg_index, chan_type);
            }
        }
    }
}

/* sets the lookahead cost map entries based on representative cost entries from routing_cost_map */
static void set_lookahead_map_costs(int from_layer_num, int segment_index, e_rr_type chan_type, util::t_routing_cost_map& routing_cost_map) {
    int chan_index = (chan_type == CHANX) ? 0 : 1;

    /* set the lookahead cost map entries with a representative cost entry from routing_cost_map */
    for (unsigned to_layer = 0; to_layer < routing_cost_map.dim_size(0); to_layer++) {
        for (unsigned ix = 0; ix < routing_cost_map.dim_size(1); ix++) {
            for (unsigned iy = 0; iy < routing_cost_map.dim_size(2); iy++) {
                util::Expansion_Cost_Entry& expansion_cost_entry = routing_cost_map[to_layer][ix][iy];

                f_wire_cost_map[from_layer_num][chan_index][segment_index][to_layer][ix][iy] = expansion_cost_entry.get_representative_cost_entry(util::e_representative_entry_method::SMALLEST);
            }
        }
    }
}

/* fills in missing lookahead map entries by copying the cost of the closest valid entry */
static void fill_in_missing_lookahead_entries(int segment_index, e_rr_type chan_type) {
    int chan_index = (chan_type == CHANX) ? 0 : 1;

    auto& device_ctx = g_vpr_ctx.device();

    /* find missing cost entries and fill them in by copying a nearby cost entry */
    for (int from_layer_num = 0; from_layer_num < device_ctx.grid.get_num_layers(); from_layer_num++) {
        for (int to_layer_num = 0; to_layer_num < device_ctx.grid.get_num_layers(); ++to_layer_num) {
            for (unsigned ix = 0; ix < device_ctx.grid.width(); ix++) {
                for (unsigned iy = 0; iy < device_ctx.grid.height(); iy++) {
                    util::Cost_Entry cost_entry = f_wire_cost_map[from_layer_num][chan_index][segment_index][to_layer_num][ix][iy];

                    if (std::isnan(cost_entry.delay) && std::isnan(cost_entry.congestion)) {
                        util::Cost_Entry copied_entry = get_nearby_cost_entry_average_neighbour(from_layer_num,
                                                                                                static_cast<int>(ix),
                                                                                                static_cast<int>(iy),
                                                                                                to_layer_num,
                                                                                                segment_index,
                                                                                                chan_index);
                        f_wire_cost_map[from_layer_num][chan_index][segment_index][to_layer_num][ix][iy] = copied_entry;
                    }
                }
            }
        }
    }
}

/* returns a cost entry in the f_wire_cost_map that is near the specified coordinates (and preferably towards (0,0)) */
static util::Cost_Entry get_nearby_cost_entry(int from_layer_num, int x, int y, int to_layer_num, int segment_index, int chan_index) {
    /* compute the slope from x,y to 0,0 and then move towards 0,0 by one unit to get the coordinates
     * of the cost entry to be copied */

    //VTR_ASSERT(x > 0 || y > 0); //Asertion fails in practise. TODO: debug

    float slope;
    if (x == 0) {
        slope = 1e12; //just a really large number
    } else if (y == 0) {
        slope = 1e-12; //just a really small number
    } else {
        slope = (float)y / (float)x;
    }

    int copy_x, copy_y;
    if (slope >= 1.0) {
        copy_y = y - 1;
        copy_x = vtr::nint((float)y / slope);
    } else {
        copy_x = x - 1;
        copy_y = vtr::nint((float)x * slope);
    }

    copy_y = std::max(copy_y, 0); //Clip to zero
    copy_x = std::max(copy_x, 0); //Clip to zero

    util::Cost_Entry copy_entry = f_wire_cost_map[from_layer_num][chan_index][segment_index][to_layer_num][copy_x][copy_y];

    /* if the entry to be copied is also empty, recurse */
    if (std::isnan(copy_entry.delay) && std::isnan(copy_entry.congestion)) {
        if (copy_x == 0 && copy_y == 0) {
            copy_entry = util::Cost_Entry(0., 0.); //(0, 0) entry is invalid so set zero to terminate recursion
            // set zero if the source and sink nodes are on the same layer. If they are not, it means that there is no connection from the source node to
            // the other layer. This means that the connection should be set to a very large number
            if (from_layer_num == to_layer_num) {
                copy_entry = util::Cost_Entry(0., 0.);
            } else {
                copy_entry = util::Cost_Entry(std::numeric_limits<float>::max() / 1e12, std::numeric_limits<float>::max() / 1e12);
            }
        } else {
            copy_entry = get_nearby_cost_entry(from_layer_num, copy_x, copy_y, to_layer_num, segment_index, chan_index);
        }
    }

    return copy_entry;
}

static util::Cost_Entry get_nearby_cost_entry_average_neighbour(int from_layer_num,
                                                                int missing_dx,
                                                                int missing_dy,
                                                                int to_layer_num,
                                                                int segment_index,
                                                                int chan_index) {
    // Make sure that the given location doesn't have a valid entry
    VTR_ASSERT(std::isnan(f_wire_cost_map[from_layer_num][chan_index][segment_index][to_layer_num][missing_dx][missing_dy].delay));
    VTR_ASSERT(std::isnan(f_wire_cost_map[from_layer_num][chan_index][segment_index][to_layer_num][missing_dx][missing_dy].congestion));

    int neighbour_num = 0;                  // Number of neighbours with valid entry
    float neighbour_delay_sum = 0;          // Acc of valid delay costs
    float neighbour_cong_sum = 0;           // Acc of valid congestion costs
    std::array<int, 3> window = {-1, 0, 1}; // Average window size
    for (int dx : window) {
        int neighbour_x = missing_dx + dx;
        if (neighbour_x < 0 || neighbour_x >= (int)f_wire_cost_map.dim_size(4)) {
            continue;
        }
        for (int dy : window) {
            int neighbour_y = missing_dy + dy;
            if (neighbour_y < 0 || neighbour_y >= (int)f_wire_cost_map.dim_size(5)) {
                continue;
            }
            util::Cost_Entry copy_entry = f_wire_cost_map[from_layer_num][chan_index][segment_index][to_layer_num][neighbour_x][neighbour_y];
            if (std::isnan(copy_entry.delay) || std::isnan(copy_entry.congestion)) {
                continue;
            }
            neighbour_delay_sum += copy_entry.delay;
            neighbour_cong_sum += copy_entry.congestion;
            neighbour_num += 1;
        }
    }

    // Store the average only if there are enough number of neighbours with valid entry
    if (neighbour_num >= VALID_NEIGHBOR_NUMBER) {
        return {neighbour_delay_sum / static_cast<float>(neighbour_num),
                neighbour_cong_sum / static_cast<float>(neighbour_num)};
    } else {
        // If there are not enough neighbours with valid entry, retrieve to the previous way of getting the missing cost
        return get_nearby_cost_entry(from_layer_num, missing_dx, missing_dy, to_layer_num, segment_index, chan_index);
    }
}

static void compute_tiles_lookahead(std::unordered_map<int, util::t_ipin_primitive_sink_delays>& intra_tile_pin_primitive_pin_delay,
                                    std::unordered_map<int, std::unordered_map<int, util::Cost_Entry>>& tile_min_cost,
                                    const t_det_routing_arch& det_routing_arch,
                                    const DeviceContext& device_ctx) {
    const auto& tiles = device_ctx.physical_tile_types;

    for (const auto& tile : tiles) {
        if (is_empty_type(&tile)) {
            continue;
        }

        compute_tile_lookahead(intra_tile_pin_primitive_pin_delay,
                               &tile,
                               det_routing_arch,
                               device_ctx.delayless_switch_idx);
        store_min_cost_to_sinks(tile_min_cost,
                                &tile,
                                intra_tile_pin_primitive_pin_delay);
    }
}

static void compute_tile_lookahead(std::unordered_map<int, util::t_ipin_primitive_sink_delays>& intra_tile_pin_primitive_pin_delay,
                                   t_physical_tile_type_ptr physical_tile,
                                   const t_det_routing_arch& det_routing_arch,
                                   const int delayless_switch) {
    RRGraphBuilder rr_graph_builder;
    int layer = 0;
    int x = 1;
    int y = 1;
    build_tile_rr_graph(rr_graph_builder,
                        det_routing_arch,
                        physical_tile,
                        layer,
                        x,
                        y,
                        delayless_switch);

    RRGraphView rr_graph{rr_graph_builder.rr_nodes(),
                         rr_graph_builder.node_lookup(),
                         rr_graph_builder.rr_node_metadata(),
                         rr_graph_builder.rr_edge_metadata(),
                         g_vpr_ctx.device().rr_indexed_data,
                         g_vpr_ctx.device().rr_rc_data,
                         rr_graph_builder.rr_segments(),
                         rr_graph_builder.rr_switch()};

    util::t_ipin_primitive_sink_delays pin_delays = util::compute_intra_tile_dijkstra(rr_graph,
                                                                                      physical_tile,
                                                                                      layer,
                                                                                      x,
                                                                                      y);

    auto insert_res = intra_tile_pin_primitive_pin_delay.insert(std::make_pair(physical_tile->index, pin_delays));
    VTR_ASSERT(insert_res.second);

    rr_graph_builder.clear();
}

static void store_min_cost_to_sinks(std::unordered_map<int, std::unordered_map<int, util::Cost_Entry>>& tile_min_cost,
                                    t_physical_tile_type_ptr physical_tile,
                                    const std::unordered_map<int, util::t_ipin_primitive_sink_delays>& intra_tile_pin_primitive_pin_delay) {
    const auto& tile_pin_delays = intra_tile_pin_primitive_pin_delay.at(physical_tile->index);
    std::unordered_map<int, util::Cost_Entry> min_cost_map;
    for (auto& primitive_sink_pair : physical_tile->primitive_class_inf) {
        int primitive_sink = primitive_sink_pair.first;
        auto min_cost = util::Cost_Entry(std::numeric_limits<float>::max() / 1e12,
                                         std::numeric_limits<float>::max() / 1e12);

        for (int pin_physical_num = 0; pin_physical_num < physical_tile->num_pins; pin_physical_num++) {
            if (get_pin_type_from_pin_physical_num(physical_tile, pin_physical_num) != e_pin_type::RECEIVER) {
                continue;
            }
            const auto& pin_delays = tile_pin_delays[pin_physical_num];
            if (pin_delays.find(primitive_sink) != pin_delays.end()) {
                auto pin_cost = pin_delays.at(primitive_sink);
                if (pin_cost.delay < min_cost.delay) {
                    min_cost = pin_cost;
                }
            }
        }
        auto insert_res = min_cost_map.insert(std::make_pair(primitive_sink, min_cost));
        VTR_ASSERT(insert_res.second);
    }

    auto insert_res = tile_min_cost.insert(std::make_pair(physical_tile->index, min_cost_map));
    VTR_ASSERT(insert_res.second);
}

static void min_chann_global_cost_map(vtr::NdMatrix<util::Cost_Entry, 4>& distance_min_cost) {
    int num_layers = g_vpr_ctx.device().grid.get_num_layers();
    int width = (int)g_vpr_ctx.device().grid.width();
    int height = (int)g_vpr_ctx.device().grid.height();
    distance_min_cost.resize({static_cast<unsigned long>(num_layers),
                              static_cast<unsigned long>(num_layers),
                              static_cast<unsigned long>(width),
                              static_cast<unsigned long>(height)});

    for (int from_layer_num = 0; from_layer_num < num_layers; from_layer_num++) {
        for (int to_layer_num = 0; to_layer_num < num_layers; to_layer_num++) {
            for (int dx = 0; dx < width; dx++) {
                for (int dy = 0; dy < height; dy++) {
                    util::Cost_Entry min_cost(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
                    for (int chan_idx = 0; chan_idx < (int)f_wire_cost_map.dim_size(1); chan_idx++) {
                        for (int seg_idx = 0; seg_idx < (int)f_wire_cost_map.dim_size(2); seg_idx++) {
                            auto cost = util::Cost_Entry(f_wire_cost_map[from_layer_num][chan_idx][seg_idx][to_layer_num][dx][dy].delay,
                                                         f_wire_cost_map[from_layer_num][chan_idx][seg_idx][to_layer_num][dx][dy].congestion);
                            if (cost.delay < min_cost.delay) {
                                min_cost.delay = cost.delay;
                                min_cost.congestion = cost.congestion;
                            }
                        }
                    }
                    distance_min_cost[from_layer_num][to_layer_num][dx][dy] = min_cost;
                }
            }
        }
    }
}

static void min_opin_distance_cost_map(const util::t_src_opin_delays& src_opin_delays, vtr::NdMatrix<util::Cost_Entry, 5>& distance_min_cost) {
    /**
     * This function calculates and stores the minimum cost to reach a point on layer `n_sink`, which is `dx` and `dy` further from the current point
     * on layer `n_source` and is located on physical tile type `t`. To compute this cost, the function iterates over all output pins of tile `t`,
     * and for each pin, iterates over all segment types accessible by it. It then determines and stores the minimum cost to the destination point.
     * "src_opin_delays" stores the routing segments accessible by each OPIN of each physical type on each layer. After getting the accessible segment types,
     * "get_wire_cost_entry" is called to get the cost from that segment type to the destination point.
     */
    int num_tile_types = g_vpr_ctx.device().physical_tile_types.size();
    int num_layers = g_vpr_ctx.device().grid.get_num_layers();
    int width = (int)g_vpr_ctx.device().grid.width();
    int height = (int)g_vpr_ctx.device().grid.height();
    distance_min_cost.resize({static_cast<unsigned long>(num_tile_types),
                              static_cast<unsigned long>(num_layers),
                              static_cast<unsigned long>(num_layers),
                              static_cast<unsigned long>(width),
                              static_cast<unsigned long>(height)});

    for (int tile_type_idx = 0; tile_type_idx < num_tile_types; tile_type_idx++) {
        for (int from_layer_num = 0; from_layer_num < num_layers; from_layer_num++) {
            for (int to_layer_num = 0; to_layer_num < num_layers; to_layer_num++) {
                for (int dx = 0; dx < width; dx++) {
                    for (int dy = 0; dy < height; dy++) {
                        util::Cost_Entry min_cost(std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
                        for (const auto& tile_opin_map : src_opin_delays[from_layer_num][tile_type_idx]) {
                            float expected_delay_cost = std::numeric_limits<float>::infinity();
                            float expected_cong_cost = std::numeric_limits<float>::infinity();

                            for (const auto& layer_src_opin_delay_map : tile_opin_map) {
                                float layer_expected_delay_cost = std::numeric_limits<float>::infinity();
                                float layer_expected_cong_cost = std::numeric_limits<float>::infinity();
                                if (layer_src_opin_delay_map.empty()) {
                                    layer_expected_delay_cost = std::numeric_limits<float>::max() / 1e12;
                                    layer_expected_cong_cost = std::numeric_limits<float>::max() / 1e12;
                                } else {
                                    for (const auto& kv : layer_src_opin_delay_map) {
                                        const util::t_reachable_wire_inf& reachable_wire_inf = kv.second;
                                        if (reachable_wire_inf.wire_rr_type == SINK) {
                                            continue;
                                        }
                                        util::Cost_Entry wire_cost_entry;

                                        wire_cost_entry = get_wire_cost_entry(reachable_wire_inf.wire_rr_type,
                                                                              reachable_wire_inf.wire_seg_index,
                                                                              reachable_wire_inf.layer_number,
                                                                              dx,
                                                                              dy,
                                                                              to_layer_num);

                                        float this_delay_cost = reachable_wire_inf.delay + wire_cost_entry.delay;
                                        float this_cong_cost = reachable_wire_inf.congestion + wire_cost_entry.congestion;

                                        layer_expected_delay_cost = std::min(layer_expected_delay_cost, this_delay_cost);
                                        layer_expected_cong_cost = std::min(layer_expected_cong_cost, this_cong_cost);
                                    }
                                }
                                expected_delay_cost = std::min(expected_delay_cost, layer_expected_delay_cost);
                                expected_cong_cost = std::min(expected_cong_cost, layer_expected_cong_cost);
                            }

                            if (expected_delay_cost < min_cost.delay) {
                                min_cost.delay = expected_delay_cost;
                                min_cost.congestion = expected_cong_cost;
                            }
                        }
                        distance_min_cost[tile_type_idx][from_layer_num][to_layer_num][dx][dy] = min_cost;
                    }
                }
            }
        }
    }
}

//
// When writing capnp targetted serialization, always allow compilation when
// VTR_ENABLE_CAPNPROTO=OFF.  Generally this means throwing an exception
// instead.
//
#ifndef VTR_ENABLE_CAPNPROTO

#    define DISABLE_ERROR                               \
        "is disabled because VTR_ENABLE_CAPNPROTO=OFF." \
        "Re-compile with CMake option VTR_ENABLE_CAPNPROTO=ON to enable."

void read_router_lookahead(const std::string& /*file*/) {
    VPR_THROW(VPR_ERROR_PLACE, "MapLookahead::read_router_lookahead " DISABLE_ERROR);
}

void write_router_lookahead(const std::string& /*file*/) {
    VPR_THROW(VPR_ERROR_PLACE, "MapLookahead::write_router_lookahead " DISABLE_ERROR);
}

static void read_intra_cluster_router_lookahead(std::unordered_map<int, util::t_ipin_primitive_sink_delays>& /*intra_tile_pin_primitive_pin_delay*/,
                                                const std::string& /*file*/) {
    VPR_THROW(VPR_ERROR_PLACE, "MapLookahead::read_intra_cluster_router_lookahead " DISABLE_ERROR);
}

static void write_intra_cluster_router_lookahead(const std::string& /*file*/,
                                                 const std::unordered_map<int, util::t_ipin_primitive_sink_delays>& /*intra_tile_pin_primitive_pin_delay*/) {
    VPR_THROW(VPR_ERROR_PLACE, "MapLookahead::write_intra_cluster_router_lookahead " DISABLE_ERROR);
}

#else /* VTR_ENABLE_CAPNPROTO */

static void ToCostEntry(util::Cost_Entry* out, const VprMapCostEntry::Reader& in) {
    out->delay = in.getDelay();
    out->congestion = in.getCongestion();
}

static void FromCostEntry(VprMapCostEntry::Builder* out, const util::Cost_Entry& in) {
    out->setDelay(in.delay);
    out->setCongestion(in.congestion);
}

static void toIntEntry(std::vector<int>& out,
                       int idx,
                       const int& cost) {
    out[idx] = cost;
}

static void fromIntEntry(::capnp::List<int64_t, ::capnp::Kind::PRIMITIVE>::Builder& out,
                         int idx,
                         const int& cost) {
    out.set(idx, cost);
}

static void toPairEntry(std::unordered_map<int, util::Cost_Entry>& map_out,
                        const int& key,
                        const VprMapCostEntry::Reader& cap_cost) {
    VTR_ASSERT(map_out.find(key) == map_out.end());
    util::Cost_Entry cost(cap_cost.getDelay(), cap_cost.getCongestion());
    map_out[key] = cost;
}

static void fromPairEntry(::capnp::List<int64_t, ::capnp::Kind::PRIMITIVE>::Builder& out_key,
                          ::capnp::List<::VprMapCostEntry, ::capnp::Kind::STRUCT>::Builder& out_val,
                          int flat_idx,
                          const int& key,
                          const util::Cost_Entry& cost) {
    out_key.set(flat_idx, key);
    out_val[flat_idx].setDelay(cost.delay);
    out_val[flat_idx].setCongestion(cost.congestion);
}

static void getIntraClusterArrayFlatSize(int& num_tile_types,
                                         int& num_pins,
                                         int& num_sinks,
                                         const std::unordered_map<int, util::t_ipin_primitive_sink_delays>& intra_tile_pin_primitive_pin_delay) {
    const auto& physical_tile_types = g_vpr_ctx.device().physical_tile_types;
    num_tile_types = (int)physical_tile_types.size();

    num_pins = 0;
    for (const auto& tile_type : intra_tile_pin_primitive_pin_delay) {
        num_pins += (int)tile_type.second.size();
    }

    num_sinks = 0;
    for (const auto& tile_type : intra_tile_pin_primitive_pin_delay) {
        for (const auto& pin_sink : tile_type.second) {
            num_sinks += (int)pin_sink.size();
        }
    }
}

static void read_intra_cluster_router_lookahead(std::unordered_map<int, util::t_ipin_primitive_sink_delays>& intra_tile_pin_primitive_pin_delay,
                                                const std::string& file) {
    MmapFile f(file);

    /* Increase reader limit to 1G words to allow for large files. */
    ::capnp::ReaderOptions opts = default_large_capnp_opts();
    ::capnp::FlatArrayMessageReader reader(f.getData(), opts);

    auto map = reader.getRoot<VprIntraClusterLookahead>();

    std::vector<int> physical_tile_num_pin_arr;
    toVector<int64_t, int>(physical_tile_num_pin_arr,
                           map.getPhysicalTileNumPins(),
                           toIntEntry);

    std::vector<int> pin_num_sink_arr;
    toVector<int64_t, int>(pin_num_sink_arr,
                           map.getPinNumSinks(),
                           toIntEntry);

    int num_seen_pair = 0;
    int num_seen_pin = 0;
    for (int physical_tile_idx = 0; physical_tile_idx < (int)physical_tile_num_pin_arr.size(); physical_tile_idx++) {
        int num_pins = physical_tile_num_pin_arr[physical_tile_idx];
        util::t_ipin_primitive_sink_delays tile_pin_sink_cost_map(num_pins);

        for (int pin_num = 0; pin_num < num_pins; pin_num++) {
            std::unordered_map<int, util::Cost_Entry> pin_sink_cost_map;
            toUnorderedMap<int64_t, VprMapCostEntry, int, util::Cost_Entry>(pin_sink_cost_map,
                                                                            num_seen_pair,
                                                                            num_seen_pair + pin_num_sink_arr[num_seen_pin],
                                                                            map.getPinSinks(),
                                                                            map.getPinSinkCosts(),
                                                                            toPairEntry);
            tile_pin_sink_cost_map[pin_num] = pin_sink_cost_map;
            num_seen_pair += (int)pin_sink_cost_map.size();
            VTR_ASSERT((int)pin_sink_cost_map.size() == pin_num_sink_arr[num_seen_pin]);
            ++num_seen_pin;
        }
        intra_tile_pin_primitive_pin_delay[physical_tile_idx] = tile_pin_sink_cost_map;
    }
}

static void write_intra_cluster_router_lookahead(const std::string& file,
                                                 const std::unordered_map<int, util::t_ipin_primitive_sink_delays>& intra_tile_pin_primitive_pin_delay) {
    ::capnp::MallocMessageBuilder builder;

    auto vpr_intra_cluster_lookahead_builder = builder.initRoot<VprIntraClusterLookahead>();

    int num_tile_types, num_pins, num_sinks;
    getIntraClusterArrayFlatSize(num_tile_types,
                                 num_pins,
                                 num_sinks,
                                 intra_tile_pin_primitive_pin_delay);

    std::vector<int> physical_tile_num_pin_arr(num_tile_types, 0);
    {
        for (const auto& physical_type : intra_tile_pin_primitive_pin_delay) {
            int physical_type_idx = physical_type.first;
            physical_tile_num_pin_arr[physical_type_idx] = (int)physical_type.second.size();
        }

        ::capnp::List<int64_t>::Builder physical_tile_num_pin_arr_builder = vpr_intra_cluster_lookahead_builder.initPhysicalTileNumPins(num_tile_types);
        fromVector<int64_t, int>(physical_tile_num_pin_arr_builder,
                                 physical_tile_num_pin_arr,
                                 fromIntEntry);
    }

    std::vector<int> pin_num_sink_arr(num_pins, 0);
    {
        int num_seen_pin = 0;
        for (int physical_tile_idx = 0; physical_tile_idx < num_tile_types; ++physical_tile_idx) {
            if (intra_tile_pin_primitive_pin_delay.find(physical_tile_idx) == intra_tile_pin_primitive_pin_delay.end()) {
                continue;
            }
            for (const auto& pin_sinks : intra_tile_pin_primitive_pin_delay.at(physical_tile_idx)) {
                pin_num_sink_arr[num_seen_pin] = (int)pin_sinks.size();
                ++num_seen_pin;
            }
        }
        ::capnp::List<int64_t>::Builder pin_num_sink_arr_builder = vpr_intra_cluster_lookahead_builder.initPinNumSinks(num_pins);
        fromVector<int64_t, int>(pin_num_sink_arr_builder,
                                 pin_num_sink_arr,
                                 fromIntEntry);
    }

    {
        ::capnp::List<int64_t>::Builder pin_sink_arr_builder = vpr_intra_cluster_lookahead_builder.initPinSinks(num_sinks);
        ::capnp::List<VprMapCostEntry>::Builder pin_sink_cost_builder = vpr_intra_cluster_lookahead_builder.initPinSinkCosts(num_sinks);

        int num_seen_pin = 0;
        for (int physical_tile_idx = 0; physical_tile_idx < num_tile_types; ++physical_tile_idx) {
            for (int pin_num = 0; pin_num < physical_tile_num_pin_arr[physical_tile_idx]; ++pin_num) {
                const std::unordered_map<int, util::Cost_Entry>& pin_sinks = intra_tile_pin_primitive_pin_delay.at(physical_tile_idx).at(pin_num);
                FromUnorderedMap<int64_t, VprMapCostEntry, int, util::Cost_Entry>(
                    pin_sink_arr_builder,
                    pin_sink_cost_builder,
                    num_seen_pin,
                    pin_sinks,
                    fromPairEntry);
                num_seen_pin += (int)pin_sinks.size();
            }
        }
    }

    writeMessageToFile(file, &builder);
}

void read_router_lookahead(const std::string& file) {
    vtr::ScopedStartFinishTimer timer("Loading router wire lookahead map");
    MmapFile f(file);

    /* Increase reader limit to 1G words to allow for large files. */
    ::capnp::ReaderOptions opts = default_large_capnp_opts();
    ::capnp::FlatArrayMessageReader reader(f.getData(), opts);

    auto map = reader.getRoot<VprMapLookahead>();

    ToNdMatrix<6, VprMapCostEntry, util::Cost_Entry>(&f_wire_cost_map, map.getCostMap(), ToCostEntry);
}

void write_router_lookahead(const std::string& file) {
    ::capnp::MallocMessageBuilder builder;

    auto map = builder.initRoot<VprMapLookahead>();

    auto cost_map = map.initCostMap();
    FromNdMatrix<6, VprMapCostEntry, util::Cost_Entry>(&cost_map, f_wire_cost_map, FromCostEntry);

    writeMessageToFile(file, &builder);
}

#endif
