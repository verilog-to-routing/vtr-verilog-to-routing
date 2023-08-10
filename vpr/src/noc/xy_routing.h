#ifndef VTR_XYROUTING_H
#define VTR_XYROUTING_H

/**
 * @file 
 * @brief This file defines the XYRouting class, which represents a direction
 * oriented routing algorithm.
 * 
 * Overview
 * ========
 * The XYRouting class performs packet routing between routers in the
 * NoC. This class is based on the XY routing algorithm.
 * 
 * XY Routing Algorithm
 * --------------------
 * The algorithm works by first travelling in the X-direction and then
 * in the Y-direction. 
 * 
 * First the algorithm compares the source router and the 
 * destination router, checking the coordinates in the X-axis. If the
 * coordinates are not the same (so not horizontally aligned), then the
 * algorithm moves in the direction towards the destination router in the 
 * X-axis. For each router in the path, the algorithm checks again to see
 * whether it is horizontally aligned with the destination router;
 * otherwise it moves in the direction of the destination router (once again
 * the movement is done in the X-axis). 
 * 
 * Once horizontally aligned (current router in the path
 * has the same X-coordinate as the destination) the algorithm
 * checks to see whether the y-axis coordinates match between the destination 
 * router and the current router in the path (checking for vertical alignment).
 * Similar to the x-axis movement, the algorithm moves in the Y-axis towards
 * the destination router. Once again, at each router in the path the algorithm
 * checks for vertical alignment; if not aligned it then moves in the y-axis
 * towards the destination router until it is aligned vertically.
 * 
 * The main aspect of this algorithm is that it is deterministic. It will always
 * move in the horizontal direction and then the vertical direction to reach
 * the destination router. The path is never affected by things like congestion,
 * latency, distance and etc..). 
 * 
 * Below we have an example of the path determined by this algorithm for a 
 * 3x3 mesh NoC:
 * ```
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
 * ```
 * In the example above, the router marked with the '*' character is the start
 * and the router marked with the '$' character is the destination. The path
 * determined by the XY-Routing algorithm is shown as "<++++".
 * 
 * Note that this routing algorithm in inherently deadlock free.
 * 
 * Usage
 * -----
 * It is recommended to use this algorithm when the NoC topology is of type
 * Mesh. This algorithm will work for other types of topologies but the
 * directional nature of the algorithm makes it ideal for mesh topologies. If
 * the algorithm fails to find a router then an error is thrown; this should
 * only happen for non-mesh topologies.
 * 
 */

#include "noc_routing.h"

/**
 * @brief This enum describes the all the possible
 * directions the XY routing algorithm can choose
 * to travel.
 */
enum class RouteDirection {
    LEFT,  /*!< Moving towards the negative X-axis*/
    RIGHT, /*!< Moving towards the positive X-axis*/
    UP,    /*!< Moving towards the positive Y-axis*/
    DOWN   /*!< Moving towards the negative Y-axis*/
};

class XYRouting : public NocRouting {
  public:
    ~XYRouting() override;

    /**
     * @brief Finds a route that goes from the starting router in a 
     * traffic flow to the destination router. Uses the XY-routing
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
     * @param flow_route Stores the path returned by this function
     * as a series of NoC links found by 
     * a NoC routing algorithm between two routers in a traffic flow.
     * The function will clear any
     * previously stored path and re-insert the new found path between the
     * two routers. 
     * @param noc_model A model of the NoC. This is used to traverse the
     * NoC and find a route between the two routers.
     */
    void route_flow(NocRouterId src_router_id, NocRouterId sink_router_id, std::vector<NocLinkId>& flow_route, const NocStorage& noc_model) override;

    // internally used helper functions
  private:
    /**
     * @brief Based on the position of the current router the algorithm is
     * visiting, this function determines the next direction to travel. 
     * 
     * @param sink_router_x_position The horizontal grid position of the
     * destination router on the FPGA
     * @param sink_router_y_position  The vertical grid position of the
     * destination router on the FPGA
     * @param curr_router_x_position The horizontal grid position of the
     * router that is currently being visited on the FPGA
     * @param curr_router_y_position The vertical grid position of the router
     * that is currently being visited on the FPGA
     * @return RouteDirection The direction to travel next
     */
    RouteDirection get_direction_to_travel(int sink_router_x_position, int sink_router_y_position, int curr_router_x_position, int curr_router_y_position);

    /**
     * @brief Given the direction to travel next, this function determines
     * the outgoing link that should be used to travel in the intended direction.
     * Each router may have any number of outgoing links and each link is not
     * guaranteed to point in the intended direction, So this function makes
     * sure that the link chosen points in the intended direction.
     * 
     * @param curr_router_id The physical router on the FPGA that the routing
     * algorithm is currently visiting. 
     * @param curr_router_x_position The horizontal grid position of the
     * router that is currently being visited on the FPGA
     * @param curr_router_y_position he vertical grid position of the router
     * that is currently being visited on the FPGA
     * @param next_step_direction The direction to travel next
     * @param flow_route Stores the path as a series of NoC links found by 
     * a NoC routing algorithm between two routers in a traffic flow. The 
     * NoC link found to travel next will be added to this path.
     * @param visited_routers Keeps track of which routers have been reached
     * already while traversing the NoC.
     * @param noc_model A model of the NoC. This is used to traverse the
     * NoC and find a route between the two routers.
     * @return true A suitable link was found that we can traverse next
     * @return false No suitable link was found that could be traversed
     */
    bool move_to_next_router(NocRouterId& curr_router_id, int curr_router_x_position, int curr_router_y_position, RouteDirection next_step_direction, std::vector<NocLinkId>& flow_route, std::unordered_set<NocRouterId>& visited_routers, const NocStorage& noc_model);
};

#endif
