/**
 * @file rr_graph_utils.h
 *
 * @brief This file includes the most-utilized functions that manipulate the RRGraph object.
 */

#ifndef RR_GRAPH_UTILS_H
#define RR_GRAPH_UTILS_H

/* Include header files which include data structures used by
 * the function declaration
 */
#include <vector>
#include "rr_graph_fwd.h"
#include "rr_node_types.h"
#include "rr_graph_view.h"
#include "device_grid.h"

struct t_pin_chain_node {
    int pin_physical_num = OPEN;
    int nxt_node_idx = OPEN;

    t_pin_chain_node() = default;
    t_pin_chain_node(int pin_num, int nxt_idx)
        : pin_physical_num(pin_num)
        , nxt_node_idx(nxt_idx) {}
};

struct t_cluster_pin_chain {
    std::vector<int> pin_chain_idx;                    // [pin_physical_num] -> chain_num
    std::vector<std::vector<t_pin_chain_node>> chains; // [chain_num] -> chain_node
    std::vector<int> chain_sink;                       // [chain_num] -> pin num of the chain's sink

    t_cluster_pin_chain() = default;
    t_cluster_pin_chain(std::vector<int> pin_chain_idx_,
                        std::vector<std::vector<t_pin_chain_node>> chains_,
                        std::vector<int> chain_sink_)
        : pin_chain_idx(std::move(pin_chain_idx_))
        , chains(std::move(chains_))
        , chain_sink(std::move(chain_sink_)) {}

    t_cluster_pin_chain(t_cluster_pin_chain&& other) = default;
    t_cluster_pin_chain& operator=(t_cluster_pin_chain&& other) = default;
};

/**
 * @brief Get switches in a RRGraph starting at from_node and ending at to_node.
 *
 * @details
 * Uses RRGraphView::find_edges(), then converts these edges to switch IDs.
 *
 * @return A vector of switch IDs
 */
std::vector<RRSwitchId> find_rr_graph_switches(const RRGraph& rr_graph,
                                               const RRNodeId& from_node,
                                               const RRNodeId& to_node);

/**
 * @brief This function generates and returns a vector indexed by RRNodeId containing a vector of fan-in edges for each node.
 *
 * @note
 * This function is CPU expensive; complexity O(E) where E is the number of edges in rr_graph
 */
vtr::vector<RRNodeId, std::vector<RREdgeId>> get_fan_in_list(const RRGraphView& rr_graph);

/**
 * @brief This function sets better locations for SINK nodes.
 *
 * @details
 * build_rr_graph() sets the location of SINK nodes to span the entire tile they are in. This function sets the location
 * of SINK nodes to be the average coordinate of the "cluster-edge" IPINs to which they are connected
 */
void rr_set_sink_locs(const RRGraphView& rr_graph, RRGraphBuilder& rr_graph_builder, const DeviceGrid& grid);

/**
 * @brief Returns the segment number (distance along the channel) of the connection box from from_rr_type (CHANX or
 * CHANY) to to_node (IPIN).
 */
int seg_index_of_cblock(const RRGraphView& rr_graph, t_rr_type from_rr_type, int to_node);

/**
 * @brief Returns the segment number (distance along the channel) of the switch box from from_node (CHANX or CHANY) to
 * to_node (CHANX or CHANY).
 *
 * @details
 * The switch box on the left side of a CHANX segment at (i,j) has seg_index = i-1, while the switch box on the right
 * side of that segment has seg_index = i.  CHANY stuff works similarly.  Hence the range of values returned is 0 to
 * device_ctx.grid.width()-1 (if from_node is a CHANX) or 0 to device_ctx.grid.height()-1 (if from_node is a CHANY).
 */
int seg_index_of_sblock(const RRGraphView& rr_graph, int from_node, int to_node);

/**
 * @brief This function checks whether all inter-die connections are form OPINs. Return "true"
 * if that is the case. Can be used for multiple purposes. For example, to determine which type of bounding
 * box to be used to estimate the wire-length of a net.
 *
 * @param rr_graph
 *
 * @return limited_to_opin
 */
bool inter_layer_connections_limited_to_opin(const RRGraphView& rr_graph);
#endif