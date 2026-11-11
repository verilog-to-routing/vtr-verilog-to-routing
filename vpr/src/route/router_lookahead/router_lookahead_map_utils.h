#pragma once
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
#include "vtr_geometry.h"
#include "rr_graph_view.h"
#include "router_lookahead_constants.h"

namespace util {

/**
 * @brief Delay/congestion estimate to travel a relative distance. The lookahead cost maps are arrays of these.
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
    Cost_Entry(float set_delay, float set_congestion, bool set_fill = false)
        : delay(set_delay)
        , congestion(set_congestion)
        , fill(set_fill) {}

    bool valid() const {
        return !(std::isnan(delay) || std::isnan(congestion));
    }
};

/// @brief Smallest-delay cost entry seen at each relative coordinate during a Dijkstra flood for a given segment
///        and channel type. [0..num_layers-1][0..device_ctx.grid.width()-1][0..device_ctx.grid.height()-1]
typedef vtr::NdMatrix<Cost_Entry, 3> t_routing_cost_map;

/// @brief Returns the cost to reach (delta_x, delta_y, to_layer) from a wire of the given
///        (chan_type, seg_index, from_layer).
typedef Cost_Entry (*WireCostFunc)(e_rr_type chan_type, int seg_index, int from_layer, int delta_x, int delta_y, int to_layer);

/**
 * @brief An entry in the Dijkstra expansion priority queue used to profile the routing network.
 *
 * The delay is estimated from the rr_indexed_data T_linear values (the same data the VPR 7 lookahead uses)
 * and the congestion is the accumulated base cost.
 */
class PQ_Entry {
  public:
    RRNodeId rr_node;          ///<The RR node this entry represents
    float delay;               ///<Backward delay of the path to this node; also the queue priority
    float congestion_upstream; ///<Backward congestion (base cost) of the path to this node

    /// @brief Creates an entry for node, extending parent's path. parent == nullptr creates the starting entry.
    PQ_Entry(RRNodeId node, const PQ_Entry* parent);

    bool operator<(const PQ_Entry& obj) const {
        // Inserted into max priority queue so want queue entries with a lower cost to be greater
        return delay > obj.delay;
    }
};

/// @brief Scratch data for run_dijkstra(), kept outside the call to avoid repeated allocations.
struct t_dijkstra_data {
    vtr::vector<RRNodeId, bool> node_expanded;       ///<Whether each node has already been expanded
    vtr::vector<RRNodeId, float> node_visited_costs; ///<Cheapest cost each node has been pushed with (-1 if never)
    std::priority_queue<PQ_Entry> pq;                ///<Expansion priority queue
};

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

/**
 * @brief For each tile, iterate over its OPINs and store which segment types are accessible from each OPIN
 */
t_src_opin_delays compute_router_src_opin_lookahead(bool is_flat,
                                                    int route_verbosity,
                                                    bool device_model_warnings);

t_chan_ipins_delays compute_router_chan_ipin_lookahead(int route_verbosity,
                                                       bool device_model_warnings);

t_ipin_primitive_sink_delays compute_intra_tile_dijkstra(const RRGraphView& rr_graph,
                                                         t_physical_tile_type_ptr physical_tile,
                                                         const t_physical_tile_loc& tile_loc);

/* returns index of a node from which to start routing */
RRNodeId get_chanxy_start_node(int layer, int start_x, int start_y, int target_x, int target_y, e_rr_type rr_type, int seg_index, int track_offset);

RRNodeId get_chanz_start_node(int start_x, int start_y, int seg_index, int track_offset, Direction dir);

/**
 * @brief Computes the absolute delta_x and delta_y offset
 * required to reach to_node from from_node
 * @param from_node The starting node
 * @param to_node The destination node
 * @return (delta_x, delta_y) offset required to reach to
 * to_node from from_node.
 */
std::pair<int, int> get_xy_deltas(RRNodeId from_node, RRNodeId to_node);

/**
 * @brief Computes the adjusted position of an RR graph node.
 * This function does not modify the position of the given node.
 * It only returns the computed adjusted position.
 * Adjusted position is:
 *  - CHANX/CHANY nodes: Position of the driver for directional wires and wire center for bidirectional wires
 *  - CHANZ nodes: (xlow, ylow) of the node, which would be the same as (xhigh, yhigh)
 *  - OPIN/IPIN nodes: Position of the channel it can reach based on which side of the block it is at.
 *  - SOURCE/SINK nodes: Centroid of the tile
 * @param rr The ID of the node whose adjusted position is desired.
 * @return The adjusted position (x, y).
 */
std::pair<int, int> get_adjusted_rr_position(const RRNodeId rr);

/**
 * @brief Computes the routing cost map for a given wire segment/channel type by running a Dijkstra flood
 * from a set of representative sample locations.
 */
t_routing_cost_map get_routing_cost_map(int longest_seg_length,
                                        unsigned from_layer_num,
                                        const e_rr_type chan_type,
                                        const t_segment_inf& segment_inf,
                                        const std::unordered_map<int, std::unordered_set<int>>& sample_locs,
                                        bool sample_all_locs,
                                        int route_verbosity,
                                        bool device_model_warnings);

/**
 * @brief Iterate over all of the wire segments accessible from the SOURCE/OPIN (stored in src_opin_delay_map) and return the minimum cost (congestion and delay) across them to the sink
 * @param src_opin_delay_map
 * @param delta_x
 * @param delta_y
 * @param to_layer_num
 * @param wire_cost_func call back function that would return a cost to get to a given location from the given segment
 * @return (delay, congestion)
 */
std::pair<float, float> get_cost_from_src_opin(const std::map<int, util::t_reachable_wire_inf>& src_opin_delay_map,
                                               int delta_x,
                                               int delta_y,
                                               int to_layer_num,
                                               WireCostFunc wire_cost_func);

void dump_readable_router_lookahead_map(const std::string& file_name,
                                        const std::vector<int>& dim_sizes,
                                        WireCostFunc wire_cost_func);

/// @brief Converts a routing channel type (CHANX/CHANY/CHANZ) to an index
/// to access the channel type dimension of the router lookahead table.
inline int chan_type_to_index(e_rr_type chan_type) {
    int chan_index;
    switch (chan_type) {
        case e_rr_type::CHANX:
            chan_index = 0;
            break;
        case e_rr_type::CHANY:
            chan_index = 1;
            break;
        case e_rr_type::CHANZ:
            chan_index = 2;
            break;
        default:
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Unsupported channel type.");
    }

    return chan_index;
}

} // namespace util
