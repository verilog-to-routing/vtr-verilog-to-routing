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
 * only happen for non-mesh topologies. If this algorithms is used for non-mesh
 * topologies, it might be able to generate routes for all traffic flows, but
 * the generated routes are not guaranteed to be deadlock free in a non-mesh
 * topology. Im mesh and torus topologies, xy-routing algorithm is guaranteed
 * to generate deadlock free routes.
 */

#include "turn_model_routing.h"


class XYRouting : public TurnModelRouting {
  public:
    ~XYRouting() override;

  private:
    const std::vector<TurnModelRouting::Direction>& get_legal_directions(NocRouterId src_router_id,
                                                                         NocRouterId curr_router_id,
                                                                         NocRouterId dst_router_id,
                                                                         const NocStorage& noc_model) override;

    TurnModelRouting::Direction select_next_direction(const std::vector<TurnModelRouting::Direction>& legal_directions,
                                                      NocRouterId src_router_id,
                                                      NocRouterId dst_router_id,
                                                      NocRouterId curr_router_id,
                                                      NocTrafficFlowId traffic_flow_id,
                                                      const NocStorage& noc_model) override;

    bool is_turn_legal(const std::array<std::reference_wrapper<const NocRouter>, 3>& noc_routers) const override;

  private:
    const std::vector<TurnModelRouting::Direction> x_axis_directions {TurnModelRouting::Direction::LEFT, TurnModelRouting::Direction::RIGHT};
    const std::vector<TurnModelRouting::Direction> y_axis_directions {TurnModelRouting::Direction::UP, TurnModelRouting::Direction::DOWN};
};

#endif