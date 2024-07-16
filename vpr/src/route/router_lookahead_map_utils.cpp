#include "router_lookahead_map_utils.h"

/** @file This file contains utility functions that can be shared among different
 * lookahead computation strategies.
 *
 * In general, this utility library contains:
 *
 * - Different dijkstra expansion algorithms used to perform specific tasks, such as computing the SOURCE/OPIN --> CHAN lookup tables
 * - Cost Entries definitions used when generating and querying the lookahead
 *
 * To access the utility functions, the util namespace needs to be used. */

#include <fstream>
#include "globals.h"
#include "vpr_context.h"
#include "vtr_math.h"
#include "vtr_time.h"
#include "route_common.h"
#include "route_debug.h"

/**
 * We will profile delay/congestion using this many tracks for each wire type.
 * Larger values increase the time to compute the lookahead, but may give
 * more accurate lookahead estimates during routing.
 */
static constexpr int MAX_TRACK_OFFSET = 16;

static void dijkstra_flood_to_wires(int itile, RRNodeId inode, util::t_src_opin_delays& src_opin_delays);

static void dijkstra_flood_to_ipins(RRNodeId node, util::t_chan_ipins_delays& chan_ipins_delays);

/**
 * @param itile
 * @return Return the maximum ptc number of the SOURCE/OPINs of a tile type
 */
static int get_tile_src_opin_max_ptc(int itile);

static t_physical_tile_loc pick_sample_tile(int layer_num, t_physical_tile_type_ptr tile_type, t_physical_tile_loc prev);

static void run_intra_tile_dijkstra(const RRGraphView& rr_graph,
                                    util::t_ipin_primitive_sink_delays& pin_delays,
                                    t_physical_tile_type_ptr physical_tile,
                                    RRNodeId starting_node_id);

/* runs Dijkstra's algorithm from specified node until all nodes have been visited. Each time a pin is visited, the delay/congestion information
 * to that pin is stored is added to an entry in the routing_cost_map */
static void run_dijkstra(RRNodeId start_node,
                         int start_x,
                         int start_y,
                         util::t_routing_cost_map& routing_cost_map,
                         util::t_dijkstra_data& data,
                         const std::unordered_map<int, std::unordered_set<int>>& sample_locs,
                         bool sample_all_locs);

/* iterates over the children of the specified node and selectively pushes them onto the priority queue */
static void expand_dijkstra_neighbours(util::PQ_Entry parent_entry,
                                       vtr::vector<RRNodeId, float>& node_visited_costs,
                                       vtr::vector<RRNodeId, bool>& node_expanded,
                                       std::priority_queue<util::PQ_Entry>& pq);


/**
 * @brief Computes the adjusted position of an RR graph node.
 * This function does not modify the position of the given node.
 * It only returns the computed adjusted position.
 * @param rr The ID of the node whose adjusted position is desired.
 * @return The adjusted position (x, y).
 */
static std::pair<int, int> get_adjusted_rr_position(RRNodeId rr);

/**
 * @brief Computes the adjusted location of a pin to match the position of
 * the channel it can reach based on which side of the block it is at.
 * @param rr The corresponding node of a pin whose adjusted positions
 * is desired.
 * @return The adjusted position (x, y).
 */
static std::pair<int, int> get_adjusted_rr_pin_position(RRNodeId rr);

/**
 * @brief Computed the adjusted position of a node of type
 * CHANX or CHANY. For uni-directional wires, return the position
 * of the driver, and for bi-directional wires, compute the middle point.
 * @param rr The ID of the node whose adjusted position is desired.
 * @return The adjusted position (x, y).
 */
static std::pair<int, int> get_adjusted_rr_wire_position(RRNodeId rr);

/**
 * @brief Computes the adjusted position and source and sink nodes.
 * SOURCE/SINK nodes assume the full dimensions of their associated block/
 * This function computes the average position for the given node.
 * @param rr SOURCE or SINK node whose adjusted position is needed.
 * @return The adjusted position (x, y).
 */
static std::pair<int, int> get_adjusted_rr_src_sink_position(RRNodeId rr);

// Constants needed to reduce the bounding box when expanding CHAN wires to reach the IPINs.
// These are used when finding all the delays to get to the IPINs of all the different tile types
// of the device.
//
// The node expansions to get to the center of the bounding box (where the IPINs are located) start from
// the edge of the bounding box defined by these constants.
//
// E.g.: IPIN locations is found at (3,5). A bounding box around (3, 5) is generated with the following
//       corners.
//          x_min: 1 (x - X_OFFSET)
//          x_max: 5 (x + X_OFFSET)
//          y_min: 3 (y - Y_OFFSET)
//          y_max: 7 (y + Y_OFFSET)
#define X_OFFSET 2
#define Y_OFFSET 2

// Maximum dijkstra expansions when exploring the CHAN --> IPIN connections
// This sets a limit on the dijkstra expansions to reach an IPIN connection. Given that we
// want to have the data on the last delay to get to an IPIN, we do not want to have an unbounded
// number of hops to get to the IPIN, as this would result in high run-times.
//
// E.g.: if the constant value is set to 2, the following expansions are perfomed:
//          - CHANX --> CHANX --> exploration interrupted: Maximum expansion level reached
//          - CHANX --> IPIN --> exploration interrupted: IPIN found, no need to expand further
#define MAX_EXPANSION_LEVEL 1

// The special segment type index used to identify a direct connection between an OPIN to IPIN that
// does not go through the CHANX/CHANY nodes.
//
// This is used when building the SROURCE/OPIN --> CHAN lookup table that contains additional delay and
// congestion data to reach the CHANX/CHANY nodes which is not present in the lookahead cost map.
static constexpr int DIRECT_CONNECT_SPECIAL_SEG_TYPE = -1;

namespace util {

PQ_Entry::PQ_Entry(RRNodeId set_rr_node, int /*switch_ind*/, float parent_delay, float parent_R_upstream, float parent_congestion_upstream, bool starting_node) {
    this->rr_node = set_rr_node;

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    this->delay = parent_delay;
    this->congestion_upstream = parent_congestion_upstream;
    this->R_upstream = parent_R_upstream;
    if (!starting_node) {
        auto cost_index = rr_graph.node_cost_index(RRNodeId(set_rr_node));
        //this->delay += rr_graph.node_C(RRNodeId(set_rr_node)) * (g_rr_switch_inf[switch_ind].R + 0.5*rr_graph.node_R(RRNodeId(set_rr_node))) +
        //              g_rr_switch_inf[switch_ind].Tdel;

        //FIXME going to use the delay data that the VPR7 lookahead uses. For some reason the delay calculation above calculates
        //    a value that's just a little smaller compared to what VPR7 lookahead gets. While the above calculation should be more accurate,
        //    I have found that it produces the same CPD results but with worse runtime.
        //
        //    TODO: figure out whether anything's wrong with the calculation above and use that instead. If not, add the other
        //          terms like T_quadratic and R_upstream to the calculation below (they are == 0 or UNDEFINED for buffered archs I think)

        //NOTE: We neglect the T_quadratic and C_load terms and Switch R, so this lookahead is likely
        //      less accurate on unbuffered (e.g. pass-gate) architectures

        this->delay += device_ctx.rr_indexed_data[cost_index].T_linear;

        this->congestion_upstream += device_ctx.rr_indexed_data[cost_index].base_cost;
    }

    if (this->delay < 0) {
        VTR_LOG("NEGATIVE DELAY!\n");
    }

    /* set the cost of this node */
    this->cost = this->delay;
}

PQ_Entry_Delay::PQ_Entry_Delay(
    RRNodeId set_rr_node,
    int switch_ind,
    const PQ_Entry_Delay* parent) {
    this->rr_node = set_rr_node;

    if (parent != nullptr) {
        auto& device_ctx = g_vpr_ctx.device();
        const auto& rr_graph = device_ctx.rr_graph;
        float Tsw = rr_graph.rr_switch_inf(RRSwitchId(switch_ind)).Tdel;
        float Rsw = rr_graph.rr_switch_inf(RRSwitchId(switch_ind)).R;
        float Cnode = rr_graph.node_C(set_rr_node);
        float Rnode = rr_graph.node_R(set_rr_node);

        float T_linear = 0.f;
        if (rr_graph.rr_switch_inf(RRSwitchId(switch_ind)).buffered()) {
            T_linear = Tsw + Rsw * Cnode + 0.5 * Rnode * Cnode;
        } else { /* Pass transistor */
            T_linear = Tsw + 0.5 * Rsw * Cnode;
        }

        VTR_ASSERT(T_linear >= 0.);
        this->delay_cost = parent->delay_cost + T_linear;
    } else {
        this->delay_cost = 0.f;
    }
}

PQ_Entry_Base_Cost::PQ_Entry_Base_Cost(
    RRNodeId set_rr_node,
    int switch_ind,
    const PQ_Entry_Base_Cost* parent) {
    this->rr_node = set_rr_node;

    if (parent != nullptr) {
        auto& device_ctx = g_vpr_ctx.device();
        const auto& rr_graph = device_ctx.rr_graph;
        if (rr_graph.rr_switch_inf(RRSwitchId(switch_ind)).configurable()) {
            this->base_cost = parent->base_cost + get_single_rr_cong_base_cost(set_rr_node);
        } else {
            this->base_cost = parent->base_cost;
        }
    } else {
        this->base_cost = 0.f;
    }
}

/* returns cost entry with the smallest delay */
Cost_Entry Expansion_Cost_Entry::get_smallest_entry() const {
    Cost_Entry smallest_entry;

    for (auto entry : this->cost_vector) {
        if (!smallest_entry.valid() || entry.delay < smallest_entry.delay) {
            smallest_entry = entry;
        }
    }

    return smallest_entry;
}

/* returns a cost entry that represents the average of all the recorded entries */
Cost_Entry Expansion_Cost_Entry::get_average_entry() const {
    float avg_delay = 0;
    float avg_congestion = 0;

    for (auto cost_entry : this->cost_vector) {
        avg_delay += cost_entry.delay;
        avg_congestion += cost_entry.congestion;
    }

    avg_delay /= (float)this->cost_vector.size();
    avg_congestion /= (float)this->cost_vector.size();

    return Cost_Entry(avg_delay, avg_congestion);
}

/* returns a cost entry that represents the geomean of all the recorded entries */
Cost_Entry Expansion_Cost_Entry::get_geomean_entry() const {
    float geomean_delay = 0;
    float geomean_cong = 0;
    for (auto cost_entry : this->cost_vector) {
        geomean_delay += log(cost_entry.delay);
        geomean_cong += log(cost_entry.congestion);
    }

    geomean_delay = exp(geomean_delay / (float)this->cost_vector.size());
    geomean_cong = exp(geomean_cong / (float)this->cost_vector.size());

    return Cost_Entry(geomean_delay, geomean_cong);
}

/* returns a cost entry that represents the medial of all recorded entries */
Cost_Entry Expansion_Cost_Entry::get_median_entry() const {
    /* find median by binning the delays of all entries and then chosing the bin
     * with the largest number of entries */

    // This is code that needs to be revisited. For the time being, if the median entry
    // method calculation is used an assertion is thrown.
    VTR_ASSERT_MSG(false, "Get median entry calculation method is not implemented!");

    int num_bins = 10;

    /* find entries with smallest and largest delays */
    Cost_Entry min_del_entry;
    Cost_Entry max_del_entry;
    for (auto entry : this->cost_vector) {
        if (!min_del_entry.valid() || entry.delay < min_del_entry.delay) {
            min_del_entry = entry;
        }
        if (!max_del_entry.valid() || entry.delay > max_del_entry.delay) {
            max_del_entry = entry;
        }
    }

    /* get the bin size */
    float delay_diff = max_del_entry.delay - min_del_entry.delay;
    float bin_size = delay_diff / (float)num_bins;

    /* sort the cost entries into bins */
    std::vector<std::vector<Cost_Entry>> entry_bins(num_bins, std::vector<Cost_Entry>());
    for (auto entry : this->cost_vector) {
        float bin_num = floor((entry.delay - min_del_entry.delay) / bin_size);

        VTR_ASSERT(vtr::nint(bin_num) >= 0 && vtr::nint(bin_num) <= num_bins);
        if (vtr::nint(bin_num) == num_bins) {
            /* largest entry will otherwise have an out-of-bounds bin number */
            bin_num -= 1;
        }
        entry_bins[vtr::nint(bin_num)].push_back(entry);
    }

    /* find the bin with the largest number of elements */
    int largest_bin = 0;
    int largest_size = 0;
    for (int ibin = 0; ibin < num_bins; ibin++) {
        if (entry_bins[ibin].size() > (unsigned)largest_size) {
            largest_bin = ibin;
            largest_size = (unsigned)entry_bins[ibin].size();
        }
    }

    /* get the representative delay of the largest bin */
    Cost_Entry representative_entry = entry_bins[largest_bin][0];

    return representative_entry;
}

template<typename Entry>
void expand_dijkstra_neighbours(const RRGraphView& rr_graph,
                                const Entry& parent_entry,
                                std::vector<Search_Path>* paths,
                                std::vector<bool>* node_expanded,
                                std::priority_queue<Entry,
                                                    std::vector<Entry>,
                                                    std::greater<Entry>>* pq) {
    RRNodeId parent = parent_entry.rr_node;

    for (int iedge = 0; iedge < rr_graph.num_edges(parent); iedge++) {
        int child_node_ind = size_t(rr_graph.edge_sink_node(RRNodeId(parent), iedge));
        int switch_ind = rr_graph.edge_switch(parent, iedge);

        /* skip this child if it has already been expanded from */
        if ((*node_expanded)[child_node_ind]) {
            continue;
        }

        Entry child_entry(RRNodeId(child_node_ind), switch_ind, &parent_entry);
        VTR_ASSERT(child_entry.cost() >= 0);

        /* Create (if it doesn't exist) or update (if the new cost is lower)
         * to specified node */
        Search_Path path_entry = {child_entry.cost(), size_t(parent), iedge};
        auto& path = (*paths)[child_node_ind];
        if (path_entry.cost < path.cost) {
            pq->push(child_entry);
            path = path_entry;
        }
    }
}

template void expand_dijkstra_neighbours(const RRGraphView& rr_graph,
                                         const PQ_Entry_Delay& parent_entry,
                                         std::vector<Search_Path>* paths,
                                         std::vector<bool>* node_expanded,
                                         std::priority_queue<PQ_Entry_Delay,
                                                             std::vector<PQ_Entry_Delay>,
                                                             std::greater<PQ_Entry_Delay>>* pq);
template void expand_dijkstra_neighbours(const RRGraphView& rr_graph,
                                         const PQ_Entry_Base_Cost& parent_entry,
                                         std::vector<Search_Path>* paths,
                                         std::vector<bool>* node_expanded,
                                         std::priority_queue<PQ_Entry_Base_Cost,
                                                             std::vector<PQ_Entry_Base_Cost>,
                                                             std::greater<PQ_Entry_Base_Cost>>* pq);

t_src_opin_delays compute_router_src_opin_lookahead(bool is_flat) {
    vtr::ScopedStartFinishTimer timer("Computing src/opin lookahead");
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_graph;

    int num_layers = device_ctx.grid.get_num_layers();

    t_src_opin_delays src_opin_delays;
    src_opin_delays.resize(num_layers);
    std::vector<int> tile_max_ptc(device_ctx.physical_tile_types.size(), OPEN);

    // Get the maximum OPIN ptc for each tile type to reserve src_opin_delays
    for (int itile = 0; itile < (int)device_ctx.physical_tile_types.size(); itile++) {
        tile_max_ptc[itile] = get_tile_src_opin_max_ptc(itile);
    }

    // Resize src_opin_delays to accommodate enough ptc and layer
    for (int layer_num = 0; layer_num < num_layers; layer_num++) {
        src_opin_delays[layer_num].resize(device_ctx.physical_tile_types.size());
        for (int itile = 0; itile < (int)device_ctx.physical_tile_types.size(); itile++) {
            src_opin_delays[layer_num][itile].resize(tile_max_ptc[itile] + 1);
            for (int ptc_num = 0; ptc_num <= tile_max_ptc[itile]; ptc_num++) {
                src_opin_delays[layer_num][itile][ptc_num].resize(num_layers);
            }
        }
    }

    //We assume that the routing connectivity of each instance of a physical tile is the same,
    //and so only measure one instance of each type
    for (int from_layer_num = 0; from_layer_num < num_layers; from_layer_num++) {
        for (size_t itile = 0; itile < device_ctx.physical_tile_types.size(); ++itile) {
            if (device_ctx.grid.num_instances(&device_ctx.physical_tile_types[itile], from_layer_num) == 0) {
                continue;
            }
            for (e_rr_type rr_type : {SOURCE, OPIN}) {
                t_physical_tile_loc sample_loc(OPEN, OPEN, OPEN);

                size_t num_sampled_locs = 0;
                bool ptcs_with_no_delays = true;
                while (ptcs_with_no_delays) { //Haven't found wire connected to ptc
                    ptcs_with_no_delays = false;

                    sample_loc = pick_sample_tile(from_layer_num,
                                                  &device_ctx.physical_tile_types[itile],
                                                  sample_loc);

                    if (sample_loc.x == OPEN && sample_loc.y == OPEN && sample_loc.layer_num == OPEN) {
                        //No untried instances of the current tile type left
                        VTR_LOG_WARN("Found no %sample locations for %s in %s\n",
                                     (num_sampled_locs == 0) ? "" : "more ",
                                     rr_node_typename[rr_type],
                                     device_ctx.physical_tile_types[itile].name);
                        break;
                    }

                    //VTR_LOG("Sampling %s at (%d,%d)\n", device_ctx.physical_tile_types[itile].name, sample_loc.x(), sample_loc.y());
                    const std::vector<RRNodeId>& rr_nodes_at_loc = device_ctx.rr_graph.node_lookup().find_grid_nodes_at_all_sides(sample_loc.layer_num, sample_loc.x, sample_loc.y, rr_type);
                    for (RRNodeId node_id : rr_nodes_at_loc) {
                        int ptc = rr_graph.node_ptc_num(node_id);
                        // For the time being, we decide to not let the lookahead explore the node inside the clusters
                        if (!is_inter_cluster_node(rr_graph, node_id)) {
                            continue;
                        }

                        VTR_ASSERT(ptc < int(src_opin_delays[from_layer_num][itile].size()));

                        //Find the wire types which are reachable from inode and record them and
                        //the cost to reach them
                        dijkstra_flood_to_wires(itile,
                                                node_id,
                                                src_opin_delays);

                        bool reachable_wire_found = false;
                        for (int to_layer_num = 0; to_layer_num < num_layers; to_layer_num++) {
                            if (!src_opin_delays[from_layer_num][itile][ptc][to_layer_num].empty()) {
                                reachable_wire_found = true;
                                break;
                            }
                        }
                        if (reachable_wire_found) {
                            VTR_LOGV_DEBUG(f_router_debug, "Found no reachable wires from %s (%s) at (%d,%d,%d)\n",
                                           rr_node_typename[rr_type],
                                           rr_node_arch_name(node_id, is_flat).c_str(),
                                           sample_loc.x,
                                           sample_loc.y,
                                           sample_loc.layer_num,
                                           is_flat);

                            ptcs_with_no_delays = true;
                        }
                    }

                    ++num_sampled_locs;
                }
                if (ptcs_with_no_delays) {
                    VPR_ERROR(VPR_ERROR_ROUTE, "Some SOURCE/OPINs have no reachable wires\n");
                }
            }
        }
    }

    return src_opin_delays;
}

t_chan_ipins_delays compute_router_chan_ipin_lookahead() {
    vtr::ScopedStartFinishTimer timer("Computing chan/ipin lookahead");
    auto& device_ctx = g_vpr_ctx.device();
    const auto& node_lookup = device_ctx.rr_graph.node_lookup();

    t_chan_ipins_delays chan_ipins_delays;

    chan_ipins_delays.resize(device_ctx.grid.get_num_layers());
    for (int layer_num = 0; layer_num < device_ctx.grid.get_num_layers(); layer_num++) {
        chan_ipins_delays[layer_num].resize(device_ctx.physical_tile_types.size());
    }

    //We assume that the routing connectivity of each instance of a physical tile is the same,
    //and so only measure one instance of each type
    for (int layer_num = 0; layer_num < device_ctx.grid.get_num_layers(); layer_num++) {
        for (auto tile_type : device_ctx.physical_tile_types) {
            if (device_ctx.grid.num_instances(&tile_type, layer_num) == 0) {
                continue;
            }
            t_physical_tile_loc sample_loc(OPEN, OPEN, OPEN);

            sample_loc = pick_sample_tile(layer_num, &tile_type, sample_loc);

            if (sample_loc.x == OPEN && sample_loc.y == OPEN && sample_loc.layer_num == OPEN) {
                //No untried instances of the current tile type left
                VTR_LOG_WARN("Found no sample locations for %s\n",
                             tile_type.name);
                continue;
            }

            int min_x = std::max(0, sample_loc.x - X_OFFSET);
            int min_y = std::max(0, sample_loc.y - Y_OFFSET);
            int max_x = std::min(int(device_ctx.grid.width()), sample_loc.x + X_OFFSET);
            int max_y = std::min(int(device_ctx.grid.height()), sample_loc.y + Y_OFFSET);

            for (int ix = min_x; ix < max_x; ix++) {
                for (int iy = min_y; iy < max_y; iy++) {
                    for (auto rr_type : {CHANX, CHANY}) {
                        for (const RRNodeId& node_id : node_lookup.find_channel_nodes(sample_loc.layer_num, ix, iy, rr_type)) {
                            //Find the IPINs which are reachable from the wires within the bounding box
                            //around the selected tile location
                            dijkstra_flood_to_ipins(node_id, chan_ipins_delays);
                        }
                    }
                }
            }
        }
    }

    return chan_ipins_delays;
}

t_ipin_primitive_sink_delays compute_intra_tile_dijkstra(const RRGraphView& rr_graph,
                                                         t_physical_tile_type_ptr physical_tile,
                                                         int layer,
                                                         int x,
                                                         int y) {
    auto tile_pins_vec = get_flat_tile_pins(physical_tile);
    int max_ptc_num = get_tile_pin_max_ptc(physical_tile, true);

    t_ipin_primitive_sink_delays pin_delays;
    pin_delays.resize(max_ptc_num);

    for (int pin_physical_num : tile_pins_vec) {
        RRNodeId pin_node_id = get_pin_rr_node_id(rr_graph.node_lookup(),
                                                  physical_tile,
                                                  layer,
                                                  x,
                                                  y,
                                                  pin_physical_num);
        VTR_ASSERT(pin_node_id != RRNodeId::INVALID());

        run_intra_tile_dijkstra(rr_graph, pin_delays, physical_tile, pin_node_id);
    }

    return pin_delays;
}

/* returns index of a node from which to start routing */
RRNodeId get_start_node(int layer, int start_x, int start_y, int target_x, int target_y, t_rr_type rr_type, int seg_index, int track_offset) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    const auto& node_lookup = rr_graph.node_lookup();

    RRNodeId result = RRNodeId::INVALID();

    if (rr_type != CHANX && rr_type != CHANY) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Must start lookahead routing from CHANX or CHANY node\n");
    }

    /* determine which direction the wire should go in based on the start & target coordinates */
    Direction direction = Direction::INC;
    if ((rr_type == CHANX && target_x < start_x) || (rr_type == CHANY && target_y < start_y)) {
        direction = Direction::DEC;
    }

    int start_lookup_x = start_x;
    int start_lookup_y = start_y;

    /* find first node in channel that has specified segment index and goes in the desired direction */
    for (const RRNodeId& node_id : node_lookup.find_channel_nodes(layer, start_lookup_x, start_lookup_y, rr_type)) {
        VTR_ASSERT(rr_graph.node_type(node_id) == rr_type);

        Direction node_direction = rr_graph.node_direction(node_id);
        auto node_cost_ind = rr_graph.node_cost_index(node_id);
        int node_seg_ind = device_ctx.rr_indexed_data[node_cost_ind].seg_index;

        if ((node_direction == direction || node_direction == Direction::BIDIR) && node_seg_ind == seg_index) {
            /* found first track that has the specified segment index and goes in the desired direction */
            result = node_id;
            if (track_offset == 0) {
                break;
            }
            track_offset -= 2;
        }
    }

    return result;
}

std::pair<int, int> get_xy_deltas(RRNodeId from_node, RRNodeId to_node) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    e_rr_type from_type = rr_graph.node_type(from_node);
    e_rr_type to_type = rr_graph.node_type(to_node);

    int delta_x, delta_y;

    if (!is_chan(from_type) && !is_chan(to_type)) {
        //Alternate formulation for non-channel types
        auto [from_x, from_y] = get_adjusted_rr_position(from_node);
        auto [to_x, to_y] = get_adjusted_rr_position(to_node);

        delta_x = to_x - from_x;
        delta_y = to_y - from_y;
    } else {
        //Traditional formulation

        /* get chan/seg coordinates of the from/to nodes. seg coordinate is along the wire,
         * chan coordinate is orthogonal to the wire */
        int from_seg_low;
        int from_seg_high;
        int from_chan;
        int to_seg;
        int to_chan;
        if (from_type == CHANY) {
            from_seg_low = rr_graph.node_ylow(from_node);
            from_seg_high = rr_graph.node_yhigh(from_node);
            from_chan = rr_graph.node_xlow(from_node);
            to_seg = rr_graph.node_ylow(to_node);
            to_chan = rr_graph.node_xlow(to_node);
        } else {
            from_seg_low = rr_graph.node_xlow(from_node);
            from_seg_high = rr_graph.node_xhigh(from_node);
            from_chan = rr_graph.node_ylow(from_node);
            to_seg = rr_graph.node_xlow(to_node);
            to_chan = rr_graph.node_ylow(to_node);
        }

        /* now we want to count the minimum number of *channel segments* between the from and to nodes */
        int delta_seg, delta_chan;

        /* orthogonal to wire */
        int no_need_to_pass_by_clb = 0; //if we need orthogonal wires then we don't need to pass by the target CLB along the current wire direction
        if (to_chan > from_chan + 1) {
            /* above */
            delta_chan = to_chan - from_chan;
            no_need_to_pass_by_clb = 1;
        } else if (to_chan < from_chan) {
            /* below */
            delta_chan = from_chan - to_chan + 1;
            no_need_to_pass_by_clb = 1;
        } else {
            /* adjacent to current channel */
            delta_chan = 0;
            no_need_to_pass_by_clb = 0;
        }

        /* along same direction as wire. */
        if (to_seg > from_seg_high) {
            /* ahead */
            delta_seg = to_seg - from_seg_high - no_need_to_pass_by_clb;
        } else if (to_seg < from_seg_low) {
            /* behind */
            delta_seg = from_seg_low - to_seg - no_need_to_pass_by_clb;
        } else {
            /* along the span of the wire */
            delta_seg = 0;
        }

        /* account for wire direction. lookahead map was computed by looking up and to the right starting at INC wires. for targets
         * that are opposite of the wire direction, let's add 1 to delta_seg */
        Direction from_dir = rr_graph.node_direction(from_node);
        if (is_chan(from_type)
            && ((to_seg < from_seg_low && from_dir == Direction::INC) || (to_seg > from_seg_high && from_dir == Direction::DEC))) {
            delta_seg++;
        }

        if (from_type == CHANY) {
            delta_x = delta_chan;
            delta_y = delta_seg;
        } else {
            delta_x = delta_seg;
            delta_y = delta_chan;
        }
    }

    VTR_ASSERT_SAFE(std::abs(delta_x) < (int)device_ctx.grid.width());
    VTR_ASSERT_SAFE(std::abs(delta_y) < (int)device_ctx.grid.height());

    return {delta_x, delta_y};
}

t_routing_cost_map get_routing_cost_map(int longest_seg_length,
                                        int from_layer_num,
                                        const e_rr_type& chan_type,
                                        const t_segment_inf& segment_inf,
                                        const std::unordered_map<int, std::unordered_set<int>>& sample_locs,
                                        bool sample_all_locs) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    const auto& grid = device_ctx.grid;

    //Start sampling at the lower left non-corner
    int ref_x = 1;
    int ref_y = 1;

    //Sample from locations near the reference location (to capture maximum distance paths)
    //Also sample from locations at least the longest wire length away from the edge (to avoid
    //edge effects for shorter distances)
    std::vector<int> ref_increments = {0,
                                       1,
                                       longest_seg_length,
                                       longest_seg_length + 1};

    //Uniquify the increments (avoid sampling the same locations repeatedly if they happen to
    //overlap)
    std::stable_sort(ref_increments.begin(), ref_increments.end());
    ref_increments.erase(std::unique(ref_increments.begin(), ref_increments.end()), ref_increments.end());

    //Upper right non-corner
    int target_x = device_ctx.grid.width() - 2;
    int target_y = device_ctx.grid.height() - 2;

    //if arch file specifies die_number="layer_num" doesn't require inter-cluster
    //programmable routing resources, then we shouldn't profile wire segment types in
    //the current layer
    if (!device_ctx.inter_cluster_prog_routing_resources[from_layer_num]) {
        return t_routing_cost_map();
    }

    //First try to pick good representative sample locations for each type
    std::vector<RRNodeId> sample_nodes;
    std::vector<e_rr_type> chan_types;
    if (segment_inf.parallel_axis == X_AXIS)
        chan_types.push_back(CHANX);
    else if (segment_inf.parallel_axis == Y_AXIS)
        chan_types.push_back(CHANY);
    else //Both for BOTH_AXIS segments and special segments such as clock_networks we want to search in both directions.
        chan_types.insert(chan_types.end(), {CHANX, CHANY});

    for (int ref_inc : ref_increments) {
        int sample_x = ref_x + ref_inc;
        int sample_y = ref_y + ref_inc;

        if (sample_x >= int(grid.width())) continue;
        if (sample_y >= int(grid.height())) continue;

        for (int track_offset = 0; track_offset < MAX_TRACK_OFFSET; track_offset += 2) {
            /* get the rr node index from which to start routing */
            RRNodeId start_node = get_start_node(from_layer_num, sample_x, sample_y,
                                                 target_x, target_y, //non-corner upper right
                                                 chan_type, segment_inf.seg_index, track_offset);

            if (!start_node) {
                continue;
            }
            // TODO: Temporary - After testing benchmarks this can be deleted
            VTR_ASSERT(rr_graph.node_layer(start_node) == from_layer_num);

            sample_nodes.emplace_back(start_node);
        }
    }

    //If we failed to find any representative sample locations, search exhaustively
    //
    //This is to ensure we sample 'unusual' wire types which may not exist in all channels
    //(e.g. clock routing)
    if (sample_nodes.empty()) {
        //Try an exhaustive search to find a suitable sample point
        for (RRNodeId rr_node : rr_graph.nodes()) {
            auto rr_type = rr_graph.node_type(rr_node);
            if (rr_type != chan_type) continue;
            if (rr_graph.node_layer(rr_node) != from_layer_num) continue;

            auto cost_index = rr_graph.node_cost_index(rr_node);
            VTR_ASSERT(cost_index != RRIndexedDataId(OPEN));

            int seg_index = device_ctx.rr_indexed_data[cost_index].seg_index;

            if (seg_index == segment_inf.seg_index) {
                sample_nodes.push_back(rr_node);
            }

            if (sample_nodes.size() >= ref_increments.size()) {
                break;
            }
        }
    }

    //Finally, now that we have a list of sample locations, run a Dijkstra flood from
    //each sample location to profile the routing network from this type


    t_routing_cost_map routing_cost_map({static_cast<unsigned long>(device_ctx.grid.get_num_layers()), device_ctx.grid.width(), device_ctx.grid.height()});

    if (sample_nodes.empty()) {
        VTR_LOG_WARN("Unable to find any sample location for segment %s type '%s' (length %d)\n",
                     rr_node_typename[chan_type],
                     segment_inf.name.c_str(),
                     segment_inf.length);
    } else {
        //reset cost for this segment
        routing_cost_map.fill(Expansion_Cost_Entry());

        // to avoid multiple memory allocation and de-allocations in run_dijkstra()
        // dijkstra_data is created outside the for loop and passed by reference to dijkstra_data()
        t_dijkstra_data dijkstra_data;

        for (RRNodeId sample_node : sample_nodes) {
            int sample_x = rr_graph.node_xlow(sample_node);
            int sample_y = rr_graph.node_ylow(sample_node);

            if (rr_graph.node_direction(sample_node) == Direction::DEC) {
                sample_x = rr_graph.node_xhigh(sample_node);
                sample_y = rr_graph.node_yhigh(sample_node);
            }

            run_dijkstra(sample_node,
                         sample_x,
                         sample_y,
                         routing_cost_map,
                         dijkstra_data,
                         sample_locs,
                         sample_all_locs);
        }
    }

    return routing_cost_map;
}

std::pair<float, float> get_cost_from_src_opin(const std::map<int, util::t_reachable_wire_inf>& src_opin_delay_map,
                                               int delta_x,
                                               int delta_y,
                                               int to_layer_num,
                                               WireCostFunc wire_cost_func) {
    float expected_delay_cost = std::numeric_limits<float>::infinity();
    float expected_cong_cost = std::numeric_limits<float>::infinity();
    if (src_opin_delay_map.empty()) {
        //During lookahead profiling we were unable to find any wires which connected
        //to this PTC.
        //
        //This can sometimes occur at very low channel widths (e.g. during min W search on
        //small designs) where W discretization combined with fraction Fc may cause some
        //pins/sources to be left disconnected.
        //
        //Such RR graphs are of course unroutable, but that should be determined by the
        //router. So just return an arbitrary value here rather than error.

        //We choose to return the largest (non-infinite) value possible, but scaled
        //down by a large factor to maintain some dynamic range in case this value ends
        //up being processed (e.g. by the timing analyzer).
        //
        //The cost estimate should still be *extremely* large compared to a typical delay, and
        //so should ensure that the router de-prioritizes exploring this path, but does not
        //forbid the router from trying.
        expected_delay_cost = std::numeric_limits<float>::max() / 1e12;
        expected_cong_cost = std::numeric_limits<float>::max() / 1e12;
    } else {
        //From the current SOURCE/OPIN we look-up the wiretypes which are reachable
        //and then add the estimates from those wire types for the distance of interest.
        //If there are multiple options we use the minimum value.
        for (const auto& [_, reachable_wire_inf] : src_opin_delay_map) {

            util::Cost_Entry wire_cost_entry;
            if (reachable_wire_inf.wire_rr_type == SINK) {
                //Some pins maybe reachable via a direct (OPIN -> IPIN) connection.
                //In the lookahead, we treat such connections as 'special' wire types
                //with no delay/congestion cost
                wire_cost_entry.delay = 0;
                wire_cost_entry.congestion = 0;
            } else {
                //For an actual accessible wire, we query the wire look-up to get its
                //delay and congestion cost estimates
                wire_cost_entry = wire_cost_func(reachable_wire_inf.wire_rr_type,
                                                 reachable_wire_inf.wire_seg_index,
                                                 reachable_wire_inf.layer_number,
                                                 delta_x,
                                                 delta_y,
                                                 to_layer_num);
            }

            float this_delay_cost = reachable_wire_inf.delay + wire_cost_entry.delay;
            float this_cong_cost = reachable_wire_inf.congestion + wire_cost_entry.congestion;

            expected_delay_cost = std::min(expected_delay_cost, this_delay_cost);
            expected_cong_cost = std::min(expected_cong_cost, this_cong_cost);
        }
    }

    return std::make_pair(expected_delay_cost, expected_cong_cost);
}

void dump_readable_router_lookahead_map(const std::string& file_name, const std::vector<int>& dim_sizes, WireCostCallBackFunction wire_cost_func) {
    VTR_ASSERT(vtr::check_file_name_extension(file_name, ".csv"));
    const auto& grid = g_vpr_ctx.device().grid;

    int num_layers = grid.get_num_layers();
    int grid_width = static_cast<int>(grid.width());
    int grid_height = static_cast<int>(grid.height());

    std::ofstream ofs(file_name);
    if (!ofs.good()) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Unable to open file '%s' for writing\n", file_name.c_str());
    }

    VTR_ASSERT(dim_sizes[0] == num_layers);
    VTR_ASSERT(dim_sizes[1] == 2);
    VTR_ASSERT(dim_sizes[3] == num_layers);
    VTR_ASSERT(dim_sizes.size() == 5 || (dim_sizes.size() == 6 && dim_sizes[4] == grid_width && dim_sizes[5] == grid_height));

    ofs << "from_layer,"
           "chan_type,"
           "seg_type,"
           "to_layer,"
           "delta_x,"
           "delta_y,"
           "cong_cost,"
           "delay_cost\n";

    for (int from_layer_num = 0; from_layer_num < num_layers; from_layer_num++) {
        for (e_rr_type chan_type : {CHANX, CHANY}) {
            for (int seg_index = 0; seg_index < dim_sizes[2]; seg_index++) {
                for (int to_layer_num = 0; to_layer_num < num_layers; to_layer_num++) {
                    for (int dx = 0; dx < grid_width; dx++) {
                        for (int dy = 0; dy < grid_height; dy++) {
                            auto cost = wire_cost_func(chan_type, seg_index, from_layer_num, dx, dy, to_layer_num);
                            ofs << from_layer_num << ","
                                << rr_node_typename[chan_type] << ","
                                << seg_index << ","
                                << to_layer_num << ","
                                << dx << ","
                                << dy << ","
                                << cost.congestion << ","
                                << cost.delay << "\n";
                        }
                    }
                }
            }
        }
    }
}

} // namespace util

static void dijkstra_flood_to_wires(int itile,
                                    RRNodeId node,
                                    util::t_src_opin_delays& src_opin_delays) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    struct t_pq_entry {
        float delay;
        float congestion;
        RRNodeId node;

        bool operator<(const t_pq_entry& rhs) const {
            return this->delay < rhs.delay;
        }
    };

    std::priority_queue<t_pq_entry> pq;

    t_pq_entry root;
    root.congestion = 0.;
    root.delay = 0.;
    root.node = node;

    int ptc = rr_graph.node_ptc_num(node);
    int root_layer_num = rr_graph.node_layer(node);

    /*
     * Perform Dijkstra from the SOURCE/OPIN of interest, stopping at the first
     * reachable wires (i.e. until we hit the inter-block routing network), or a SINK
     * (via a direct-connect).
     *
     * Note that typical RR graphs are structured :
     *
     *      SOURCE ---> OPIN --> CHANX/CHANY
     *              |
     *              --> OPIN --> CHANX/CHANY
     *              |
     *             ...
     *
     *   possibly with direct-connects of the form:
     *
     *      SOURCE --> OPIN --> IPIN --> SINK
     *
     * and there is a small number of fixed hops from SOURCE/OPIN to CHANX/CHANY or
     * to a SINK (via a direct-connect), so this runs very fast (i.e. O(1))
     */
    pq.push(root);
    while (!pq.empty()) {
        t_pq_entry curr = pq.top();
        pq.pop();

        e_rr_type curr_rr_type = rr_graph.node_type(curr.node);
        int curr_layer_num = rr_graph.node_layer(curr.node);
        if (curr_rr_type == CHANX || curr_rr_type == CHANY || curr_rr_type == SINK) {
            //We stop expansion at any CHANX/CHANY/SINK
            int seg_index;
            if (curr_rr_type != SINK) {
                //It's a wire, figure out its type
                auto cost_index = rr_graph.node_cost_index(curr.node);
                seg_index = device_ctx.rr_indexed_data[cost_index].seg_index;
            } else {
                //This is a direct-connect path between an IPIN and OPIN,
                //which terminated at a SINK.
                //
                //We treat this as a 'special' wire type.
                //When the src_opin_delays data structure is queried and a SINK rr_type
                //is found, the lookahead is not accessed to get the cost entries
                //as this is a special case of direct connections between OPIN and IPIN.
                seg_index = DIRECT_CONNECT_SPECIAL_SEG_TYPE;
            }

            //Keep costs of the best path to reach each wire type
            if (!src_opin_delays[root_layer_num][itile][ptc][curr_layer_num].count(seg_index)
                || curr.delay < src_opin_delays[root_layer_num][itile][ptc][curr_layer_num][seg_index].delay) {
                src_opin_delays[root_layer_num][itile][ptc][curr_layer_num][seg_index].wire_rr_type = curr_rr_type;
                src_opin_delays[root_layer_num][itile][ptc][curr_layer_num][seg_index].wire_seg_index = seg_index;
                src_opin_delays[root_layer_num][itile][ptc][curr_layer_num][seg_index].layer_number = curr_layer_num;
                src_opin_delays[root_layer_num][itile][ptc][curr_layer_num][seg_index].delay = curr.delay;
                src_opin_delays[root_layer_num][itile][ptc][curr_layer_num][seg_index].congestion = curr.congestion;
            }

        } else if (curr_rr_type == SOURCE || curr_rr_type == OPIN || curr_rr_type == IPIN) {
            //We allow expansion through SOURCE/OPIN/IPIN types
            auto cost_index = rr_graph.node_cost_index(curr.node);
            float incr_cong = device_ctx.rr_indexed_data[cost_index].base_cost; //Current nodes congestion cost

            for (RREdgeId edge : rr_graph.edge_range(curr.node)) {
                int iswitch = rr_graph.rr_nodes().edge_switch(edge);
                float incr_delay = rr_graph.rr_switch_inf(RRSwitchId(iswitch)).Tdel;

                RRNodeId next_node = rr_graph.rr_nodes().edge_sink_node(edge);
                // For the time being, we decide to not let the lookahead explore the node inside the clusters

                if (!is_inter_cluster_node(rr_graph, next_node)) {
                    // Don't go inside the clusters
                    continue;
                }

                t_pq_entry next;
                next.congestion = curr.congestion + incr_cong; //Of current node
                next.delay = curr.delay + incr_delay;          //To reach next node
                next.node = next_node;

                pq.push(next);
            }
        } else {
            VPR_ERROR(VPR_ERROR_ROUTE, "Unrecognized RR type");
        }
    }
}

static void dijkstra_flood_to_ipins(RRNodeId node, util::t_chan_ipins_delays& chan_ipins_delays) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_graph;

    struct t_pq_entry {
        float delay;
        float congestion;
        RRNodeId node;
        int level;

        bool operator<(const t_pq_entry& rhs) const {
            return this->delay < rhs.delay;
        }
    };

    std::priority_queue<t_pq_entry> pq;

    t_pq_entry root;
    root.congestion = 0.;
    root.delay = 0.;
    root.node = node;
    root.level = 0;

    int root_layer = rr_graph.node_layer(node);

    /*
     * Perform Djikstra from the CHAN of interest, stopping at the the first
     * reachable IPIN
     *
     * Note that typical RR graphs are structured :
     *
     *      CHANX/CHANY --> CHANX/CHANY --> ... --> CHANX/CHANY --> IPIN --> SINK
     *                  |
     *                  --> CHANX/CHANY --> ... --> CHANX/CHANY --> IPIN --> SINK
     *                  |
     *                  ...
     *
     * and there is a variable number of hops from a given CHANX/CHANY  to IPIN.
     * To avoid impacting on run-time, a fixed number of hops is performed. This
     * should be enough to find the delay from the last CAHN to IPIN connection.
     */
    pq.push(root);

    float site_pin_delay = std::numeric_limits<float>::infinity();

    while (!pq.empty()) {
        t_pq_entry curr = pq.top();
        pq.pop();

        e_rr_type curr_rr_type = rr_graph.node_type(curr.node);
        if (curr_rr_type == IPIN) {
            int node_x = rr_graph.node_xlow(curr.node);
            int node_y = rr_graph.node_ylow(curr.node);
            int node_layer = rr_graph.node_layer(curr.node);

            auto tile_type = device_ctx.grid.get_physical_type({node_x, node_y, node_layer});
            int itile = tile_type->index;

            int ptc = rr_graph.node_ptc_num(curr.node);

            if (ptc >= int(chan_ipins_delays[root_layer][itile].size())) {
                chan_ipins_delays[root_layer][itile].resize(ptc + 1); //Inefficient but functional...
            }

            site_pin_delay = std::min(curr.delay, site_pin_delay);
            //Keep costs of the best path to reach each wire type
            chan_ipins_delays[root_layer][itile][ptc].wire_rr_type = curr_rr_type;
            chan_ipins_delays[root_layer][itile][ptc].delay = site_pin_delay;
            chan_ipins_delays[root_layer][itile][ptc].congestion = curr.congestion;
        } else if (curr_rr_type == CHANX || curr_rr_type == CHANY) {
            if (curr.level >= MAX_EXPANSION_LEVEL) {
                continue;
            }

            //We allow expansion through SOURCE/OPIN/IPIN types
            auto cost_index = rr_graph.node_cost_index(curr.node);
            float new_cong = device_ctx.rr_indexed_data[cost_index].base_cost; //Current nodes congestion cost

            for (RREdgeId edge : rr_graph.edge_range(curr.node)) {
                int iswitch = rr_graph.rr_nodes().edge_switch(edge);
                float new_delay = rr_graph.rr_switch_inf(RRSwitchId(iswitch)).Tdel;

                RRNodeId next_node = rr_graph.rr_nodes().edge_sink_node(edge);

                if (rr_graph.node_layer(next_node) != root_layer) {
                    //Don't change the layer
                    continue;
                }

                t_pq_entry next;
                next.congestion = new_cong; //Of current node
                next.delay = new_delay;     //To reach next node
                next.node = next_node;
                next.level = curr.level + 1;

                pq.push(next);
            }
        } else {
            VPR_ERROR(VPR_ERROR_ROUTE, "Unrecognized RR type");
        }
    }
}

static int get_tile_src_opin_max_ptc(int itile) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& physical_tile = device_ctx.physical_tile_types[itile];
    int max_ptc = 0;

    // Output pin
    for (const auto& class_inf : physical_tile.class_inf) {
        if (class_inf.type != e_pin_type::DRIVER) {
            continue;
        }
        for (const auto& pin_ptc : class_inf.pinlist) {
            if (pin_ptc > max_ptc) {
                max_ptc = pin_ptc;
            }
        }
    }

    return max_ptc;
}

static t_physical_tile_loc pick_sample_tile(int layer_num, t_physical_tile_type_ptr tile_type, t_physical_tile_loc prev) {
    //Very simple for now, just pick the fist matching tile found
    t_physical_tile_loc loc(OPEN, OPEN, OPEN);

    //VTR_LOG("Prev: %d,%d\n", prev.x, prev.y);

    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

    int y_init = prev.y + 1; //Start searching next element above prev

    if (device_ctx.grid.num_instances(tile_type, layer_num) == 0) {
        return loc;
    }
    for (int x = prev.x; x < int(grid.width()); ++x) {
        if (x < 0) continue;

        //VTR_LOG("  x: %d\n", x);

        for (int y = y_init; y < int(grid.height()); ++y) {
            if (y < 0) continue;

            //VTR_LOG("   y: %d\n", y);
            if (grid.get_physical_type(t_physical_tile_loc(x, y, layer_num)) == tile_type) {
                loc.x = x;
                loc.y = y;
                loc.layer_num = layer_num;
                break;
            }
        }

        if (loc.x != OPEN && loc.y != OPEN && loc.layer_num != OPEN) {
            break;
        } else {
            y_init = 0; //Prepare to search next column
        }
    }

    //VTR_LOG("Next: %d,%d\n", loc.x, loc.y);

    return loc;
}

static void run_intra_tile_dijkstra(const RRGraphView& rr_graph,
                                    util::t_ipin_primitive_sink_delays& pin_delays,
                                    t_physical_tile_type_ptr /*physical_tile*/,
                                    RRNodeId starting_node_id) {
    // device_ctx should not be used to access rr_graph, since the graph get from device_ctx is not the intra-tile graph
    const auto& device_ctx = g_vpr_ctx.device();

    vtr::vector<RRNodeId, bool> node_expanded;
    node_expanded.resize(rr_graph.num_nodes());
    std::fill(node_expanded.begin(), node_expanded.end(), false);

    vtr::vector<RRNodeId, float> node_seen_cost;
    node_seen_cost.resize(rr_graph.num_nodes());
    std::fill(node_seen_cost.begin(), node_seen_cost.end(), -1.);

    struct t_pq_entry {
        float delay;
        float congestion;
        RRNodeId node;

        bool operator<(const t_pq_entry& rhs) const {
            return this->delay < rhs.delay;
        }

        bool operator>(const t_pq_entry& rhs) const {
            return this->delay > rhs.delay;
        }
    };

    std::priority_queue<t_pq_entry, std::vector<t_pq_entry>, std::greater<t_pq_entry>> pq;

    t_pq_entry root;
    root.congestion = 0.;
    root.delay = 0.;
    root.node = starting_node_id;

    int root_ptc = rr_graph.node_ptc_num(root.node);
    std::unordered_map<int, util::Cost_Entry>& starting_pin_delay_map = pin_delays.at(root_ptc);
    pq.push(root);

    while (!pq.empty()) {
        t_pq_entry curr = pq.top();
        pq.pop();
        if (node_expanded[curr.node]) {
            continue;
        } else {
            node_expanded[curr.node] = true;
        }
        auto curr_type = rr_graph.node_type(curr.node);
        VTR_ASSERT(curr_type != t_rr_type::CHANX && curr_type != t_rr_type::CHANY);
        if (curr_type != SINK) {
            for (RREdgeId edge : rr_graph.edge_range(curr.node)) {
                RRNodeId next_node = rr_graph.rr_nodes().edge_sink_node(edge);
                auto cost_index = rr_graph.node_cost_index(next_node);
                int iswitch = rr_graph.rr_nodes().edge_switch(edge);

                float incr_delay = rr_graph.rr_switch_inf(RRSwitchId(iswitch)).Tdel;
                float incr_cong = device_ctx.rr_indexed_data[cost_index].base_cost;

                t_pq_entry next;
                next.congestion = curr.congestion + incr_cong; //Of current node
                next.delay = curr.delay + incr_delay;          //To reach next node
                next.node = next_node;

                if (node_seen_cost[next_node] < 0. || node_seen_cost[next_node] > next.delay) {
                    node_seen_cost[next_node] = next.delay;
                    pq.push(next);
                }
            }
        } else {
            int curr_ptc = rr_graph.node_ptc_num(curr.node);
            if (starting_pin_delay_map.find(curr_ptc) == starting_pin_delay_map.end() || starting_pin_delay_map.at(curr_ptc).delay > curr.delay) {
                starting_pin_delay_map[curr_ptc] = util::Cost_Entry(curr.delay, curr.congestion);
            }
        }
    }
}

/* runs Dijkstra's algorithm from specified node until all nodes have been visited. Each time a pin is visited, the delay/congestion information
 * to that pin is stored is added to an entry in the routing_cost_map */
static void run_dijkstra(RRNodeId start_node,
                         int start_x,
                         int start_y,
                         util::t_routing_cost_map& routing_cost_map,
                         util::t_dijkstra_data& data,
                         const std::unordered_map<int, std::unordered_set<int>>& sample_locs,
                         bool sample_all_locs) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    auto& node_expanded = data.node_expanded;
    node_expanded.resize(rr_graph.num_nodes());
    std::fill(node_expanded.begin(), node_expanded.end(), false);

    auto& node_visited_costs = data.node_visited_costs;
    node_visited_costs.resize(rr_graph.num_nodes());
    std::fill(node_visited_costs.begin(), node_visited_costs.end(), -1.0);

    /* a priority queue for expansion */
    std::priority_queue<util::PQ_Entry>& pq = data.pq;

    //Clear priority queue if non-empty
    while (!pq.empty()) {
        pq.pop();
    }

    /* first entry has no upstream delay or congestion */
    pq.emplace(start_node, UNDEFINED, 0, 0, 0, true);

    /* now do routing */
    while (!pq.empty()) {
        util::PQ_Entry current = pq.top();
        pq.pop();

        RRNodeId curr_node = current.rr_node;

        /* check that we haven't already expanded from this node */
        if (node_expanded[curr_node]) {
            continue;
        }

        //VTR_LOG("Expanding with delay=%10.3g cong=%10.3g (%s)\n", current.delay, current.congestion_upstream, describe_rr_node(rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, curr_node).c_str());

        /* if this node is an ipin record its congestion/delay in the routing_cost_map */
        if (rr_graph.node_type(curr_node) == IPIN) {
            VTR_ASSERT_SAFE(rr_graph.node_xlow(curr_node) == rr_graph.node_xhigh(curr_node));
            VTR_ASSERT_SAFE(rr_graph.node_ylow(curr_node) == rr_graph.node_yhigh(curr_node));
            int ipin_x = rr_graph.node_xlow(curr_node);
            int ipin_y = rr_graph.node_ylow(curr_node);
            int ipin_layer = rr_graph.node_layer(curr_node);

            if (ipin_x >= start_x && ipin_y >= start_y) {
                auto [delta_x, delta_y] = util::get_xy_deltas(start_node, curr_node);
                delta_x = std::abs(delta_x);
                delta_y = std::abs(delta_y);

                bool store_this_pin = true;
                if (!sample_all_locs) {
                    if (sample_locs.find(delta_x) == sample_locs.end()) {
                        store_this_pin = false;
                    } else {
                        if (sample_locs.at(delta_x).find(delta_y) == sample_locs.at(delta_x).end()) {
                            store_this_pin = false;
                        }
                    }
                }

                if (store_this_pin) {
                    routing_cost_map[ipin_layer][delta_x][delta_y].add_cost_entry(util::e_representative_entry_method::SMALLEST,
                                                                                  current.delay,
                                                                                  current.congestion_upstream);
                }
            }
        }

        expand_dijkstra_neighbours(current, node_visited_costs, node_expanded, pq);
        node_expanded[curr_node] = true;
    }
}

/* iterates over the children of the specified node and selectively pushes them onto the priority queue */
static void expand_dijkstra_neighbours(util::PQ_Entry parent_entry,
                                       vtr::vector<RRNodeId, float>& node_visited_costs,
                                       vtr::vector<RRNodeId, bool>& node_expanded,
                                       std::priority_queue<util::PQ_Entry>& pq) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    RRNodeId parent = parent_entry.rr_node;

    for (t_edge_size edge : rr_graph.edges(parent)) {
        RRNodeId child_node = rr_graph.edge_sink_node(parent, edge);
        // For the time being, we decide to not let the lookahead explore the node inside the clusters

        if (!is_inter_cluster_node(rr_graph, child_node)) {
            continue;
        }
        int switch_ind = size_t(rr_graph.edge_switch(parent, edge));

        if (rr_graph.node_type(child_node) == SINK) return;

        /* skip this child if it has already been expanded from */
        if (node_expanded[child_node]) {
            continue;
        }

        util::PQ_Entry child_entry(child_node, switch_ind, parent_entry.delay,
                                   parent_entry.R_upstream, parent_entry.congestion_upstream, false);

        //VTR_ASSERT(child_entry.cost >= 0); //Asertion fails in practise. TODO: debug

        /* skip this child if it has been visited with smaller cost */
        if (node_visited_costs[child_node] >= 0 && node_visited_costs[child_node] < child_entry.cost) {
            continue;
        }

        /* finally, record the cost with which the child was visited and put the child entry on the queue */
        node_visited_costs[child_node] = child_entry.cost;
        pq.push(child_entry);
    }
}

static std::pair<int, int> get_adjusted_rr_position(const RRNodeId rr) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    e_rr_type rr_type = rr_graph.node_type(rr);

    if (is_chan(rr_type)) {
        return get_adjusted_rr_wire_position(rr);
    } else if (is_pin(rr_type)) {
        return get_adjusted_rr_pin_position(rr);
    } else {
        VTR_ASSERT_SAFE(is_src_sink(rr_type));
        return get_adjusted_rr_src_sink_position(rr);
    }
}

static std::pair<int, int> get_adjusted_rr_pin_position(const RRNodeId rr) {
    /*
     * VPR uses a co-ordinate system where wires above and to the right of a block
     * are at the same location as the block:
     *
     *
     *       <-----------C
     *    D
     *    |  +---------+  ^
     *    |  |         |  |
     *    |  |  (1,1)  |  |
     *    |  |         |  |
     *    V  +---------+  |
     *                    A
     *     B----------->
     *
     * So wires are located as follows:
     *
     *      A: (1, 1) CHANY
     *      B: (1, 0) CHANX
     *      C: (1, 1) CHANX
     *      D: (0, 1) CHANY
     *
     * But all pins incident on the surrounding channels
     * would be at (1,1) along with a relevant side.
     *
     * In the following, we adjust the positions of pins to
     * account for the channel they are incident too.
     *
     * Note that blocks at (0,*) such as IOs, may have (unconnected)
     * pins on the left side, so we also clip the minimum x to zero.
     * Similarly for blocks at (*,0) we clip the minimum y to zero.
     */
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_graph;

    VTR_ASSERT_SAFE(is_pin(rr_graph.node_type(rr)));
    VTR_ASSERT_SAFE(rr_graph.node_xlow(rr) == rr_graph.node_xhigh(rr));
    VTR_ASSERT_SAFE(rr_graph.node_ylow(rr) == rr_graph.node_yhigh(rr));

    int x = rr_graph.node_xlow(rr);
    int y = rr_graph.node_ylow(rr);

    /* Use the first side we can find
     * Note that this may NOT return an accurate coordinate
     * when a rr node appears on different sides
     * However, current test show that the simple strategy provides
     * a good trade-off between runtime and quality of results
     */
    auto it = std::find_if(SIDES.begin(), SIDES.end(), [&](const e_side candidate_side) {
        return rr_graph.is_node_on_specific_side(rr, candidate_side);
    });

    e_side rr_side = (it != SIDES.end()) ? *it : NUM_SIDES;
    VTR_ASSERT_SAFE(NUM_SIDES != rr_side);

    if (rr_side == LEFT) {
        x -= 1;
        x = std::max(x, 0);
    } else if (rr_side == BOTTOM && y > 0) {
        y -= 1;
        y = std::max(y, 0);
    }

    return {x, y};
}

static std::pair<int, int> get_adjusted_rr_wire_position(const RRNodeId rr) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    VTR_ASSERT_SAFE(is_chan(rr_graph.node_type(rr)));

    Direction rr_dir = rr_graph.node_direction(rr);

    if (rr_dir == Direction::DEC) {
        return {rr_graph.node_xhigh(rr),
                rr_graph.node_yhigh(rr)};
    } else if (rr_dir == Direction::INC) {
        return {rr_graph.node_xlow(rr),
                rr_graph.node_ylow(rr)};
    } else {
        VTR_ASSERT_SAFE(rr_dir == Direction::BIDIR);
        //Not sure what to do here...
        //Try average for now.
        return {vtr::nint((rr_graph.node_xlow(rr) + rr_graph.node_xhigh(rr)) / 2.),
                vtr::nint((rr_graph.node_ylow(rr) + rr_graph.node_yhigh(rr)) / 2.)};
    }
}

static std::pair<int, int> get_adjusted_rr_src_sink_position(const RRNodeId rr) {
    //SOURCE/SINK nodes assume the full dimensions of their
    //associated block

    //Use the average position.
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    VTR_ASSERT_SAFE(is_src_sink(rr_graph.node_type(rr)));

    return {vtr::nint((rr_graph.node_xlow(rr) + rr_graph.node_xhigh(rr)) / 2.),
            vtr::nint((rr_graph.node_ylow(rr) + rr_graph.node_yhigh(rr)) / 2.)};
}
