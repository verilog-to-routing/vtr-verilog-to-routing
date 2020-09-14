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
 * In the case of SRC/OPIN -> SINK estimates the resuls from the two look-ups are added together (and the minimum taken
 * if there are multiple possiblities).
 */

#include <cmath>
#include <vector>
#include <queue>
#include <ctime>
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
#include "route_timing.h"

#ifdef VTR_ENABLE_CAPNPROTO
#    include "capnp/serialize.h"
#    include "map_lookahead.capnp.h"
#    include "ndmatrix_serdes.h"
#    include "mmap_file.h"
#    include "serdes_utils.h"
#endif /* VTR_ENABLE_CAPNPROTO */

/* we will profile delay/congestion using this many tracks for each wire type */
#define MAX_TRACK_OFFSET 16

/* we're profiling routing cost over many tracks for each wire type, so we'll have many cost entries at each |dx|,|dy| offset.
 * there are many ways to "boil down" the many costs at each offset to a single entry for a given (wire type, chan_type) combination --
 * we can take the smallest cost, the average, median, etc. This define selects the method we use.
 * See e_representative_entry_method */
#define REPRESENTATIVE_ENTRY_METHOD SMALLEST

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

/* a class that stores delay/congestion information for a given relative coordinate during the Dijkstra expansion.
 * since it stores multiple cost entries, it is later boiled down to a single representative cost entry to be stored
 * in the final lookahead cost map */
class Expansion_Cost_Entry {
  public:
    std::vector<Cost_Entry> cost_vector;

  private:
    Cost_Entry get_smallest_entry();
    Cost_Entry get_average_entry();
    Cost_Entry get_geomean_entry();
    Cost_Entry get_median_entry();

  public:
    void add_cost_entry(float add_delay, float add_congestion) {
        Cost_Entry cost_entry(add_delay, add_congestion);
        if (REPRESENTATIVE_ENTRY_METHOD == SMALLEST) {
            /* taking the smallest-delay entry anyway, so no need to push back multple entries */
            if (this->cost_vector.empty()) {
                this->cost_vector.push_back(cost_entry);
            } else {
                if (add_delay < this->cost_vector[0].delay) {
                    this->cost_vector[0] = cost_entry;
                }
            }
        } else {
            this->cost_vector.push_back(cost_entry);
        }
    }
    void clear_cost_entries() {
        this->cost_vector.clear();
    }

    Cost_Entry get_representative_cost_entry(e_representative_entry_method method) {
        float nan = std::numeric_limits<float>::quiet_NaN();
        Cost_Entry entry(nan, nan);

        if (!cost_vector.empty()) {
            switch (method) {
                case FIRST:
                    entry = cost_vector[0];
                    break;
                case SMALLEST:
                    entry = this->get_smallest_entry();
                    break;
                case AVERAGE:
                    entry = this->get_average_entry();
                    break;
                case GEOMEAN:
                    entry = this->get_geomean_entry();
                    break;
                case MEDIAN:
                    entry = this->get_median_entry();
                    break;
                default:
                    break;
            }
        }
        return entry;
    }
};

/* a class that represents an entry in the Dijkstra expansion priority queue */
class PQ_Entry {
  public:
    RRNodeId rr_node; //index in device_ctx.rr_nodes that this entry represents
    float cost;       //the cost of the path to get to this node

    /* store backward delay, R and congestion info */
    float delay;
    float R_upstream;
    float congestion_upstream;

    PQ_Entry(RRNodeId set_rr_node, int /*switch_ind*/, float parent_delay, float parent_R_upstream, float parent_congestion_upstream, bool starting_node) {
        this->rr_node = set_rr_node;

        auto& device_ctx = g_vpr_ctx.device();
        this->delay = parent_delay;
        this->congestion_upstream = parent_congestion_upstream;
        this->R_upstream = parent_R_upstream;
        if (!starting_node) {
            int cost_index = device_ctx.rr_nodes.node_cost_index(RRNodeId(set_rr_node));
            //this->delay += g_rr_nodes[set_rr_node].C() * (g_rr_switch_inf[switch_ind].R + 0.5*g_rr_nodes[set_rr_node].R()) +
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

    bool operator<(const PQ_Entry& obj) const {
        /* inserted into max priority queue so want queue entries with a lower cost to be greater */
        return (this->cost > obj.cost);
    }
};

/* used during Dijkstra expansion to store delay/congestion info lists for each relative coordinate for a given segment and channel type.
 * the list at each coordinate is later boiled down to a single representative cost entry to be stored in the final cost map */
typedef vtr::Matrix<Expansion_Cost_Entry> t_routing_cost_map; //[0..device_ctx.grid.width()-1][0..device_ctx.grid.height()-1]

struct t_dijkstra_data {
    /* a list of boolean flags (one for each rr node) to figure out if a certain node has already been expanded */
    vtr::vector<RRNodeId, bool> node_expanded;
    /* for each node keep a list of the cost with which that node has been visited (used to determine whether to push
     * a candidate node onto the expansion queue */
    vtr::vector<RRNodeId, float> node_visited_costs;
    /* a priority queue for expansion */
    std::priority_queue<PQ_Entry> pq;
};

/******** File-Scope Variables ********/

//Look-up table from CHANX/CHANY (to SINKs) for various distances
t_wire_cost_map f_wire_cost_map;

/******** File-Scope Functions ********/
Cost_Entry get_wire_cost_entry(e_rr_type rr_type, int seg_index, int delta_x, int delta_y);
static void compute_router_wire_lookahead(const std::vector<t_segment_inf>& segment_inf);

/* returns index of a node from which to start routing */
static RRNodeId get_start_node(int start_x, int start_y, int target_x, int target_y, t_rr_type rr_type, int seg_index, int track_offset);
/* runs Dijkstra's algorithm from specified node until all nodes have been visited. Each time a pin is visited, the delay/congestion information
 * to that pin is stored is added to an entry in the routing_cost_map */
static void run_dijkstra(RRNodeId start_node, int start_x, int start_y, t_routing_cost_map& routing_cost_map, t_dijkstra_data* data);
/* iterates over the children of the specified node and selectively pushes them onto the priority queue */
static void expand_dijkstra_neighbours(PQ_Entry parent_entry, vtr::vector<RRNodeId, float>& node_visited_costs, vtr::vector<RRNodeId, bool>& node_expanded, std::priority_queue<PQ_Entry>& pq);
/* sets the lookahead cost map entries based on representative cost entries from routing_cost_map */
static void set_lookahead_map_costs(int segment_index, e_rr_type chan_type, t_routing_cost_map& routing_cost_map);
/* fills in missing lookahead map entries by copying the cost of the closest valid entry */
static void fill_in_missing_lookahead_entries(int segment_index, e_rr_type chan_type);
/* returns a cost entry in the f_wire_cost_map that is near the specified coordinates (and preferably towards (0,0)) */
static Cost_Entry get_nearby_cost_entry(int x, int y, int segment_index, int chan_index);
/* returns the absolute delta_x and delta_y offset required to reach to_node from from_node */
static void get_xy_deltas(const RRNodeId from_node, const RRNodeId to_node, int* delta_x, int* delta_y);
static void adjust_rr_position(const RRNodeId rr, int& x, int& y);
static void adjust_rr_pin_position(const RRNodeId rr, int& x, int& y);
static void adjust_rr_wire_position(const RRNodeId rr, int& x, int& y);
static void adjust_rr_src_sink_position(const RRNodeId rr, int& x, int& y);

static void print_wire_cost_map(const std::vector<t_segment_inf>& segment_inf);
static void print_router_cost_map(const t_routing_cost_map& router_cost_map);

/******** Interface class member function definitions ********/
float MapLookahead::get_expected_cost(RRNodeId current_node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    t_rr_type rr_type = rr_graph.node_type(current_node);

    if (rr_type == CHANX || rr_type == CHANY || rr_type == SOURCE || rr_type == OPIN) {
        float delay_cost, cong_cost;

        // Get the total cost using the combined delay and congestion costs
        std::tie(delay_cost, cong_cost) = get_expected_delay_and_cong(current_node, target_node, params, R_upstream);
        return delay_cost + cong_cost;
    } else if (rr_type == IPIN) { /* Change if you're allowing route-throughs */
        return (device_ctx.rr_indexed_data[SINK_COST_INDEX].base_cost);
    } else { /* Change this if you want to investigate route-throughs */
        return (0.);
    }
}

/******** Function Definitions ********/
/* queries the lookahead_map (should have been computed prior to routing) to get the expected cost
 * from the specified source to the specified target */
std::pair<float, float> MapLookahead::get_expected_delay_and_cong(RRNodeId from_node, RRNodeId to_node, const t_conn_cost_params& params, float /*R_upstream*/) const {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    int delta_x, delta_y;
    get_xy_deltas(from_node, to_node, &delta_x, &delta_y);
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

        t_physical_tile_type_ptr tile_type = device_ctx.grid[rr_graph.node_xlow(from_node)][rr_graph.node_ylow(from_node)].type;
        auto tile_index = std::distance(&device_ctx.physical_tile_types[0], tile_type);

        auto from_ptc = rr_graph.node_ptc_num(from_node);

        if (this->src_opin_delays[tile_index][from_ptc].empty()) {
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
            //down by a large factor to maintain some dynaimc range in case this value ends
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
            for (const auto& kv : this->src_opin_delays[tile_index][from_ptc]) {
                const util::t_reachable_wire_inf& reachable_wire_inf = kv.second;

                Cost_Entry wire_cost_entry;
                if (reachable_wire_inf.wire_rr_type == SINK) {
                    //Some pins maybe reachable via a direct (OPIN -> IPIN) connection.
                    //In the lookahead, we treat such connections as 'special' wire types
                    //with no delay/congestion cost
                    wire_cost_entry.delay = 0;
                    wire_cost_entry.congestion = 0;
                } else {
                    //For an actual accessible wire, we query the wire look-up to get it's
                    //delay and congestion cost estimates
                    wire_cost_entry = get_wire_cost_entry(reachable_wire_inf.wire_rr_type, reachable_wire_inf.wire_seg_index, delta_x, delta_y);
                }

                float this_delay_cost = (1. - params.criticality) * (reachable_wire_inf.delay + wire_cost_entry.delay);
                float this_cong_cost = (params.criticality) * (reachable_wire_inf.congestion + wire_cost_entry.congestion);

                expected_delay_cost = std::min(expected_delay_cost, this_delay_cost);
                expected_cong_cost = std::min(expected_cong_cost, this_cong_cost);
            }
        }

        VTR_ASSERT_SAFE_MSG(std::isfinite(expected_delay_cost),
                            vtr::string_fmt("Lookahead failed to estimate cost from %s: %s",
                                            rr_node_arch_name(size_t(from_node)).c_str(),
                                            describe_rr_node(size_t(from_node)).c_str())
                                .c_str());

    } else if (from_type == CHANX || from_type == CHANY) {
        VTR_ASSERT_SAFE(from_type == CHANX || from_type == CHANY);
        //When estimating costs from a wire, we directly look-up the result in the wire lookahead (f_wire_cost_map)

        int from_cost_index = rr_graph.node_cost_index(from_node);
        int from_seg_index = device_ctx.rr_indexed_data[from_cost_index].seg_index;

        VTR_ASSERT(from_seg_index >= 0);

        /* now get the expected cost from our lookahead map */
        Cost_Entry cost_entry = get_wire_cost_entry(from_type, from_seg_index, delta_x, delta_y);

        float expected_delay = cost_entry.delay;
        float expected_cong = cost_entry.congestion;

        expected_delay_cost = params.criticality * expected_delay;
        expected_cong_cost = (1.0 - params.criticality) * expected_cong;

        VTR_ASSERT_SAFE_MSG(std::isfinite(expected_delay_cost),
                            vtr::string_fmt("Lookahead failed to estimate cost from %s: %s",
                                            rr_node_arch_name(size_t(from_node)).c_str(),
                                            describe_rr_node(size_t(from_node)).c_str())
                                .c_str());
    } else if (from_type == IPIN) { /* Change if you're allowing route-throughs */
        return std::make_pair(0., device_ctx.rr_indexed_data[SINK_COST_INDEX].base_cost);
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
    this->src_opin_delays = util::compute_router_src_opin_lookahead();
}

void MapLookahead::read(const std::string& file) {
    read_router_lookahead(file);

    //Next, compute which wire types are accessible (and the cost to reach them)
    //from the different physical tile type's SOURCEs & OPINs
    this->src_opin_delays = util::compute_router_src_opin_lookahead();
}

void MapLookahead::write(const std::string& file) const {
    write_router_lookahead(file);
}

/******** Function Definitions ********/

Cost_Entry get_wire_cost_entry(e_rr_type rr_type, int seg_index, int delta_x, int delta_y) {
    VTR_ASSERT_SAFE(rr_type == CHANX || rr_type == CHANY);

    int chan_index = 0;
    if (rr_type == CHANY) {
        chan_index = 1;
    }

    VTR_ASSERT_SAFE(delta_x < (int)f_wire_cost_map.dim_size(2));
    VTR_ASSERT_SAFE(delta_y < (int)f_wire_cost_map.dim_size(3));

    return f_wire_cost_map[chan_index][seg_index][delta_x][delta_y];
}

static void compute_router_wire_lookahead(const std::vector<t_segment_inf>& segment_inf) {
    vtr::ScopedStartFinishTimer timer("Computing wire lookahead");

    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;
    auto& rr_graph = device_ctx.rr_nodes;

    //Re-allocate
    f_wire_cost_map = t_wire_cost_map({2, segment_inf.size(), device_ctx.grid.width(), device_ctx.grid.height()});

    int longest_length = 0;
    for (const auto& seg_inf : segment_inf) {
        longest_length = std::max(longest_length, seg_inf.length);
    }

    //Start sampling at the lower left non-corner
    int ref_x = 1;
    int ref_y = 1;

    //Sample from locations near the reference location (to capture maximum distance paths)
    //Also sample from locations at least the longest wire length away from the edge (to avoid
    //edge effects for shorter distances)
    std::vector<int> ref_increments = {0, 1,
                                       longest_length, longest_length + 1};

    //Uniquify the increments (avoid sampling the same locations repeatedly if they happen to
    //overlap)
    std::sort(ref_increments.begin(), ref_increments.end());
    ref_increments.erase(std::unique(ref_increments.begin(), ref_increments.end()), ref_increments.end());

    //Upper right non-corner
    int target_x = device_ctx.grid.width() - 2;
    int target_y = device_ctx.grid.height() - 2;

    //Profile each wire segment type
    for (int iseg = 0; iseg < int(segment_inf.size()); iseg++) {
        //First try to pick good representative sample locations for each type
        std::map<t_rr_type, std::vector<RRNodeId>> sample_nodes;
        for (e_rr_type chan_type : {CHANX, CHANY}) {
            for (int ref_inc : ref_increments) {
                int sample_x = ref_x + ref_inc;
                int sample_y = ref_y + ref_inc;

                if (sample_x >= int(grid.width())) continue;
                if (sample_y >= int(grid.height())) continue;

                for (int track_offset = 0; track_offset < MAX_TRACK_OFFSET; track_offset += 2) {
                    /* get the rr node index from which to start routing */
                    RRNodeId start_node = get_start_node(sample_x, sample_y,
                                                         target_x, target_y, //non-corner upper right
                                                         chan_type, iseg, track_offset);

                    if (!start_node) {
                        continue;
                    }

                    sample_nodes[chan_type].push_back(RRNodeId(start_node));
                }
            }
        }

        //If we failed to find any representative sample locations, search exhaustively
        //
        //This is to ensure we sample 'unusual' wire types which may not exist in all channels
        //(e.g. clock routing)
        for (e_rr_type chan_type : {CHANX, CHANY}) {
            if (!sample_nodes[chan_type].empty()) continue;

            //Try an exhaustive search to find a suitable sample point
            for (int inode = 0; inode < int(device_ctx.rr_nodes.size()); ++inode) {
                auto rr_node = RRNodeId(inode);
                auto rr_type = rr_graph.node_type(rr_node);
                if (rr_type != chan_type) continue;

                int cost_index = rr_graph.node_cost_index(rr_node);
                VTR_ASSERT(cost_index != OPEN);

                int seg_index = device_ctx.rr_indexed_data[cost_index].seg_index;

                if (seg_index == iseg) {
                    sample_nodes[chan_type].push_back(RRNodeId(inode));
                }

                if (sample_nodes[chan_type].size() >= ref_increments.size()) {
                    break;
                }
            }
        }

        //Finally, now that we have a list of sample locations, run a Djikstra flood from
        //each sample location to profile the routing network from this type

        t_dijkstra_data dijkstra_data;
        t_routing_cost_map routing_cost_map({device_ctx.grid.width(), device_ctx.grid.height()});

        for (e_rr_type chan_type : {CHANX, CHANY}) {
            if (sample_nodes[chan_type].empty()) {
                VTR_LOG_WARN("Unable to find any sample location for segment %s type '%s' (length %d)\n",
                             rr_node_typename[chan_type],
                             segment_inf[iseg].name.c_str(),
                             segment_inf[iseg].length);
            } else {
                //reset cost for this segment
                routing_cost_map.fill(Expansion_Cost_Entry());

                for (RRNodeId sample_node : sample_nodes[chan_type]) {
                    int sample_x = rr_graph.node_xlow(sample_node);
                    int sample_y = rr_graph.node_ylow(sample_node);

                    if (rr_graph.node_direction(sample_node) == DEC_DIRECTION) {
                        sample_x = rr_graph.node_xhigh(sample_node);
                        sample_y = rr_graph.node_yhigh(sample_node);
                    }

                    run_dijkstra(sample_node,
                                 sample_x,
                                 sample_y,
                                 routing_cost_map,
                                 &dijkstra_data);
                }

                if (false) print_router_cost_map(routing_cost_map);

                /* boil down the cost list in routing_cost_map at each coordinate to a representative cost entry and store it in the lookahead
                 * cost map */
                set_lookahead_map_costs(iseg, chan_type, routing_cost_map);

                /* fill in missing entries in the lookahead cost map by copying the closest cost entries (cost map was computed based on
                 * a reference coordinate > (0,0) so some entries that represent a cross-chip distance have not been computed) */
                fill_in_missing_lookahead_entries(iseg, chan_type);
            }
        }
    }

    if (false) print_wire_cost_map(segment_inf);
}

/* returns index of a node from which to start routing */
static RRNodeId get_start_node(int start_x, int start_y, int target_x, int target_y, t_rr_type rr_type, int seg_index, int track_offset) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    int result = UNDEFINED;

    if (rr_type != CHANX && rr_type != CHANY) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Must start lookahead routing from CHANX or CHANY node\n");
    }

    /* determine which direction the wire should go in based on the start & target coordinates */
    e_direction direction = INC_DIRECTION;
    if ((rr_type == CHANX && target_x < start_x) || (rr_type == CHANY && target_y < start_y)) {
        direction = DEC_DIRECTION;
    }

    int start_lookup_x = start_x;
    int start_lookup_y = start_y;
    if (rr_type == CHANX) {
        //Bizarely, rr_node_indices stores CHANX with swapped x/y...
        std::swap(start_lookup_x, start_lookup_y);
    }

    const std::vector<int>& channel_node_list = device_ctx.rr_node_indices[rr_type][start_lookup_x][start_lookup_y][0];

    /* find first node in channel that has specified segment index and goes in the desired direction */
    for (unsigned itrack = 0; itrack < channel_node_list.size(); itrack++) {
        int node_ind = channel_node_list[itrack];
        if (node_ind < 0) continue;

        RRNodeId node_id(node_ind);

        VTR_ASSERT(rr_graph.node_type(node_id) == rr_type);

        e_direction node_direction = rr_graph.node_direction(node_id);
        int node_cost_ind = rr_graph.node_cost_index(node_id);
        int node_seg_ind = device_ctx.rr_indexed_data[node_cost_ind].seg_index;

        if ((node_direction == direction || node_direction == BI_DIRECTION) && node_seg_ind == seg_index) {
            /* found first track that has the specified segment index and goes in the desired direction */
            result = node_ind;
            if (track_offset == 0) {
                break;
            }
            track_offset -= 2;
        }
    }

    return RRNodeId(result);
}

/* runs Dijkstra's algorithm from specified node until all nodes have been visited. Each time a pin is visited, the delay/congestion information
 * to that pin is stored is added to an entry in the routing_cost_map */
static void run_dijkstra(RRNodeId start_node, int start_x, int start_y, t_routing_cost_map& routing_cost_map, t_dijkstra_data* data) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    auto& node_expanded = data->node_expanded;
    node_expanded.resize(device_ctx.rr_nodes.size());
    std::fill(node_expanded.begin(), node_expanded.end(), false);

    auto& node_visited_costs = data->node_visited_costs;
    node_visited_costs.resize(device_ctx.rr_nodes.size());
    std::fill(node_visited_costs.begin(), node_visited_costs.end(), -1.0);

    /* a priority queue for expansion */
    std::priority_queue<PQ_Entry>& pq = data->pq;

    //Clear priority queue if non-empty
    while (!pq.empty()) {
        pq.pop();
    }

    /* first entry has no upstream delay or congestion */
    PQ_Entry first_entry(start_node, UNDEFINED, 0, 0, 0, true);

    pq.push(first_entry);

    /* now do routing */
    while (!pq.empty()) {
        PQ_Entry current = pq.top();
        pq.pop();

        RRNodeId curr_node = current.rr_node;

        /* check that we haven't already expanded from this node */
        if (node_expanded[curr_node]) {
            continue;
        }

        //VTR_LOG("Expanding with delay=%10.3g cong=%10.3g (%s)\n", current.delay, current.congestion_upstream, describe_rr_node(curr_node).c_str());

        /* if this node is an ipin record its congestion/delay in the routing_cost_map */
        if (rr_graph.node_type(curr_node) == IPIN) {
            int ipin_x = rr_graph.node_xlow(curr_node);
            int ipin_y = rr_graph.node_ylow(curr_node);

            if (ipin_x >= start_x && ipin_y >= start_y) {
                int delta_x, delta_y;
                get_xy_deltas(start_node, curr_node, &delta_x, &delta_y);
                delta_x = std::abs(delta_x);
                delta_y = std::abs(delta_y);

                routing_cost_map[delta_x][delta_y].add_cost_entry(current.delay, current.congestion_upstream);
            }
        }

        expand_dijkstra_neighbours(current, node_visited_costs, node_expanded, pq);
        node_expanded[curr_node] = true;
    }
}

/* iterates over the children of the specified node and selectively pushes them onto the priority queue */
static void expand_dijkstra_neighbours(PQ_Entry parent_entry, vtr::vector<RRNodeId, float>& node_visited_costs, vtr::vector<RRNodeId, bool>& node_expanded, std::priority_queue<PQ_Entry>& pq) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    RRNodeId parent = parent_entry.rr_node;

    for (RREdgeId edge : rr_graph.edge_range(parent)) {
        RRNodeId child_node = rr_graph.edge_sink_node(edge);
        int switch_ind = rr_graph.edge_switch(edge);

        if (rr_graph.node_type(child_node) == SINK) return;

        /* skip this child if it has already been expanded from */
        if (node_expanded[child_node]) {
            continue;
        }

        PQ_Entry child_entry(child_node, switch_ind, parent_entry.delay,
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

/* sets the lookahead cost map entries based on representative cost entries from routing_cost_map */
static void set_lookahead_map_costs(int segment_index, e_rr_type chan_type, t_routing_cost_map& routing_cost_map) {
    int chan_index = 0;
    if (chan_type == CHANY) {
        chan_index = 1;
    }

    /* set the lookahead cost map entries with a representative cost entry from routing_cost_map */
    for (unsigned ix = 0; ix < routing_cost_map.dim_size(0); ix++) {
        for (unsigned iy = 0; iy < routing_cost_map.dim_size(1); iy++) {
            Expansion_Cost_Entry& expansion_cost_entry = routing_cost_map[ix][iy];

            f_wire_cost_map[chan_index][segment_index][ix][iy] = expansion_cost_entry.get_representative_cost_entry(REPRESENTATIVE_ENTRY_METHOD);
        }
    }
}

/* fills in missing lookahead map entries by copying the cost of the closest valid entry */
static void fill_in_missing_lookahead_entries(int segment_index, e_rr_type chan_type) {
    int chan_index = 0;
    if (chan_type == CHANY) {
        chan_index = 1;
    }

    auto& device_ctx = g_vpr_ctx.device();

    /* find missing cost entries and fill them in by copying a nearby cost entry */
    for (unsigned ix = 0; ix < device_ctx.grid.width(); ix++) {
        for (unsigned iy = 0; iy < device_ctx.grid.height(); iy++) {
            Cost_Entry cost_entry = f_wire_cost_map[chan_index][segment_index][ix][iy];

            if (std::isnan(cost_entry.delay) && std::isnan(cost_entry.congestion)) {
                Cost_Entry copied_entry = get_nearby_cost_entry(ix, iy, segment_index, chan_index);
                f_wire_cost_map[chan_index][segment_index][ix][iy] = copied_entry;
            }
        }
    }
}

/* returns a cost entry in the f_wire_cost_map that is near the specified coordinates (and preferably towards (0,0)) */
static Cost_Entry get_nearby_cost_entry(int x, int y, int segment_index, int chan_index) {
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

    Cost_Entry copy_entry = f_wire_cost_map[chan_index][segment_index][copy_x][copy_y];

    /* if the entry to be copied is also empty, recurse */
    if (std::isnan(copy_entry.delay) && std::isnan(copy_entry.congestion)) {
        if (copy_x == 0 && copy_y == 0) {
            copy_entry = Cost_Entry(0., 0.); //(0, 0) entry is invalid so set zero to terminate recursion
        } else {
            copy_entry = get_nearby_cost_entry(copy_x, copy_y, segment_index, chan_index);
        }
    }

    return copy_entry;
}

/* returns cost entry with the smallest delay */
Cost_Entry Expansion_Cost_Entry::get_smallest_entry() {
    Cost_Entry smallest_entry;

    for (auto entry : this->cost_vector) {
        if (std::isnan(smallest_entry.delay) || entry.delay < smallest_entry.delay) {
            smallest_entry = entry;
        }
    }

    return smallest_entry;
}

/* returns a cost entry that represents the average of all the recorded entries */
Cost_Entry Expansion_Cost_Entry::get_average_entry() {
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
Cost_Entry Expansion_Cost_Entry::get_geomean_entry() {
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
Cost_Entry Expansion_Cost_Entry::get_median_entry() {
    /* find median by binning the delays of all entries and then chosing the bin
     * with the largest number of entries */

    int num_bins = 10;

    /* find entries with smallest and largest delays */
    Cost_Entry min_del_entry;
    Cost_Entry max_del_entry;
    for (auto entry : this->cost_vector) {
        if (std::isnan(min_del_entry.delay) || entry.delay < min_del_entry.delay) {
            min_del_entry = entry;
        }
        if (std::isnan(max_del_entry.delay) || entry.delay > max_del_entry.delay) {
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

/* returns the absolute delta_x and delta_y offset required to reach to_node from from_node */
static void get_xy_deltas(const RRNodeId from_node, const RRNodeId to_node, int* delta_x, int* delta_y) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    e_rr_type from_type = rr_graph.node_type(from_node);
    e_rr_type to_type = rr_graph.node_type(to_node);

    if (!is_chan(from_type) && !is_chan(to_type)) {
        //Alternate formulation for non-channel types
        int from_x = 0;
        int from_y = 0;
        adjust_rr_position(from_node, from_x, from_y);

        int to_x = 0;
        int to_y = 0;
        adjust_rr_position(to_node, to_x, to_y);

        *delta_x = to_x - from_x;
        *delta_y = to_y - from_y;
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
        e_direction from_dir = rr_graph.node_direction(from_node);
        if (is_chan(from_type)
            && ((to_seg < from_seg_low && from_dir == INC_DIRECTION) || (to_seg > from_seg_high && from_dir == DEC_DIRECTION))) {
            delta_seg++;
        }

        if (from_type == CHANY) {
            *delta_x = delta_chan;
            *delta_y = delta_seg;
        } else {
            *delta_x = delta_seg;
            *delta_y = delta_chan;
        }
    }

    VTR_ASSERT_SAFE(std::abs(*delta_x) < (int)device_ctx.grid.width());
    VTR_ASSERT_SAFE(std::abs(*delta_y) < (int)device_ctx.grid.height());
}

static void adjust_rr_position(const RRNodeId rr, int& x, int& y) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    e_rr_type rr_type = rr_graph.node_type(rr);

    if (is_chan(rr_type)) {
        adjust_rr_wire_position(rr, x, y);
    } else if (is_pin(rr_type)) {
        adjust_rr_pin_position(rr, x, y);
    } else {
        VTR_ASSERT_SAFE(is_src_sink(rr_type));
        adjust_rr_src_sink_position(rr, x, y);
    }
}

static void adjust_rr_pin_position(const RRNodeId rr, int& x, int& y) {
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
    auto& rr_graph = device_ctx.rr_nodes;

    VTR_ASSERT_SAFE(is_pin(rr_graph.node_type(rr)));
    VTR_ASSERT_SAFE(rr_graph.node_xlow(rr) == rr_graph.node_xhigh(rr));
    VTR_ASSERT_SAFE(rr_graph.node_ylow(rr) == rr_graph.node_yhigh(rr));

    x = rr_graph.node_xlow(rr);
    y = rr_graph.node_ylow(rr);

    e_side rr_side = rr_graph.node_side(rr);

    if (rr_side == LEFT) {
        x -= 1;
        x = std::max(x, 0);
    } else if (rr_side == BOTTOM && y > 0) {
        y -= 1;
        y = std::max(y, 0);
    }
}

static void adjust_rr_wire_position(const RRNodeId rr, int& x, int& y) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    VTR_ASSERT_SAFE(is_chan(rr_graph.node_type(rr)));

    e_direction rr_dir = rr_graph.node_direction(rr);

    if (rr_dir == DEC_DIRECTION) {
        x = rr_graph.node_xhigh(rr);
        y = rr_graph.node_yhigh(rr);
    } else if (rr_dir == INC_DIRECTION) {
        x = rr_graph.node_xlow(rr);
        y = rr_graph.node_ylow(rr);
    } else {
        VTR_ASSERT_SAFE(rr_dir == BI_DIRECTION);
        //Not sure what to do here...
        //Try average for now.
        x = vtr::nint((rr_graph.node_xlow(rr) + rr_graph.node_xhigh(rr)) / 2.);
        y = vtr::nint((rr_graph.node_ylow(rr) + rr_graph.node_yhigh(rr)) / 2.);
    }
}

static void adjust_rr_src_sink_position(const RRNodeId rr, int& x, int& y) {
    //SOURCE/SINK nodes assume the full dimensions of their
    //associated block
    //
    //Use the average position.
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    VTR_ASSERT_SAFE(is_src_sink(rr_graph.node_type(rr)));

    x = vtr::nint((rr_graph.node_xlow(rr) + rr_graph.node_xhigh(rr)) / 2.);
    y = vtr::nint((rr_graph.node_ylow(rr) + rr_graph.node_yhigh(rr)) / 2.);
}

static void print_wire_cost_map(const std::vector<t_segment_inf>& segment_inf) {
    auto& device_ctx = g_vpr_ctx.device();

    for (size_t chan_index = 0; chan_index < f_wire_cost_map.dim_size(0); chan_index++) {
        for (size_t iseg = 0; iseg < f_wire_cost_map.dim_size(1); iseg++) {
            vtr::printf("Seg %d (%s, length %d) %d\n",
                        iseg,
                        segment_inf[iseg].name.c_str(),
                        segment_inf[iseg].length,
                        chan_index);
            for (size_t iy = 0; iy < device_ctx.grid.height(); iy++) {
                for (size_t ix = 0; ix < device_ctx.grid.width(); ix++) {
                    vtr::printf("%2d,%2d: %10.3g\t", ix, iy, f_wire_cost_map[chan_index][iseg][ix][iy].delay);
                }
                vtr::printf("\n");
            }
            vtr::printf("\n\n");
        }
    }
}

static void print_router_cost_map(const t_routing_cost_map& router_cost_map) {
    VTR_LOG("Djikstra Flood Costs:\n");
    for (size_t x = 0; x < router_cost_map.dim_size(0); x++) {
        for (size_t y = 0; y < router_cost_map.dim_size(1); y++) {
            VTR_LOG("(%zu,%zu):\n", x, y);

            for (size_t i = 0; i < router_cost_map[x][y].cost_vector.size(); ++i) {
                Cost_Entry entry = router_cost_map[x][y].cost_vector[i];
                VTR_LOG("  %d: delay=%10.3g cong=%10.3g\n", i, entry.delay, entry.congestion);
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
    VPR_THROW(VPR_ERROR_PLACE, "MapLookahead::read " DISABLE_ERROR);
}

void DeltaDelayModel::write(const std::string& /*file*/) const {
    VPR_THROW(VPR_ERROR_PLACE, "MapLookahead::write " DISABLE_ERROR);
}

#else /* VTR_ENABLE_CAPNPROTO */

static void ToCostEntry(Cost_Entry* out, const VprMapCostEntry::Reader& in) {
    out->delay = in.getDelay();
    out->congestion = in.getCongestion();
}

static void FromCostEntry(VprMapCostEntry::Builder* out, const Cost_Entry& in) {
    out->setDelay(in.delay);
    out->setCongestion(in.congestion);
}

void read_router_lookahead(const std::string& file) {
    vtr::ScopedStartFinishTimer timer("Loading router wire lookahead map");
    MmapFile f(file);

    /* Increase reader limit to 1G words to allow for large files. */
    ::capnp::ReaderOptions opts = default_large_capnp_opts();
    ::capnp::FlatArrayMessageReader reader(f.getData(), opts);

    auto map = reader.getRoot<VprMapLookahead>();

    ToNdMatrix<4, VprMapCostEntry, Cost_Entry>(&f_wire_cost_map, map.getCostMap(), ToCostEntry);
}

void write_router_lookahead(const std::string& file) {
    ::capnp::MallocMessageBuilder builder;

    auto map = builder.initRoot<VprMapLookahead>();

    auto cost_map = map.initCostMap();
    FromNdMatrix<4, VprMapCostEntry, Cost_Entry>(&cost_map, f_wire_cost_map, FromCostEntry);

    writeMessageToFile(file, &builder);
}

#endif
