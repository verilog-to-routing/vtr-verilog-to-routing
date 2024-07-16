#ifndef BFS_ROUTING
#define BFS_ROUTING

/**
 * @file 
 * @brief This file defines the BFSRouting class.
 * 
 * Overview
 * ========
 * The BFSRouting class performs packet routing between physical routers in the
 * FPGA. This class is based on the BFS algorithm.
 * 
 * The BFS algorithm is used to explore the NoC from the starting router in the
 * traffic flow. Once the destination router is found a path from the source to
 * the destination router is generated. The main advantage of this algorithm is
 * that the found path from the source to the destination router uses the 
 * minimum number of links required within the NoC. This algorithm does not
 * guarantee deadlock freedom. In other words, the algorithm might generate
 * routes that form cycles in channel dependency graph.
 */

#include <vector>
#include <unordered_map>

#include "noc_routing.h"

class BFSRouting : public NocRouting {
  public:
    ~BFSRouting() override;

    /**
     * @brief Finds a route that goes from the starting router in a 
     * traffic flow to the destination router. Uses the BFS
     * algorithm to determine the route. A route consists of a series
     * of links that should be traversed when travelling between two routers
     * within the NoC.
     * 
     * @param src_router_id The source router of a traffic flow. Identifies 
     * the starting point of the route within the NoC. This represents a 
     * physical router on the FPGA.
     * @param sink_router_id The destination router of a traffic flow.
     * Identifies the ending point of the route within the NoC.This represents a 
     * physical router on the FPGA.
     * @param traffic_flow_id The unique ID for the traffic flow being routed.
     * @param flow_route Stores the path returned by this function
     * as a series of NoC links found by 
     * a NoC routing algorithm between two routers in a traffic flow.
     * The function will clear any
     * previously stored path and re-insert the new found path between the
     * two routers. 
     * @param noc_model A model of the NoC. This is used to traverse the
     * NoC and find a route between the two routers.
     */
    void route_flow(NocRouterId src_router_id,
                    NocRouterId sink_router_id,
                    NocTrafficFlowId traffic_flow_id,
                    std::vector<NocLinkId>& flow_route,
                    const NocStorage& noc_model) override;

    // internally used helper functions
  private:
    /**
     * @brief Traces the path taken by the BFS routing algorithm from the destination router to the starting router. 
     * Starting with the destination router, the parent link (link taken to get to this router) is found and is added
     * to the path. Then the algorithm moves to the source router of the parent link. Then it repeats the previous process
     * of finding the parent link, adding it to the path and moving to the source router. This is repeated until the
     * starting router is reached.
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
    void generate_route(NocRouterId sink_router_id,
                        std::vector<NocLinkId>& flow_route,
                        const NocStorage& noc_model,
                        const std::unordered_map<NocRouterId, NocLinkId>& router_parent_link);
};

#endif