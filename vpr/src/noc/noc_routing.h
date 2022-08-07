#ifndef VTR_NOCROUTING_H
#define VTR_NOCROUTING_H

/**
 * @file NocRouting.h
 * @brief This file defines the NocRouting class, which handles the
 * routing between routers within the NoC. It describes the routing
 * algorithm for the NoC.
 * 
 * Overview
 * ========
 * The NocRouting class is an abstract class. It is intended to be used as
 * a base class and should not be used on its own. The NocRouting
 * class describes the structure of what a routing algorithm class should
 * look like. 
 * 
 * Usage
 * -----
 * When a new routing algorithm for the NoC is needed, a new class should be
 * made that inherits this class. Then two things need to be done:
 *  - The routing algorithm should be implemented inside the route_flow
 *    function and should match the prototype dclared below
 *  - The found route should be stored inside the member variable defined below
 *    called routed_path.
 */

#include <vector>

#include "noc_data_types.h"
#include "noc_traffic_flows.h"
#include "noc_storage.h"

class NocRouting {

    // protected pure virtual functions that should be implemendted in derived classes.
  protected:
    virtual ~NocRouting() {}

    /**
     * @brief Finds a route that goes from the starting router in a 
     * traffic flow to the destination router. A route consists of a series
     * of links that should be traversed when travelling between two routers
     * within the NoC. Derived classes will
     * primarily differ by the routing algorithm they use. The expectation
     * is that this function should be overridden in the derived classes
     * to implement the routing algorithm.
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
    virtual void route_flow(NocRouterId src_router_id, NocRouterId sink_router_id, std::vector<NocLinkId>& flow_route, const NocStorage& noc_model) = 0;
};

#endif
