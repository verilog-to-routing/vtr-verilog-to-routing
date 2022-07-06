#ifndef VTR_XYROUTING_H
#define VTR_XYROUTING_H

/**
 * @file 
 * @brief This file defines the XYRouting class, which represents a directional
 * oriented routing algorithm.
 * 
 * Overview
 * ========
 * The XYRouting class performs routing between routers in the
 * NoC. This class is based on the XY routing algorithm.
 * 
 * XY Routing Algorithm
 * --------------------
 * The algorithm works by first travelling in the X-direction and then
 * in the Y-direction. 
 * 
 * First the algorithm compares the source router and the 
 * destination router, checking the corrdinates in the X-axis. If the
 * corrdinates are not the same (so not horizontall aligned), then the
 * algorithm moves in the direction towards the destination router in the 
 * X-axis. For each router in the path, the algorithm checks again to see
 * whether it is horizontall aligned with the destination router,
 * otherwise in moves in the direction to the destination router (once again
 * the movement is done in the X-axis). 
 * 
 * Once horizontally aligned (current router in the path
 * has the same X-coordinate as the destination) the algorithm
 * checks to see whether the y-axis coordinates match between the destination 
 * router and the current router in the path (checking for vertical alignment).
 * Similiar to the x-axis movement, the algorithm moves in the Y-axis towards
 * the destination router. Once again, at each router in the path the algorithm
 * checks for vertical alignment, if not aligned it then moves in the y-axis
 * towards the destination router until it is aligned vertically.
 * 
 * The main aspect of this algorithm is that it is deterministic. It will always
 * move in the horizontal direction and then the vertical direction to reach
 * the destination router. The path is never affected by things like congestion,
 * latency, distance and etc..). 
 * 
 * Below we have an example of the path determined by this algorithm for a 
 * 3x3 mesh NoC:
 * 
 *   ---------                   ---------                    ---------
 *   /       /                   /       /                    /       /
 *   /   $   / ----------------- /       / ------------------ /       /
 *   /       /                   /       /                    /       /
 *   ---------                   ---------                    --------- 
 *       /^                          /                            /
 *       /+                          /                            /
 *       /+                          /                            /
 *       /+                          /                            /
 *   ---------                   ---------                    ---------
 *   /       /                   /       /                    /       /
 *   /       / ----------------- /       / ------------------ /       /
 *   /       /                   /       /                    /       /
 *   ---------                   ---------                    ---------     
 *       /^                          /                            /
 *       /+                          /                            /
 *       /+                          /                            /
 *       /+                          /                            /
 *   ---------                   ---------                    ---------
 *   /       /<++++++++++++++++++/       /<+++++++++++++++++++/       /
 *   /       / ----------------- /       / ------------------ /   *   /
 *   /       /                   /       /                    /       /
 *   ---------                   ---------                    ---------
 * 
 * In the example above, the router marked with the '*' character is the start
 * and the router marked with the '$' character is the destination. The path
 * determined by the XY-Routing algorithm is shown as "<++++".
 * 
 * Usage
 * -----
 * It is recommmended to use this algorithm when the NoC topology is of type
 * Mesh. This algorithm will work for other types of toplogies but the
 * directional nature of the algorithm make it ideal for mesh topologies. If
 * the algorithm fails to find a router then an error is thrown.
 * 
 */

#include "NocRouting.h"

/**
 * @brief This enum describes the all the possible
 * directions the XY routing algorithm can choose
 * to travel.
 */
enum RouteDirection {
    LEFT, /*!< Moving towards the negative X-axis*/ 
    RIGHT,/*!< Moving towards the positive X-axis*/
    UP, /*!< Moving towards the positive Y-axis*/
    DOWN /*!< Moving towards the negative Y-axis*/
};

class XYRouting : public NocRouting {

  private:

    /**
     * @brief Keeps track of which routers have been vistited already
     * while traversing the NoC. This variable will help determine
     * cases where a route could not be found and the algorithm is
     * stuck going back and forth between routers it has already
     * visited.
     * 
     */
    std::unordered_set<NocRouterId> visited_routers;

  public:
    /**
     * @brief Finds a route that goes from the starting router in a 
     * traffic flow to the destination router. Uses the XY-routing
     * algorithm to determine the route.
     * 
     * @param src_router_id The source router of a traffic flow. Identifies 
     * the starting point of the route within the NoC. This is represents
     * a unique identifier of the source router.
     * @param sink_router_id The destination router of a traffic flow.
     * Identifies the ending point of the route within the NoC.This is 
     * represents a unique identifier of the destination router.
     * @param noc_model A model of the NoC. This is used to traverse the
     * NoC and find a route between the two routers.
     */
    void route_flow(NocRouterId src_router_id, NocRouterId sink_router_id, const NocStorage& noc_model) override;

    /**
     * @brief Based on the position of the current router the algorithm is
     * visiting, this function determines the next direction to travel. 
     * 
     * @param sink_router_x_position 
     * @param sink_router_y_position 
     * @param curr_router_x_position 
     * @param curr_router_y_position 
     * @return RouteDirection 
     */
    RouteDirection get_direction_to_travel(int sink_router_x_position, int sink_router_y_position, int curr_router_x_position, int curr_router_y_position);

    /**
     * @brief Given the direction to travel next, this function determines
     * the outgoing link that should be used to travel in the inteded direction.
     * Each router may have any number of outgoing links and each link is not
     * guaranteed to point in the intended direction, So this function makes
     * sure that the link chosen points in the intended direction.
     * 
     * @param curr_router
     * @param curr_router_x_position
     * @param curr_router_y_position
     * @param next_step_direction 
     * @param noc_model 
     * @return true 
     * @return false 
     */
    bool move_to_next_router(NocRouterId& curr_router_id, int curr_router_x_position, int curr_router_y_position, RouteDirection next_step_direction, const NocStorage& noc_model);

};

#endif
