#ifndef ROUTER_LOOKAHEAD_MAP_UTILS_H_
#define ROUTER_LOOKAHEAD_MAP_UTILS_H_
/*
 * The router lookahead provides an estimate of the cost from an intermediate node to the target node
 * during directed (A*-like) routing.
 *
 * The VPR 7.0 classic lookahead (route/route_timing.c ==> get_timing_driven_expected_cost) lower-bounds the remaining delay and
 * congestion by assuming that a minimum number of wires, of the same type as the current node being expanded, can be used
 * to complete the route. While this method is efficient, it can run into trouble with architectures that use
 * multiple interconnected wire types.
 *
 * The lookahead in this file pre-computes delay/congestion costs up and to the right of a starting tile. This generates
 * delay/congestion tables for {CHANX, CHANY} channel types, over all wire types defined in the architecture file.
 * See Section 3.2.4 in Oleg Petelin's MASc thesis (2016) for more discussion.
 *
 */

#include <cmath>
#include <limits>
#include <vector>
#include <queue>
#include <unordered_map>
#include "vpr_types.h"
#include "vtr_geometry.h"
#include "rr_node.h"
#include "rr_graph_view.h"

namespace util {

class Cost_Entry;

class Expansion_Cost_Entry;
/* used during Dijkstra expansion to store delay/congestion info lists for each relative coordinate for a given segment and channel type.
 * the list at each coordinate is later boiled down to a single representative cost entry to be stored in the final cost map */
typedef vtr::NdMatrix<Expansion_Cost_Entry, 3> t_routing_cost_map; //[0..num_layers][0..device_ctx.grid.width()-1][0..device_ctx.grid.height()-1]

typedef Cost_Entry (*WireCostCallBackFunction)(e_rr_type, int, int, int, int, int);

/* a class that represents an entry in the Dijkstra expansion priority queue */
class PQ_Entry {
  public:
    RRNodeId rr_node; //index in device_ctx.rr_nodes that this entry represents
    float cost;       //the cost of the path to get to this node

    /* store backward delay, R and congestion info */
    float delay;
    float R_upstream;
    float congestion_upstream;

    PQ_Entry(RRNodeId set_rr_node, int /*switch_ind*/, float parent_delay, float parent_R_upstream, float parent_congestion_upstream, bool starting_node);

    bool operator<(const PQ_Entry& obj) const {
        /* inserted into max priority queue so want queue entries with a lower cost to be greater */
        return (this->cost > obj.cost);
    }
};

struct t_dijkstra_data {
    /* a list of boolean flags (one for each rr node) to figure out if a certain node has already been expanded */
    vtr::vector<RRNodeId, bool> node_expanded;
    /* for each node keep a list of the cost with which that node has been visited (used to determine whether to push
     * a candidate node onto the expansion queue */
    vtr::vector<RRNodeId, float> node_visited_costs;
    /* a priority queue for expansion */
    std::priority_queue<PQ_Entry> pq;
};

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

/**
 * @brief f_cost_map is an array of these cost entries that specifies delay/congestion estimates to travel relative x/y distances
 */
class Cost_Entry {
  public:
    float delay;      ///<Delay value of the cost entry
    float congestion; ///<Base cost value of the cost entry
    bool fill;        ///<Boolean specifying whether this Entry was created as a result of the cost map
                      ///<holes filling procedure

    Cost_Entry()
        : delay(std::numeric_limits<float>::quiet_NaN())
        , congestion(std::numeric_limits<float>::quiet_NaN())
        , fill(false) {}
    Cost_Entry(float set_delay, float set_congestion)
        : delay(set_delay)
        , congestion(set_congestion)
        , fill(false) {}
    Cost_Entry(float set_delay, float set_congestion, bool set_fill)
        : delay(set_delay)
        , congestion(set_congestion)
        , fill(set_fill) {}
    bool valid() const {
        return !(std::isnan(delay) || std::isnan(congestion));
    }

    bool operator==(const Cost_Entry& other) const {
        return delay == other.delay && congestion == other.congestion;
    }
};

/**
 * @brief a class that stores delay/congestion information for a given relative coordinate during the Dijkstra expansion.
 *
 * Since it stores multiple cost entries, it is later boiled down to a single representative cost entry to be stored
 * in the final lookahead cost map
 *
 * This class is used for the lookahead map implementation only
 */
class Expansion_Cost_Entry {
  private:
    std::vector<Cost_Entry> cost_vector; ///<This vector is used to store different cost entries that are treated
                                         ///<differently based on the below representative cost entry method computations
                                         ///<Currently, only the smallest entry is stored in the vector, but this prepares
                                         ///<the ground for other possible representative cost entry calculation methods.
                                         ///<The vector is filled for a specific (chan_index, segment_index, delta_x, delta_y) cost entry.

    Cost_Entry get_smallest_entry() const;
    Cost_Entry get_average_entry() const;
    Cost_Entry get_geomean_entry() const;
    Cost_Entry get_median_entry() const;

  public:
    void add_cost_entry(e_representative_entry_method method,
                        float add_delay,
                        float add_congestion) {
        Cost_Entry cost_entry(add_delay, add_congestion);
        if (method == SMALLEST) {
            /* taking the smallest-delay entry anyway, so no need to push back multiple entries */
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

    Cost_Entry get_representative_cost_entry(e_representative_entry_method method) const {
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

// Keys in the RoutingCosts map
struct RoutingCostKey {
    // segment type index
    int seg_index;

    // offset of the destination connection box from the starting segment
    vtr::Point<int> delta;

    RoutingCostKey()
        : seg_index(-1)
        , delta(0, 0) {}
    RoutingCostKey(int seg_index_arg, vtr::Point<int> delta_arg)
        : seg_index(seg_index_arg)
        , delta(delta_arg) {}

    bool operator==(const RoutingCostKey& other) const {
        return seg_index == other.seg_index && delta == other.delta;
    }
};

// hash implementation for RoutingCostKey
struct HashRoutingCostKey {
    std::size_t operator()(RoutingCostKey const& key) const noexcept {
        std::size_t hash = std::hash<int>{}(key.seg_index);
        vtr::hash_combine(hash, key.delta.x());
        vtr::hash_combine(hash, key.delta.y());
        return hash;
    }
};

// Map used to store intermediate routing costs
typedef std::unordered_map<RoutingCostKey, float, HashRoutingCostKey> RoutingCosts;

// A version of PQ_Entry that only calculates and stores the delay.
class PQ_Entry_Delay {
  public:
    RRNodeId rr_node; //index in device_ctx.rr_nodes that this entry represents
    float delay_cost; //the cost of the path to get to this node

    PQ_Entry_Delay(RRNodeId set_rr_node, int /*switch_ind*/, const PQ_Entry_Delay* parent);

    float cost() const {
        return delay_cost;
    }

    void adjust_Tsw(float amount) {
        delay_cost += amount;
    }

    bool operator>(const PQ_Entry_Delay& obj) const {
        return (this->delay_cost > obj.delay_cost);
    }
};

// A version of PQ_Entry that only calculates and stores the base cost.
class PQ_Entry_Base_Cost {
  public:
    RRNodeId rr_node; //index in device_ctx.rr_nodes that this entry represents
    float base_cost;

    PQ_Entry_Base_Cost(RRNodeId set_rr_node, int /*switch_ind*/, const PQ_Entry_Base_Cost* parent);

    float cost() const {
        return base_cost;
    }

    void adjust_Tsw(float /* amount */) {
        // do nothing
    }

    bool operator>(const PQ_Entry_Base_Cost& obj) const {
        return (this->base_cost > obj.base_cost);
    }
};

struct Search_Path {
    float cost;
    size_t parent;
    int edge;
};

/* iterates over the children of the specified node and selectively pushes them onto the priority queue */
template<typename Entry>
void expand_dijkstra_neighbours(const RRGraphView& rr_graph,
                                const Entry& parent_entry,
                                std::vector<Search_Path>* paths,
                                std::vector<bool>* node_expanded,
                                std::priority_queue<Entry,
                                                    std::vector<Entry>,
                                                    std::greater<Entry>>* pq);

struct t_reachable_wire_inf {
    e_rr_type wire_rr_type;
    int wire_seg_index;
    int layer_number;

    //Costs to reach the wire type from the current node
    float congestion;
    float delay;
};

//[0..device_ctx.physical_tile_types.size()-1][0..max_ptc-1][wire_seg_index]
// ^                                           ^             ^
// |                                           |             |
// physical block type index                   |             Reachable wire info
//                                             |
//                                             SOURCE/OPIN ptc
//
// This data structure stores a set of delay and congestion values for each wire that can be reached from a given
// SOURCE/OPIN of a given tile type.
//
// When querying this data structure, the minimum cost is computed for each delay/congestion pair, and returned
// as the lookahead expected cost. [opin/src layer_num][tile_index][opin/src ptc_number][to_layer_num] -> pair<seg_index, t_reachable_wire_inf>
typedef std::vector<std::vector<std::vector<std::vector<std::map<int, t_reachable_wire_inf>>>>> t_src_opin_delays;

//[from pin ptc num][target src ptc num]->cost
typedef std::vector<std::unordered_map<int, Cost_Entry>> t_ipin_primitive_sink_delays;

//[0..device_ctx.physical_tile_types.size()-1][0..max_ptc-1]
// ^                                           ^
// |                                           |
// physical block type index                   |
//                                             |
//                                             SINK ptc
//
// This data structure stores the minimum delay to reach a specific SINK from the last connection between the wire (CHANX/CHANY)
// and the tile's IPIN. If there are many connections to the same IPIN, the one with the minimum delay is selected.
typedef std::vector<std::vector<std::vector<t_reachable_wire_inf>>> t_chan_ipins_delays;

typedef Cost_Entry (*WireCostFunc)(e_rr_type, int, int, int, int, int);

/**
 * @brief For each tile, iterate over its OPINs and store which segment types are accessible from each OPIN
 * @param is_flat
 * @return
 */
t_src_opin_delays compute_router_src_opin_lookahead(bool is_flat);

t_chan_ipins_delays compute_router_chan_ipin_lookahead();

t_ipin_primitive_sink_delays compute_intra_tile_dijkstra(const RRGraphView& rr_graph,
                                                         t_physical_tile_type_ptr physical_tile,
                                                         int layer,
                                                         int x,
                                                         int y);

/* returns index of a node from which to start routing */
RRNodeId get_start_node(int layer, int start_x, int start_y, int target_x, int target_y, t_rr_type rr_type, int seg_index, int track_offset);

/**
 * @brief Computes the absolute delta_x and delta_y offset
 * required to reach to_node from from_node
 * @param from_node The starting node
 * @param to_node The destination node
 * @return (delta_x, delta_y) offset required to reach to
 * to_node from from_node.
 */
std::pair<int, int> get_xy_deltas(RRNodeId from_node, RRNodeId to_node);

t_routing_cost_map get_routing_cost_map(int longest_seg_length,
                                        int from_layer_num,
                                        const e_rr_type& chan_type,
                                        const t_segment_inf& segment_inf,
                                        const std::unordered_map<int, std::unordered_set<int>>& sample_locs,
                                        bool sample_all_locs);

/**
 * @brief Iterate over all of the wire segments accessible from the SOURCE/OPIN (stored in src_opin_delay_map) and return the minimum cost (congestion and delay) across them to the sink
 * @param src_opin_delay_map
 * @param layer_num
 * @param delta_x
 * @param delta_y
 * @param to_layer_num
 * @param wire_cost_func call back function that would return a cost ot get to a given location from the given segment
 * @return (delay, congestion)
 */
std::pair<float, float> get_cost_from_src_opin(const std::map<int, util::t_reachable_wire_inf>& src_opin_delay_map,
                                               int delta_x,
                                               int delta_y,
                                               int to_layer_num,
                                               WireCostFunc wire_cost_func);

void dump_readable_router_lookahead_map(const std::string& file_name, const std::vector<int>& dim_sizes, WireCostCallBackFunction wire_cost_func);
} // namespace util

#endif
