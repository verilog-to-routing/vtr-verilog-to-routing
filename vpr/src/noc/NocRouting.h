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

    private:
        /**
         * @brief Stores the path found by a NoC routing algorithm between two 
         * routers. This variable should be cleared before each iteration
         * where a route needs to be found between two routers. Once a route is
         * found, it can be accessed through this variable. 
         * 
         * A route consists of a series of links that should be traversed when
         * beginning at a start router to reach the destination router. 
         * 
         * The derived classes that implement a NoC routing algorithm should
         * all store the found route in this variable. Then external functions
         * should access the found route through this variable.
         */
        std::vector<NocLinkId> routed_path;

    // protected pure virtual functions that should be implemendted in derived classes.
    protected:

        /**
         * @brief Finds a route that goes from the starting router in a 
         * traffic flow to the destination router. Derived classes will
         * primarily differ by the routing algorithm they use. The expectation
         * is that this function should be implemented in the derived classes
         * to 
         * 
         * @param src_router The source router if a traffic flow. Identifies the
         * starting point within the NoC.
         * @param sink_router The destination router of a traffic flow.
         * Identifies the destination point within the NoC.
         * @param noc_model A model of the NoC. This is used to traverse the
         * NoC and find a route between the two routers.
         */
        virtual void route_flow(NocRouterId src_router, NocRouterId sink_router,const NocStorage& noc_model) = 0;


    // protected functions that are only accessible to derived classes. 
    // These functions are used to modify and access the routed_path variable.
    protected:

        /**
         * @brief Empties the routed_path variable. Whenever route needs
         * to be found between two routers, the route is stored inside
         * the routed_path variable. So each time a new route needs to be
         * found, the routed_path variable needs to be empty so that
         * it does not corrupt the new route. This function should
         * be called before a new route needs to be found.
         * 
         * NOTE: If this function is called after finding a route between two
         * routers in the NoC, then the found route is deleted.
         */
        void clear_routed_path(void);

        /**
         * @brief Adds a single link to the route between two routers.
         * 
         * @param link_in_route represents the next traversal step from
         * a router within the NoC.
         */
        void add_link_to_routed_path(NocLinkId link_in_route);

        /**
         * @brief Get the routed_path variable, which stores the
         * route between the starting and destination routers of
         * a traffic flow.
         * 
         * @return const std::vector<NocLinkId>& A vector of NocLinkIds that
         * represent all the links that should be traversed within the NoC
         * to get from a starting router to a destination router.
         */
        const std::vector<NocLinkId>& get_routed_path(void) const;

};

#endif
