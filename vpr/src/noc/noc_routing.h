#ifndef VTR_NOCROUTING_H
#define VTR_NOCROUTING_H

/**
 * @file
 * @brief This file defines the NocRouting class, which handles the
 * packet routing between routers within the NoC. It describes the routing
 * algorithm for the NoC.
 * 
 * Overview
 * ========
 * The NocRouting class is an abstract class. It is intended to be used as
 * a base class and should not be used on its own. The NocRouting
 * class is used as a base (interface) class.
 * 
 * Usage
 * -----
 * When a new routing algorithm for the NoC is needed, a new class should be
 * made that inherits this class. Then the following needs to be done:
 *  - The routing algorithm should be implemented inside the route_flow
 *    function and should match the prototype declared below
 */

#include <vector>

#include "noc_data_types.h"
#include "noc_traffic_flows.h"
#include "noc_storage.h"

class NocRouting {
    // pure virtual functions that should be implemented in derived classes.
  public:
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
     * the starting point of the route within the NoC. This represents a 
     * physical router on the FPGA.
     * @param sink_router_id The destination router of a traffic flow.
     * Identifies the ending point of the route within the NoC.This represents a 
     * physical router on the FPGA.
     * @param flow_route Stores the path returned by this function
     * as a series of NoC links found by 
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
