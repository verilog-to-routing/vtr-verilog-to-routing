#ifndef NOC_PLACE_UTILS_H
#define NOC_PLACE_UTILS_H

#include "globals.h"
#include "noc_routing.h"
#include "place_util.h"
#include "vtr_assert.h"


/* Defines how the links found in a traffic flow are updated in terms
   of their bandwidth usage.
*/
enum class link_usage_update_state {
    /* State 1: The link usages have to be incremented as the traffic
      flow route route has been updated 
    */
    increment, 
    /* State 2: The link usages have to be decremented as the traffic flow
      route is being removed
    */
    decrement
};

/**
 * @brief Routes all the traffic flows within the NoC and updates the link usage
 * for all links. This should be called after initial placement, where all the 
 * logical NoC router blocks have been placed for the first time and no traffic
 * flows have been routed yet. This function should also only be used once as
 * its intended use is to initialize the routes for all the traffic flows.
 * 
 * @param place_ctx This is used to determine the locations where the logical
 * routers have been placed after initial placement.
 * 
 */
void initial_noc_placement(PlacementContext& place_ctx);

/**
 * @brief Determines the hard router block on the FPGA where a logical router 
 * cluster block has been previously placed. The grid location where the cluster
 * blocks have bee placed is first found. Then the grid location is used to
 * determine the router block that is located there.
 * 
 * 
 * @param cluster_blk_id Represents a logical router cluster block. The hard 
 * block that this cluster has been placed on needs to be found.
 * @param placed_cluster_block_locations A datastructure that identifies the
 * placed grid locations of all cluster blocks.
 * @param noc_model An internal model that describes the NoC. COntains the 
 * locations of all hard router blocks in the device.
 * @return NocRouterId Identifies the hard block where the provided logical 
 * router cluster block has been placed on.
 */
NocRouterId get_router_block_id_where_cluster_block_is_placed(ClusterBlockId cluster_blk_id, const vtr::vector_map<ClusterBlockId, t_block_loc>* placed_cluster_block_locations, const NocStorage& noc_model);

/**
 * @brief Updates the bandwidth usages of links found in a routed traffic flow.
 * The link bandwidth usages are either incremeneted or decremented by the 
 * bandwidth of the traffic flow. If the traffic flow route is being deleted,
 * then the link bandwidth needs to be decremented. If the traffic flow
 * route has just been added then the link badnwidth needs to be incremented.
 * This function needs to be called everytime a traffic flow has been newly
 * routed.
 * 
 * @param traffic_flow_route The routed path for a traffic flow. This
 * contains a collection of links in the NoC.
 * @param noc_model An internal model that describes the NoC. COntains the 
 * locations of all hard router blocks in the device.
 * @param how_to_update_links Determines how the bandwidths of links found
 * in the traffic flow route are updated.
 * @param traffic_flow_bandwidth The bandwidth of a traffic flow. This will
 * be used to update bandwidth usage of the links.
 */
void update_traffic_flow_link_usage(const std::vector<NocLinkId>& traffic_flow_route, NocStorage& noc_model, link_usage_update_state how_to_update_links, double traffic_flow_bandwidth);















#endif