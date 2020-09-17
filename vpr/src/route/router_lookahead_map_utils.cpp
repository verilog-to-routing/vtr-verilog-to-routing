#include "router_lookahead_map_utils.h"

/*
 * This file contains utility functions that can be shared among different
 * lookahead computation strategies.
 *
 * In general, this utility library contains:
 *
 * - Different dijkstra expansion alogrithms used to perform specific tasks, such as computing the SROURCE/OPIN --> CHAN lookup tables
 * - Cost Entries definitions used when generating and querying the lookahead
 *
 * To access the utility functions, the util namespace needs to be used.
 */

#include "globals.h"
#include "vpr_context.h"
#include "vtr_math.h"
#include "vtr_time.h"
#include "route_common.h"
#include "route_timing.h"

static void dijkstra_flood_to_wires(int itile, RRNodeId inode, util::t_src_opin_delays& src_opin_delays);
static void dijkstra_flood_to_ipins(RRNodeId node, util::t_chan_ipins_delays& chan_ipins_delays);

static vtr::Point<int> pick_sample_tile(t_physical_tile_type_ptr tile_type, vtr::Point<int> start);

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
#define DIRECT_CONNECT_SPECIAL_SEG_TYPE -1;

namespace util {

PQ_Entry::PQ_Entry(
    RRNodeId set_rr_node,
    int switch_ind,
    float parent_delay,
    float parent_R_upstream,
    float parent_congestion_upstream,
    bool starting_node,
    float Tsw_adjust) {
    this->rr_node = set_rr_node;

    auto& device_ctx = g_vpr_ctx.device();
    this->delay = parent_delay;
    this->congestion_upstream = parent_congestion_upstream;
    this->R_upstream = parent_R_upstream;
    if (!starting_node) {
        float Tsw = device_ctx.rr_switch_inf[switch_ind].Tdel;
        Tsw += Tsw_adjust;
        VTR_ASSERT(Tsw >= 0.f);
        float Rsw = device_ctx.rr_switch_inf[switch_ind].R;
        float Cnode = device_ctx.rr_nodes[size_t(set_rr_node)].C();
        float Rnode = device_ctx.rr_nodes[size_t(set_rr_node)].R();

        float T_linear = 0.f;
        if (device_ctx.rr_switch_inf[switch_ind].buffered()) {
            T_linear = Tsw + Rsw * Cnode + 0.5 * Rnode * Cnode;
        } else { /* Pass transistor */
            T_linear = Tsw + 0.5 * Rsw * Cnode;
        }

        float base_cost = 0.f;
        if (device_ctx.rr_switch_inf[switch_ind].configurable()) {
            base_cost = get_single_rr_cong_base_cost(size_t(set_rr_node));
        }

        VTR_ASSERT(T_linear >= 0.);
        VTR_ASSERT(base_cost >= 0.);
        this->delay += T_linear;

        this->congestion_upstream += base_cost;
    }

    /* set the cost of this node */
    this->cost = this->delay;
}

util::PQ_Entry_Delay::PQ_Entry_Delay(
    RRNodeId set_rr_node,
    int switch_ind,
    const util::PQ_Entry_Delay* parent) {
    this->rr_node = set_rr_node;

    if (parent != nullptr) {
        auto& device_ctx = g_vpr_ctx.device();
        float Tsw = device_ctx.rr_switch_inf[switch_ind].Tdel;
        float Rsw = device_ctx.rr_switch_inf[switch_ind].R;
        float Cnode = device_ctx.rr_nodes[size_t(set_rr_node)].C();
        float Rnode = device_ctx.rr_nodes[size_t(set_rr_node)].R();

        float T_linear = 0.f;
        if (device_ctx.rr_switch_inf[switch_ind].buffered()) {
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

util::PQ_Entry_Base_Cost::PQ_Entry_Base_Cost(
    RRNodeId set_rr_node,
    int switch_ind,
    const util::PQ_Entry_Base_Cost* parent) {
    this->rr_node = set_rr_node;

    if (parent != nullptr) {
        auto& device_ctx = g_vpr_ctx.device();
        if (device_ctx.rr_switch_inf[switch_ind].configurable()) {
            this->base_cost = parent->base_cost + get_single_rr_cong_base_cost(size_t(set_rr_node));
        } else {
            this->base_cost = parent->base_cost;
        }
    } else {
        this->base_cost = 0.f;
    }
}

/* returns cost entry with the smallest delay */
util::Cost_Entry util::Expansion_Cost_Entry::get_smallest_entry() const {
    util::Cost_Entry smallest_entry;

    for (auto entry : this->cost_vector) {
        if (!smallest_entry.valid() || entry.delay < smallest_entry.delay) {
            smallest_entry = entry;
        }
    }

    return smallest_entry;
}

/* returns a cost entry that represents the average of all the recorded entries */
util::Cost_Entry util::Expansion_Cost_Entry::get_average_entry() const {
    float avg_delay = 0;
    float avg_congestion = 0;

    for (auto cost_entry : this->cost_vector) {
        avg_delay += cost_entry.delay;
        avg_congestion += cost_entry.congestion;
    }

    avg_delay /= (float)this->cost_vector.size();
    avg_congestion /= (float)this->cost_vector.size();

    return util::Cost_Entry(avg_delay, avg_congestion);
}

/* returns a cost entry that represents the geomean of all the recorded entries */
util::Cost_Entry util::Expansion_Cost_Entry::get_geomean_entry() const {
    float geomean_delay = 0;
    float geomean_cong = 0;
    for (auto cost_entry : this->cost_vector) {
        geomean_delay += log(cost_entry.delay);
        geomean_cong += log(cost_entry.congestion);
    }

    geomean_delay = exp(geomean_delay / (float)this->cost_vector.size());
    geomean_cong = exp(geomean_cong / (float)this->cost_vector.size());

    return util::Cost_Entry(geomean_delay, geomean_cong);
}

/* returns a cost entry that represents the medial of all recorded entries */
util::Cost_Entry util::Expansion_Cost_Entry::get_median_entry() const {
    /* find median by binning the delays of all entries and then chosing the bin
     * with the largest number of entries */

    // This is code that needs to be revisited. For the time being, if the median entry
    // method calculation is used an assertion is thrown.
    VTR_ASSERT_MSG(false, "Get median entry calculation method is not implemented!");

    int num_bins = 10;

    /* find entries with smallest and largest delays */
    util::Cost_Entry min_del_entry;
    util::Cost_Entry max_del_entry;
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
    std::vector<std::vector<util::Cost_Entry>> entry_bins(num_bins, std::vector<util::Cost_Entry>());
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
    util::Cost_Entry representative_entry = entry_bins[largest_bin][0];

    return representative_entry;
}

template<typename Entry>
void expand_dijkstra_neighbours(const t_rr_graph_storage& rr_nodes,
                                const Entry& parent_entry,
                                std::vector<util::Search_Path>* paths,
                                std::vector<bool>* node_expanded,
                                std::priority_queue<Entry,
                                                    std::vector<Entry>,
                                                    std::greater<Entry>>* pq) {
    RRNodeId parent = parent_entry.rr_node;

    auto& parent_node = rr_nodes[size_t(parent)];

    for (int iedge = 0; iedge < parent_node.num_edges(); iedge++) {
        int child_node_ind = parent_node.edge_sink_node(iedge);
        int switch_ind = parent_node.edge_switch(iedge);

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

template void expand_dijkstra_neighbours(const t_rr_graph_storage& rr_nodes,
                                         const PQ_Entry_Delay& parent_entry,
                                         std::vector<Search_Path>* paths,
                                         std::vector<bool>* node_expanded,
                                         std::priority_queue<PQ_Entry_Delay,
                                                             std::vector<PQ_Entry_Delay>,
                                                             std::greater<PQ_Entry_Delay>>* pq);
template void expand_dijkstra_neighbours(const t_rr_graph_storage& rr_nodes,
                                         const PQ_Entry_Base_Cost& parent_entry,
                                         std::vector<Search_Path>* paths,
                                         std::vector<bool>* node_expanded,
                                         std::priority_queue<PQ_Entry_Base_Cost,
                                                             std::vector<PQ_Entry_Base_Cost>,
                                                             std::greater<PQ_Entry_Base_Cost>>* pq);

t_src_opin_delays compute_router_src_opin_lookahead() {
    vtr::ScopedStartFinishTimer timer("Computing src/opin lookahead");
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    t_src_opin_delays src_opin_delays;

    src_opin_delays.resize(device_ctx.physical_tile_types.size());

    std::vector<int> rr_nodes_at_loc;

    //We assume that the routing connectivity of each instance of a physical tile is the same,
    //and so only measure one instance of each type
    for (size_t itile = 0; itile < device_ctx.physical_tile_types.size(); ++itile) {
        for (e_rr_type rr_type : {SOURCE, OPIN}) {
            vtr::Point<int> sample_loc(-1, -1);

            size_t num_sampled_locs = 0;
            bool ptcs_with_no_delays = true;
            while (ptcs_with_no_delays) { //Haven't found wire connected to ptc
                ptcs_with_no_delays = false;

                sample_loc = pick_sample_tile(&device_ctx.physical_tile_types[itile], sample_loc);

                if (sample_loc.x() == -1 && sample_loc.y() == -1) {
                    //No untried instances of the current tile type left
                    VTR_LOG_WARN("Found no %ssample locations for %s in %s\n",
                                 (num_sampled_locs == 0) ? "" : "more ",
                                 rr_node_typename[rr_type],
                                 device_ctx.physical_tile_types[itile].name);
                    break;
                }

                //VTR_LOG("Sampling %s at (%d,%d)\n", device_ctx.physical_tile_types[itile].name, sample_loc.x(), sample_loc.y());

                rr_nodes_at_loc.clear();

                get_rr_node_indices(device_ctx.rr_node_indices, sample_loc.x(), sample_loc.y(), rr_type, &rr_nodes_at_loc);
                for (int inode : rr_nodes_at_loc) {
                    if (inode < 0) continue;

                    RRNodeId node_id(inode);

                    int ptc = rr_graph.node_ptc_num(node_id);

                    if (ptc >= int(src_opin_delays[itile].size())) {
                        src_opin_delays[itile].resize(ptc + 1); //Inefficient but functional...
                    }

                    //Find the wire types which are reachable from inode and record them and
                    //the cost to reach them
                    dijkstra_flood_to_wires(itile, node_id, src_opin_delays);

                    if (src_opin_delays[itile][ptc].empty()) {
                        VTR_LOGV_DEBUG(f_router_debug, "Found no reachable wires from %s (%s) at (%d,%d)\n",
                                       rr_node_typename[rr_type],
                                       rr_node_arch_name(inode).c_str(),
                                       sample_loc.x(),
                                       sample_loc.y());

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

    return src_opin_delays;
}

t_chan_ipins_delays compute_router_chan_ipin_lookahead() {
    vtr::ScopedStartFinishTimer timer("Computing chan/ipin lookahead");
    auto& device_ctx = g_vpr_ctx.device();

    t_chan_ipins_delays chan_ipins_delays;

    chan_ipins_delays.resize(device_ctx.physical_tile_types.size());

    std::vector<int> rr_nodes_at_loc;

    //We assume that the routing connectivity of each instance of a physical tile is the same,
    //and so only measure one instance of each type
    for (auto tile_type : device_ctx.physical_tile_types) {
        vtr::Point<int> sample_loc(-1, -1);

        sample_loc = pick_sample_tile(&tile_type, sample_loc);

        if (sample_loc.x() == -1 && sample_loc.y() == -1) {
            //No untried instances of the current tile type left
            VTR_LOG_WARN("Found no sample locations for %s\n",
                         tile_type.name);
            continue;
        }

        int min_x = std::max(0, sample_loc.x() - X_OFFSET);
        int min_y = std::max(0, sample_loc.y() - Y_OFFSET);
        int max_x = std::min(int(device_ctx.grid.width()), sample_loc.x() + X_OFFSET);
        int max_y = std::min(int(device_ctx.grid.height()), sample_loc.y() + Y_OFFSET);

        for (int ix = min_x; ix < max_x; ix++) {
            for (int iy = min_y; iy < max_y; iy++) {
                for (auto rr_type : {CHANX, CHANY}) {
                    rr_nodes_at_loc.clear();

                    get_rr_node_indices(device_ctx.rr_node_indices, ix, iy, rr_type, &rr_nodes_at_loc);
                    for (int inode : rr_nodes_at_loc) {
                        if (inode < 0) continue;

                        RRNodeId node_id(inode);

                        //Find the IPINs which are reachable from the wires within the bounding box
                        //around the selected tile location
                        dijkstra_flood_to_ipins(node_id, chan_ipins_delays);
                    }
                }
            }
        }
    }

    return chan_ipins_delays;
}

} // namespace util

static void dijkstra_flood_to_wires(int itile, RRNodeId node, util::t_src_opin_delays& src_opin_delays) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

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

    /*
     * Perform Djikstra from the SOURCE/OPIN of interest, stopping at the the first
     * reachable wires (i.e until we hit the inter-block routing network), or a SINK
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
        if (curr_rr_type == CHANX || curr_rr_type == CHANY || curr_rr_type == SINK) {
            //We stop expansion at any CHANX/CHANY/SINK
            int seg_index;
            if (curr_rr_type != SINK) {
                //It's a wire, figure out its type
                int cost_index = rr_graph.node_cost_index(curr.node);
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
            if (!src_opin_delays[itile][ptc].count(seg_index)
                || curr.delay < src_opin_delays[itile][ptc][seg_index].delay) {
                src_opin_delays[itile][ptc][seg_index].wire_rr_type = curr_rr_type;
                src_opin_delays[itile][ptc][seg_index].wire_seg_index = seg_index;
                src_opin_delays[itile][ptc][seg_index].delay = curr.delay;
                src_opin_delays[itile][ptc][seg_index].congestion = curr.congestion;
            }

        } else if (curr_rr_type == SOURCE || curr_rr_type == OPIN || curr_rr_type == IPIN) {
            //We allow expansion through SOURCE/OPIN/IPIN types
            int cost_index = rr_graph.node_cost_index(curr.node);
            float incr_cong = device_ctx.rr_indexed_data[cost_index].base_cost; //Current nodes congestion cost

            for (RREdgeId edge : rr_graph.edge_range(curr.node)) {
                int iswitch = rr_graph.edge_switch(edge);
                float incr_delay = device_ctx.rr_switch_inf[iswitch].Tdel;

                RRNodeId next_node = rr_graph.edge_sink_node(edge);

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
    auto& rr_graph = device_ctx.rr_nodes;

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

            auto tile_type = device_ctx.grid[node_x][node_y].type;
            int itile = tile_type->index;

            int ptc = rr_graph.node_ptc_num(curr.node);

            if (ptc >= int(chan_ipins_delays[itile].size())) {
                chan_ipins_delays[itile].resize(ptc + 1); //Inefficient but functional...
            }

            site_pin_delay = std::min(curr.delay, site_pin_delay);
            //Keep costs of the best path to reach each wire type
            chan_ipins_delays[itile][ptc].wire_rr_type = curr_rr_type;
            chan_ipins_delays[itile][ptc].delay = site_pin_delay;
            chan_ipins_delays[itile][ptc].congestion = curr.congestion;
        } else if (curr_rr_type == CHANX || curr_rr_type == CHANY) {
            if (curr.level >= MAX_EXPANSION_LEVEL) {
                continue;
            }

            //We allow expansion through SOURCE/OPIN/IPIN types
            int cost_index = rr_graph.node_cost_index(curr.node);
            float new_cong = device_ctx.rr_indexed_data[cost_index].base_cost; //Current nodes congestion cost

            for (RREdgeId edge : rr_graph.edge_range(curr.node)) {
                int iswitch = rr_graph.edge_switch(edge);
                float new_delay = device_ctx.rr_switch_inf[iswitch].Tdel;

                RRNodeId next_node = rr_graph.edge_sink_node(edge);

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

static vtr::Point<int> pick_sample_tile(t_physical_tile_type_ptr tile_type, vtr::Point<int> prev) {
    //Very simple for now, just pick the fist matching tile found
    vtr::Point<int> loc(OPEN, OPEN);

    //VTR_LOG("Prev: %d,%d\n", prev.x(), prev.y());

    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

    int y_init = prev.y() + 1; //Start searching next element above prev

    for (int x = prev.x(); x < int(grid.width()); ++x) {
        if (x < 0) continue;

        //VTR_LOG("  x: %d\n", x);

        for (int y = y_init; y < int(grid.height()); ++y) {
            if (y < 0) continue;

            //VTR_LOG("   y: %d\n", y);
            if (grid[x][y].type == tile_type) {
                loc.set_x(x);
                loc.set_y(y);
                break;
            }
        }

        if (loc.x() != OPEN && loc.y() != OPEN) {
            break;
        } else {
            y_init = 0; //Prepare to search next column
        }
    }
    //VTR_LOG("Next: %d,%d\n", loc.x(), loc.y());

    return loc;
}
