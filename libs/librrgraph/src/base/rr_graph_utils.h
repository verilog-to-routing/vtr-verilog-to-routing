#ifndef RR_GRAPH_UTILS_H
#define RR_GRAPH_UTILS_H

/* Include header files which include data structures used by
 * the function declaration
 */
#include <vector>
#include "rr_graph_fwd.h"
#include "rr_node_types.h"
#include "rr_graph_view.h"

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

/* Get node-to-node switches in a RRGraph */
std::vector<RRSwitchId> find_rr_graph_switches(const RRGraph& rr_graph,
                                               const RRNodeId& from_node,
                                               const RRNodeId& to_node);

/**
 * @brief This function generates and returns a vector indexed by RRNodeId containing a list of fan-in edges for each node.
 * @param rr_graph
 * @return node_fan_in_list
 */
vtr::vector<RRNodeId, std::vector<RREdgeId>> get_fan_in_list(const RRGraphView& rr_graph);

/**
 * @brief This function sets better locations for SOURCE and SINK nodes.
 *
 * @details
 * build_rr_graph() sets the location of SINK and SOURCE nodes to span the entire tile they are in. this function:
 * - sets the location of SOURCE nodes to be the average coordinate of the OPINs of their cluster block to which they are connected
 * - sets the location of SINK nodes to be the average coordinate of the IPINs of their cluster block to which they are connected
 *
 * Note: only changes nodes in tiles which have dimensions greater than 1x1
 *
 * @param rr_graph
 * @param rr_graph_builder
 */
void set_sink_locs(const RRGraphView& rr_graph, RRGraphBuilder& rr_graph_builder);

int seg_index_of_cblock(const RRGraphView& rr_graph, t_rr_type from_rr_type, int to_node);
int seg_index_of_sblock(const RRGraphView& rr_graph, int from_node, int to_node);

/**
 * @brief This function checks whether all inter-die connections are form OPINs. Return "true"
 * if that is the case. Can be used for multiple purposes. For example, to determine which type of bounding
 * box to be used to estimate the wire-length of a net.
 * @param rr_graph
 * @return limited_to_opin
 */
bool inter_layer_connections_limited_to_opin(const RRGraphView& rr_graph);
#endif