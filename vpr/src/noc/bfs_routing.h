#ifndef BFS_ROUTING
#define BFS_ROUTING

/**
 * @file 
 * @brief This file defines the BFSRouting class, which represents a routing algorithm
 * based on Breadth First Search (BFS).
 * 
 * Overview
 * ========
 * The BFSRouting class performs routing between routers in the
 * NoC. This class is based on the BFS algorithm.
 * 
 * The BFS algorithm is used to explore the NoC from the starting router in the
 * traffic flow. Once the destination router is found a path from the source to
 * the destination router is generated. The main advantage of this algorithm is
 * that the found path from the source to the destination router uses the 
 * minimum number of links required within the NoC.
 * 
 */

#include <vector>
#include <unordered_map>
#include <queue>

#include "noc_routing.h"

class BFSRouting : public NocRouting {
  public:
    ~BFSRouting() override;

    /**
     * @brief Finds a route that goes from the starting router in a 
     * traffic flow to the destination router. Uses the breadth first search
     * algorithm to determine the route. A route consists of a series
     * of links that should be traversed when travelling between two routers
     * within the NoC.
     * 
     * @param src_router_id The source router of a traffic flow. Identifies 
     * the starting point of the route within the NoC. This is represents
     * a unique identifier of the source router.
     * @param sink_router_id The destination router of a traffic flow.
     * Identifies the ending point of the route within the NoC.This is 
     * represents a unique identifier of the destination router.
     * @param flow_route Stores the path as a series of NoC links found by 
     * a NoC routing algorithm between two routers in a traffic flow.
     * The function will clear any
     * previously stored path and re-insert the new found path between the
     * two routers. 
     * @param noc_model A model of the NoC. This is used to traverse the
     * NoC and find a route between the two routers.
     */
    void route_flow(NocRouterId src_router_id, NocRouterId sink_router_id, std::vector<NocLinkId>& flow_route, const NocStorage& noc_model) override;

    /**
     * @brief Traces the path taken by the BFS routing algorithm from the destination router to the source router. 
     * Starting with the destination router, the parent link (link taken to get to this router) is found and is added
     * to the path. Then the algorithm moves to the source router of the parent link. Then it repeats the previous process
     * of finding the parent link, adding it to the path and moving to the source router. This is repeated until the
     * source router is reached.
     * 
     * @param start_router_id The router to use as a starting point
     * when tracing back the route from the destination router. 
     * to the the starting router. Generally this
     * would be the sink router of the flow.
     * @param flow_route Stores the path as a series of NoC links found by 
     * a NoC routing algorithm between two routers in a traffic flow.
     * The function will clear any
     * previously stored path and re-insert the new found path between the
     * two routers. 
     * @param noc_model A model of the NoC. This is used to traverse the
     * NoC and find a route between the two routers.
     * @param router_parent_link Contains the parent link associated to each
     * router in the NoC (parent link is the link used to visit the router during
     * the BFS routing algorithm).
     */
    void generate_route(NocRouterId sink_router_id, std::vector<NocLinkId>& flow_route, const NocStorage& noc_model, const std::unordered_map<NocRouterId, NocLinkId>& router_parent_link);
};

#endif