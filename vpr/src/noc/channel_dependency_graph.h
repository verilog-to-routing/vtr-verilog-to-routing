#ifndef VTR_CHANNEL_DEPENDENCY_GRAPH_H
#define VTR_CHANNEL_DEPENDENCY_GRAPH_H

/**
 * @file
 * @brief This file declares the ChannelDependencyGraph class.
 *
 * Overview
 * ========
 * The NoC routing algorithm might generate routes that cause a deadlock.
 * The Channel Dependency Graph (CDG) is formed by associating a node with each
 * link in the NoC topology. For each traffic flow route, we consider the
 * consecutive links traversed to reach the destination. For each consecutively
 * traversed link pair (Li, Lj) in the given topology, we add an directed edge from
 * Vi to Vj, where Vi and Vj are nodes in CDG that are associated with Li and Lj.
 * Absence of cycles in the formed CDG guarantees deadlock freedom.
 *
 * To learn more about channel dependency graph, refer to the following papers:
 * 1) Glass, C. J., & Ni, L. M. (1992). The turn model for adaptive routing.
 * ACM SIGARCH Computer Architecture News, 20(2), 278-287.
 * 2) Dally, & Seitz. (1987). Deadlock-free message routing in multiprocessor
 * interconnection networks. IEEE Transactions on computers, 100(5), 547-553.
 */

#include "vtr_vector.h"
#include "noc_data_types.h"
#include "noc_storage.h"
#include "noc_traffic_flows.h"

class ChannelDependencyGraph {
  public:
    ChannelDependencyGraph() = delete;

    /**
     * @brief Constructor
     *
     * @param n_links The total number of NoC links.
     * @param traffic_flow_routes The route of each traffic flow generated
     * by a routing algorithm.
     */
    ChannelDependencyGraph(const NocStorage& noc_model,
                           const NocTrafficFlows& traffic_flow_storage,
                           const vtr::vector<NocTrafficFlowId, std::vector<NocLinkId>>& traffic_flow_routes,
                           const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs);

    /**
     * @brief Checks whether CDG has any cycles. A cycle implies that
     * deadlocks are possible. If CDG has no cycles, deadlock freedom
     * is guaranteed.
     *
     * @return True if the CDG has any cycles, otherwise false is returned.
     */
    bool has_cycles();

  private:
    /** An adjacency list used to represent channel dependency graph.*/
    vtr::vector<NocLinkId, std::vector<NocLinkId>> adjacency_list_;
};

#endif //VTR_CHANNEL_DEPENDENCY_GRAPH_H
