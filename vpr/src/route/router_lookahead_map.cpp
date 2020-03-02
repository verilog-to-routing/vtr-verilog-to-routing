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
 * which are reachable from each physical tile type's SOURCEs/OPINs (f_src_opin_reachable_wires). This is used for
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

/* the cost map is computed by running a Dijkstra search from channel segment rr nodes at the specified reference coordinate */
#define REF_X 3
#define REF_Y 3

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
  private:
    std::vector<Cost_Entry> cost_vector;

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
        Cost_Entry entry;

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
    int rr_node_ind; //index in device_ctx.rr_nodes that this entry represents
    float cost;      //the cost of the path to get to this node

    /* store backward delay, R and congestion info */
    float delay;
    float R_upstream;
    float congestion_upstream;

    PQ_Entry(int set_rr_node_ind, int /*switch_ind*/, float parent_delay, float parent_R_upstream, float parent_congestion_upstream, bool starting_node) {
        this->rr_node_ind = set_rr_node_ind;

        auto& device_ctx = g_vpr_ctx.device();
        this->delay = parent_delay;
        this->congestion_upstream = parent_congestion_upstream;
        this->R_upstream = parent_R_upstream;
        if (!starting_node) {
            int cost_index = device_ctx.rr_nodes[set_rr_node_ind].cost_index();
            //this->delay += g_rr_nodes[set_rr_node_ind].C() * (g_rr_switch_inf[switch_ind].R + 0.5*g_rr_nodes[set_rr_node_ind].R()) +
            //              g_rr_switch_inf[switch_ind].Tdel;

            //FIXME going to use the delay data that the VPR7 lookahead uses. For some reason the delay calculation above calculates
            //    a value that's just a little smaller compared to what VPR7 lookahead gets. While the above calculation should be more accurate,
            //    I have found that it produces the same CPD results but with worse runtime.
            //
            //    TODO: figure out whether anything's wrong with the calculation above and use that instead. If not, add the other
            //          terms like T_quadratic and R_upstream to the calculation below (they are == 0 or UNDEFINED for buffered archs I think)
            VTR_ASSERT(device_ctx.rr_indexed_data[cost_index].T_quadratic <= 0.);
            VTR_ASSERT(device_ctx.rr_indexed_data[cost_index].C_load <= 0.);
            this->delay += device_ctx.rr_indexed_data[cost_index].T_linear;

            this->congestion_upstream += device_ctx.rr_indexed_data[cost_index].base_cost;
        }

        /* set the cost of this node */
        this->cost = this->delay;
    }

    bool operator<(const PQ_Entry& obj) const {
        /* inserted into max priority queue so want queue entries with a lower cost to be greater */
        return (this->cost > obj.cost);
    }
};

struct t_reachable_wire_inf {
    e_rr_type wire_rr_type;
    int wire_seg_index;

    //Costs to reach the wire type from the current node
    float congestion;
    float delay;
};

/* used during Dijkstra expansion to store delay/congestion info lists for each relative coordinate for a given segment and channel type.
 * the list at each coordinate is later boiled down to a single representative cost entry to be stored in the final cost map */
typedef vtr::Matrix<Expansion_Cost_Entry> t_routing_cost_map; //[0..device_ctx.grid.width()-1][0..device_ctx.grid.height()-1]

typedef std::vector<std::vector<std::map<int, t_reachable_wire_inf>>> t_src_opin_reachable_wires; //[0..device_ctx.physical_tile_types.size()-1][0..max_ptc-1][wire_seg_index]

struct t_dijkstra_data {
    /* a list of boolean flags (one for each rr node) to figure out if a certain node has already been expanded */
    std::vector<bool> node_expanded;
    /* for each node keep a list of the cost with which that node has been visited (used to determine whether to push
     * a candidate node onto the expansion queue */
    std::vector<float> node_visited_costs;
    /* a priority queue for expansion */
    std::priority_queue<PQ_Entry> pq;
};

/******** File-Scope Variables ********/

constexpr int DIRECT_CONNECT_SPECIAL_SEG_TYPE = -1;

//Look-up table from CHANX/CHANY (to SINKs) for various distances
t_wire_cost_map f_wire_cost_map;

//Look-up table from SOURCE/OPIN to CHANX/CHANY of various types
t_src_opin_reachable_wires f_src_opin_reachable_wires;

/******** File-Scope Functions ********/
Cost_Entry get_wire_cost_entry(e_rr_type rr_type, int seg_index, int delta_x, int delta_y);
static void compute_router_wire_lookahead(int num_segments);
static void compute_router_src_opin_lookahead();
static vtr::Point<int> pick_sample_tile(t_physical_tile_type_ptr tile_type);

static void alloc_cost_map(int num_segments);
static void free_cost_map();
/* returns index of a node from which to start routing */
static int get_start_node_ind(int start_x, int start_y, int target_x, int target_y, t_rr_type rr_type, int seg_index, int track_offset);
/* runs Dijkstra's algorithm from specified node until all nodes have been visited. Each time a pin is visited, the delay/congestion information
 * to that pin is stored is added to an entry in the routing_cost_map */
static void run_dijkstra(int start_node_ind, int start_x, int start_y, t_routing_cost_map& routing_cost_map, t_dijkstra_data* data);
/* iterates over the children of the specified node and selectively pushes them onto the priority queue */
static void expand_dijkstra_neighbours(PQ_Entry parent_entry, std::vector<float>& node_visited_costs, std::vector<bool>& node_expanded, std::priority_queue<PQ_Entry>& pq);
/* sets the lookahead cost map entries based on representative cost entries from routing_cost_map */
static void set_lookahead_map_costs(int segment_index, e_rr_type chan_type, t_routing_cost_map& routing_cost_map);
/* fills in missing lookahead map entries by copying the cost of the closest valid entry */
static void fill_in_missing_lookahead_entries(int segment_index, e_rr_type chan_type);
/* returns a cost entry in the f_wire_cost_map that is near the specified coordinates (and preferably towards (0,0)) */
static Cost_Entry get_nearby_cost_entry(int x, int y, int segment_index, int chan_index);
/* returns the absolute delta_x and delta_y offset required to reach to_node from from_node */
static void get_xy_deltas(int from_node_ind, int to_node_ind, int* delta_x, int* delta_y);

static void print_cost_map();

/******** Interface class member function definitions ********/
float MapLookahead::get_expected_cost(int current_node, int target_node, const t_conn_cost_params& params, float /*R_upstream*/) const {
    auto& device_ctx = g_vpr_ctx.device();

    t_rr_type rr_type = device_ctx.rr_nodes[current_node].type();

    if (rr_type == CHANX || rr_type == CHANY || rr_type == SOURCE || rr_type == OPIN) {
        return get_lookahead_map_cost(current_node, target_node, params.criticality);
    } else if (rr_type == IPIN) { /* Change if you're allowing route-throughs */
        return (device_ctx.rr_indexed_data[SINK_COST_INDEX].base_cost);
    } else { /* Change this if you want to investigate route-throughs */
        return (0.);
    }
}

void MapLookahead::compute(const std::vector<t_segment_inf>& segment_inf) {
    compute_router_lookahead(segment_inf.size());
}

void MapLookahead::read(const std::string& file) {
    read_router_lookahead(file);
}

void MapLookahead::write(const std::string& file) const {
    write_router_lookahead(file);
}

/******** Function Definitions ********/
/* queries the lookahead_map (should have been computed prior to routing) to get the expected cost
 * from the specified source to the specified target */
float get_lookahead_map_cost(int from_node_ind, int to_node_ind, float criticality_fac) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& from_node = device_ctx.rr_nodes[from_node_ind];

    int delta_x, delta_y;
    get_xy_deltas(from_node_ind, to_node_ind, &delta_x, &delta_y);
    delta_x = abs(delta_x);
    delta_y = abs(delta_y);

    float expected_cost = std::numeric_limits<float>::infinity();

    e_rr_type from_type = from_node.type();
    if (from_type == SOURCE || from_type == OPIN) {
        //When estimating costs from a SOURCE/OPIN we look-up to find which wire types (and the
        //cost to reach them) in f_src_opin_reachable_wires. Once we know what wire types are
        //reachable, we query the f_wire_cost_map (i.e. the wire lookahead) to get the final
        //delay to reach the sink.

        t_physical_tile_type_ptr tile_type = device_ctx.grid[from_node.xlow()][from_node.ylow()].type;
        auto tile_index = std::distance(&device_ctx.physical_tile_types[0], tile_type);

        //From the current SOURCE/OPIN we look-up the wiretypes which are reachable
        //and then add the estimates from those wire types for the distance of interest.
        //If there are multiple options we use the minimum value.
        for (const auto& kv : f_src_opin_reachable_wires[tile_index][from_node.ptc_num()]) {
            const t_reachable_wire_inf& reachable_wire_inf = kv.second;

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

            float this_cost = (criticality_fac) * (reachable_wire_inf.congestion + wire_cost_entry.congestion)
                              + (1. - criticality_fac) * (reachable_wire_inf.delay + wire_cost_entry.delay);

            expected_cost = std::min(expected_cost, this_cost);
        }

        VTR_ASSERT_SAFE_MSG(std::isfinite(expected_cost),
                            vtr::string_fmt("Lookahead failed to estimate cost from %s: %s",
                                            rr_node_arch_name(from_node_ind).c_str(),
                                            describe_rr_node(from_node_ind).c_str())
                                .c_str());

    } else {
        VTR_ASSERT_SAFE(from_type == CHANX || from_type == CHANY);
        //When estimating costs from a wire, we directly look-up the result in the wire lookahead (f_wire_cost_map)

        int from_cost_index = from_node.cost_index();
        int from_seg_index = device_ctx.rr_indexed_data[from_cost_index].seg_index;

        VTR_ASSERT(from_seg_index >= 0);

        /* now get the expected cost from our lookahead map */
        Cost_Entry cost_entry = get_wire_cost_entry(from_type, from_seg_index, delta_x, delta_y);

        float expected_delay = cost_entry.delay;
        float expected_congestion = cost_entry.congestion;

        expected_cost = criticality_fac * expected_delay + (1.0 - criticality_fac) * expected_congestion;
    }

    return expected_cost;
}

Cost_Entry get_wire_cost_entry(e_rr_type rr_type, int seg_index, int delta_x, int delta_y) {
    VTR_ASSERT_SAFE(rr_type == CHANX || rr_type == CHANY);

    int chan_index = 0;
    if (rr_type == CHANY) {
        chan_index = 1;
    }

    return f_wire_cost_map[chan_index][seg_index][delta_x][delta_y];
}

/* Computes the lookahead map to be used by the router. If a map was computed prior to this, a new one will not be computed again.
 * The rr graph must have been built before calling this function. */
void compute_router_lookahead(int num_segments) {
    vtr::ScopedStartFinishTimer timer("Computing router lookahead map");

    //First compute the delay map when starting from the various wire types
    //(CHANX/CHANY)in the routing architecture
    compute_router_wire_lookahead(num_segments);

    //Next, compute which wire types are accessible (and the cost to reach them)
    //from the different physical tile type's SOURCEs & OPINs
    compute_router_src_opin_lookahead();
}

static void compute_router_wire_lookahead(int num_segments) {
    vtr::ScopedStartFinishTimer timer("Computing wire lookahead");

    f_wire_cost_map.clear();

    auto& device_ctx = g_vpr_ctx.device();

    /* free previous delay map and allocate new one */
    free_cost_map();
    alloc_cost_map(num_segments);

    /* run Dijkstra's algorithm for each segment type & channel type combination */

    /* allocate the cost map for use with each iseg/chan_type */
    t_routing_cost_map routing_cost_map({device_ctx.grid.width(), device_ctx.grid.height()});
    t_dijkstra_data data;
    for (int iseg = 0; iseg < num_segments; iseg++) {
        for (e_rr_type chan_type : {CHANX, CHANY}) {
            /* reset cost most for this segment */
            routing_cost_map.fill(Expansion_Cost_Entry());

            for (int ref_inc = 0; ref_inc < 3; ref_inc++) {
                for (int track_offset = 0; track_offset < MAX_TRACK_OFFSET; track_offset += 2) {
                    /* get the rr node index from which to start routing */
                    int start_node_ind = get_start_node_ind(REF_X + ref_inc, REF_Y + ref_inc,
                                                            device_ctx.grid.width() - 2, device_ctx.grid.height() - 2, //non-corner upper right
                                                            chan_type, iseg, track_offset);

                    if (start_node_ind == UNDEFINED) {
                        continue;
                    }

                    /* run Dijkstra's algorithm */
                    run_dijkstra(start_node_ind, REF_X + ref_inc, REF_Y + ref_inc, routing_cost_map, &data);
                }
            }

            /* boil down the cost list in routing_cost_map at each coordinate to a representative cost entry and store it in the lookahead
             * cost map */
            set_lookahead_map_costs(iseg, chan_type, routing_cost_map);

            /* fill in missing entries in the lookahead cost map by copying the closest cost entries (cost map was computed based on
             * a reference coordinate > (0,0) so some entries that represent a cross-chip distance have not been computed) */
            fill_in_missing_lookahead_entries(iseg, chan_type);
        }
    }

    if (false) print_cost_map();
}

static void compute_router_src_opin_lookahead() {
    vtr::ScopedStartFinishTimer timer("Computing src/opin lookahead");
    auto& device_ctx = g_vpr_ctx.device();

    f_src_opin_reachable_wires.clear();

    f_src_opin_reachable_wires.resize(device_ctx.physical_tile_types.size());

    struct t_pq_entry {
        float delay;
        float congestion;
        int inode;

        bool operator<(const t_pq_entry& rhs) const {
            return this->delay < rhs.delay;
        }
    };

    std::priority_queue<t_pq_entry> pq;
    std::vector<int> rr_node_indices;

    //We assume that the routing connectivity of each instance of a physical tile is the same,
    //and so only measure one instance of each type
    for (size_t itile = 0; itile < device_ctx.physical_tile_types.size(); ++itile) {
        vtr::Point<int> sample_loc = pick_sample_tile(&device_ctx.physical_tile_types[itile]);

        if (sample_loc.x() < 0 || sample_loc.y() < 0) {
            VTR_LOG_WARN("Failed to find sample location for tile type '%s'\n", device_ctx.physical_tile_types[itile].name);
            continue;
        }

        for (e_rr_type rr_type : {SOURCE, OPIN}) {
            get_rr_node_indices(device_ctx.rr_node_indices, sample_loc.x(), sample_loc.y(), rr_type, &rr_node_indices);
            for (int inode : rr_node_indices) {
                if (inode < 0) continue;

                int ptc = device_ctx.rr_nodes[inode].ptc_num();

                if (ptc >= int(f_src_opin_reachable_wires[itile].size())) {
                    //Inefficient work-around
                    f_src_opin_reachable_wires[itile].resize(ptc + 1);
                }

                t_pq_entry root;
                root.congestion = 0.;
                root.delay = 0.;
                root.inode = inode;

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

                    e_rr_type curr_rr_type = device_ctx.rr_nodes[curr.inode].type();
                    if (curr_rr_type == CHANX || curr_rr_type == CHANY || curr_rr_type == SINK) {
                        //We stop expansion at any CHANX/CHANY/SINK
                        int seg_index;
                        if (curr_rr_type != SINK) {
                            //It's a wire, figure out its type
                            int cost_index = device_ctx.rr_nodes[curr.inode].cost_index();
                            seg_index = device_ctx.rr_indexed_data[cost_index].seg_index;
                        } else {
                            //This is a direct-connect path between an IPIN and OPIN,
                            //which terminated at a SINK.
                            //
                            //We treat this as a 'special' wire type
                            seg_index = DIRECT_CONNECT_SPECIAL_SEG_TYPE;
                        }

                        //Keep costs of the best path to reach each wire type
                        if (!f_src_opin_reachable_wires[itile][ptc].count(seg_index)
                            || curr.delay < f_src_opin_reachable_wires[itile][ptc][seg_index].delay) {
                            f_src_opin_reachable_wires[itile][ptc][seg_index].wire_rr_type = curr_rr_type;
                            f_src_opin_reachable_wires[itile][ptc][seg_index].wire_seg_index = seg_index;
                            f_src_opin_reachable_wires[itile][ptc][seg_index].delay = curr.delay;
                            f_src_opin_reachable_wires[itile][ptc][seg_index].congestion = curr.congestion;
                        }

                    } else if (curr_rr_type == SOURCE || curr_rr_type == OPIN || curr_rr_type == IPIN) {
                        //We allow expansion through SOURCE/OPIN/IPIN types
                        int cost_index = device_ctx.rr_nodes[curr.inode].cost_index();
                        float incr_cong = device_ctx.rr_indexed_data[cost_index].base_cost; //Current nodes congestion cost

                        for (int iedge : device_ctx.rr_nodes[curr.inode].edges()) {
                            int iswitch = device_ctx.rr_nodes[curr.inode].edge_switch(iedge);
                            float incr_delay = device_ctx.rr_switch_inf[iswitch].Tdel;

                            int inext = device_ctx.rr_nodes[curr.inode].edge_sink_node(iedge);

                            t_pq_entry next;
                            next.congestion = curr.congestion + incr_cong; //Of current node
                            next.delay = curr.delay + incr_delay;          //To reach next node
                            next.inode = inext;

                            pq.push(next);
                        }
                    } else {
                        VPR_ERROR(VPR_ERROR_ROUTE, "Unrecognized RR type");
                    }
                }

                VTR_LOGV_WARN(f_src_opin_reachable_wires[itile][ptc].empty(),
                              "Found no reachable wires from %s\n", rr_node_arch_name(inode).c_str());
            }
        }
    }
}

static vtr::Point<int> pick_sample_tile(t_physical_tile_type_ptr tile_type) {
    //Very simple for now, just pick the fist matching tile found
    vtr::Point<int> loc(OPEN, OPEN);

    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;
    for (size_t x = 0; x < grid.width(); ++x) {
        for (size_t y = 0; y < grid.height(); ++y) {
            if (grid[x][y].type == tile_type) {
                loc.set_x(x);
                loc.set_y(y);
            }
        }
    }

    return loc;
}

/* returns index of a node from which to start routing */
static int get_start_node_ind(int start_x, int start_y, int target_x, int target_y, t_rr_type rr_type, int seg_index, int track_offset) {
    int result = UNDEFINED;

    if (rr_type != CHANX && rr_type != CHANY) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Must start lookahead routing from CHANX or CHANY node\n");
    }

    /* determine which direction the wire should go in based on the start & target coordinates */
    e_direction direction = INC_DIRECTION;
    if ((rr_type == CHANX && target_x < start_x) || (rr_type == CHANY && target_y < start_y)) {
        direction = DEC_DIRECTION;
    }

    auto& device_ctx = g_vpr_ctx.device();

    VTR_ASSERT(rr_type == CHANX || rr_type == CHANY);

    const std::vector<int>& channel_node_list = device_ctx.rr_node_indices[rr_type][start_x][start_y][0];

    /* find first node in channel that has specified segment index and goes in the desired direction */
    for (unsigned itrack = 0; itrack < channel_node_list.size(); itrack++) {
        int node_ind = channel_node_list[itrack];

        e_direction node_direction = device_ctx.rr_nodes[node_ind].direction();
        int node_cost_ind = device_ctx.rr_nodes[node_ind].cost_index();
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

    return result;
}

/* allocates space for cost map entries */
static void alloc_cost_map(int num_segments) {
    auto& device_ctx = g_vpr_ctx.device();

    f_wire_cost_map = t_wire_cost_map({2, size_t(num_segments), device_ctx.grid.width(), device_ctx.grid.height()});
}

/* frees the cost map. duh. */
static void free_cost_map() {
    f_wire_cost_map.clear();
}

/* runs Dijkstra's algorithm from specified node until all nodes have been visited. Each time a pin is visited, the delay/congestion information
 * to that pin is stored is added to an entry in the routing_cost_map */
static void run_dijkstra(int start_node_ind, int start_x, int start_y, t_routing_cost_map& routing_cost_map, t_dijkstra_data* data) {
    auto& device_ctx = g_vpr_ctx.device();

    std::vector<bool>& node_expanded = data->node_expanded;
    node_expanded.resize(device_ctx.rr_nodes.size());
    std::fill(node_expanded.begin(), node_expanded.end(), false);

    std::vector<float>& node_visited_costs = data->node_visited_costs;
    node_visited_costs.resize(device_ctx.rr_nodes.size());
    std::fill(node_visited_costs.begin(), node_visited_costs.end(), -1.0);

    /* a priority queue for expansion */
    std::priority_queue<PQ_Entry>& pq = data->pq;
    while (!pq.empty()) {
        pq.pop();
    }

    /* first entry has no upstream delay or congestion */
    PQ_Entry first_entry(start_node_ind, UNDEFINED, 0, 0, 0, true);

    pq.push(first_entry);

    /* now do routing */
    while (!pq.empty()) {
        PQ_Entry current = pq.top();
        pq.pop();

        int node_ind = current.rr_node_ind;

        /* check that we haven't already expanded from this node */
        if (node_expanded[node_ind]) {
            continue;
        }

        /* if this node is an ipin record its congestion/delay in the routing_cost_map */
        if (device_ctx.rr_nodes[node_ind].type() == IPIN) {
            int ipin_x = device_ctx.rr_nodes[node_ind].xlow();
            int ipin_y = device_ctx.rr_nodes[node_ind].ylow();

            if (ipin_x >= start_x && ipin_y >= start_y) {
                int delta_x, delta_y;
                get_xy_deltas(start_node_ind, node_ind, &delta_x, &delta_y);

                routing_cost_map[delta_x][delta_y].add_cost_entry(current.delay, current.congestion_upstream);
            }
        }

        expand_dijkstra_neighbours(current, node_visited_costs, node_expanded, pq);
        node_expanded[node_ind] = true;
    }
}

/* iterates over the children of the specified node and selectively pushes them onto the priority queue */
static void expand_dijkstra_neighbours(PQ_Entry parent_entry, std::vector<float>& node_visited_costs, std::vector<bool>& node_expanded, std::priority_queue<PQ_Entry>& pq) {
    auto& device_ctx = g_vpr_ctx.device();

    int parent_ind = parent_entry.rr_node_ind;

    auto& parent_node = device_ctx.rr_nodes[parent_ind];

    for (t_edge_size iedge = 0; iedge < parent_node.num_edges(); iedge++) {
        int child_node_ind = parent_node.edge_sink_node(iedge);
        int switch_ind = parent_node.edge_switch(iedge);

        /* skip this child if it has already been expanded from */
        if (node_expanded[child_node_ind]) {
            continue;
        }

        PQ_Entry child_entry(child_node_ind, switch_ind, parent_entry.delay,
                             parent_entry.R_upstream, parent_entry.congestion_upstream, false);

        //VTR_ASSERT(child_entry.cost >= 0); //Asertion fails in practise. TODO: debug

        /* skip this child if it has been visited with smaller cost */
        if (node_visited_costs[child_node_ind] >= 0 && node_visited_costs[child_node_ind] < child_entry.cost) {
            continue;
        }

        /* finally, record the cost with which the child was visited and put the child entry on the queue */
        node_visited_costs[child_node_ind] = child_entry.cost;
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

            if (cost_entry.delay < 0 && cost_entry.congestion < 0) {
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

    //VTR_ASSERT(copy_x > 0 || copy_y > 0); //Asertion fails in practise. TODO: debug

    Cost_Entry copy_entry = f_wire_cost_map[chan_index][segment_index][copy_x][copy_y];

    /* if the entry to be copied is also empty, recurse */
    if (copy_entry.delay < 0 && copy_entry.congestion < 0) {
        copy_entry = get_nearby_cost_entry(copy_x, copy_y, segment_index, chan_index);
    }

    return copy_entry;
}

/* returns cost entry with the smallest delay */
Cost_Entry Expansion_Cost_Entry::get_smallest_entry() {
    Cost_Entry smallest_entry;

    for (auto entry : this->cost_vector) {
        if (smallest_entry.delay < 0 || entry.delay < smallest_entry.delay) {
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
        if (min_del_entry.delay < 0 || entry.delay < min_del_entry.delay) {
            min_del_entry = entry;
        }
        if (max_del_entry.delay < 0 || entry.delay > max_del_entry.delay) {
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
static void get_xy_deltas(int from_node_ind, int to_node_ind, int* delta_x, int* delta_y) {
    auto& device_ctx = g_vpr_ctx.device();

    auto& from = device_ctx.rr_nodes[from_node_ind];
    auto& to = device_ctx.rr_nodes[to_node_ind];

    /* get chan/seg coordinates of the from/to nodes. seg coordinate is along the wire,
     * chan coordinate is orthogonal to the wire */
    int from_seg_low = from.xlow();
    int from_seg_high = from.xhigh();
    int from_chan = from.ylow();
    int to_seg = to.xlow();
    int to_chan = to.ylow();
    if (from.type() == CHANY) {
        from_seg_low = from.ylow();
        from_seg_high = from.yhigh();
        from_chan = from.xlow();
        to_seg = to.ylow();
        to_chan = to.xlow();
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
    if ((from.type() == CHANX || from.type() == CHANY)
        && ((to_seg < from_seg_low && from.direction() == INC_DIRECTION) || (to_seg > from_seg_high && from.direction() == DEC_DIRECTION))) {
        delta_seg++;
    }

    *delta_x = delta_seg;
    *delta_y = delta_chan;
    if (from.type() == CHANY) {
        *delta_x = delta_chan;
        *delta_y = delta_seg;
    }
}

static void print_cost_map() {
    auto& device_ctx = g_vpr_ctx.device();

    for (size_t chan_index = 0; chan_index < f_wire_cost_map.dim_size(0); chan_index++) {
        for (size_t iseg = 0; iseg < f_wire_cost_map.dim_size(1); iseg++) {
            vtr::printf("Seg %d CHAN %d\n", iseg, chan_index);
            for (size_t iy = 0; iy < device_ctx.grid.height(); iy++) {
                for (size_t ix = 0; ix < device_ctx.grid.width(); ix++) {
                    vtr::printf("%d,%d: %.3e\t", ix, iy, f_wire_cost_map[chan_index][iseg][ix][iy].delay);
                }
                vtr::printf("\n");
            }
            vtr::printf("\n\n");
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
