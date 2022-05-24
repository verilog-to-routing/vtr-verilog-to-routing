#ifndef VTR_XYROUTING_H
#define VTR_XYROUTING_H

#include "NocRouting.h"

enum RouteDirection {
    LEFT,
    RIGHT,
    UP,
    DOWN
};

/**
 * The XYRouting class description header file.
 *
 * The XYRouting class inherits from the NocRouting class and performs routing between any two routers in the NoC using the XY routing algorithm.
 * This algorithm works by moving from the source router in the x-direction until the x position is the same as the destination routers x position.
 * Then the algorithm moves in the y-direction until the y position is the same as the destination routers y position.
 *
 * This routing algorithm should only be used with a standard mesh topology. There is some error checking for cases where the mesh topology is
 * not fully connected, so an error is thrown that no route was found. But this error checking will not work for non mesh topologies.
 *
 * */

class XYRouting : public NocRouting {

  public:
    void route_flow(NocRouterId src_router, NocRouterId sink_router, std::vector<NocLinkId>& route_link_list, const NocStorage& noc_model) override;

    RouteDirection get_direction_to_travel(int sink_router_x_position, int sink_router_y_position, int curr_router_x_position, int curr_router_y_position);

    bool move_to_next_router(NocRouterId& current_router, std::vector<NocLinkId>& route_link_list, RouteDirection next_step_direction, const NocStorage& noc_model);

};

#endif
