/****************************************************************************
 * This file include most-utilized functions that manipulate on the
 * RRGraph object
 ***************************************************************************/
#include "rr_graph_util.h"
#include "rr_graph_obj.h"

/****************************************************************************
 * Find the switches interconnecting two nodes
 * Return a vector of switch ids
 ***************************************************************************/
std::vector<RRSwitchId> find_rr_graph_switches(const RRGraph& rr_graph,
                                               const RRNodeId& from_node,
                                               const RRNodeId& to_node) {
    std::vector<RRSwitchId> switches;
    std::vector<RREdgeId> edges = rr_graph.find_edges(from_node, to_node);
    if (true == edges.empty()) {
        /* edge is open, we return an empty vector of switches */
        return switches;
    }

    /* Reach here, edge list is not empty, find switch id one by one
     * and update the switch list
     */
    for (auto edge : edges) {
        switches.push_back(rr_graph.edge_switch(edge));
    }

    return switches;
}
