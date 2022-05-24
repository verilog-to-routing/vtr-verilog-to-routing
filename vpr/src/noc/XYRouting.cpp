//
// Created by srivatsan on 17/05/22.
//
#include <vector>

#include "XYRouting.h"
#include "globals.h"
#include "vpr_error.h"

void XYRouting::route_flow(NocRouterId src_router, NocRouterId sink_router, std::vector<NocLinkId>& route_link_list, const NocStorage& noc_model){

    // keep track of whether a legal route exists between the two routers
    bool route_exists = true;

    // keep track of the last router in the route as we build it
    NocRouterId current_router = src_router;

    int sink_router_x_position = noc_model.get_noc_router_grid_position_x(sink_router);
    int sink_router_y_position = noc_model.get_noc_router_grid_position_y(sink_router);

    /*
     * If we are not at the sink router then run another iteration of the XY routing algorithm
     * to decide which link to take and also what the next router will be in the path.
     *
     * */
    while((size_t)current_router != (size_t)src_router){

        int current_router_x_position = noc_model.get_noc_router_grid_position_x(current_router);
        int current_router_y_position = noc_model.get_noc_router_grid_position_y(current_router);

        // determine how we should path next
        RouteDirection next_step_direction = get_direction_to_travel(sink_router_x_position, sink_router_y_position, current_router_x_position, current_router_y_position);

        // Move to the next router based on the previously determined direction
        route_exists = move_to_next_router(current_router, route_link_list, next_step_direction, noc_model);

        // if we didn't find a legal router to move to then throw an error that there is no path between the source and destination routers
        if (!route_exists){
            VPR_FATAL_ERROR(VPR_ERROR_OTHER, "No route could be found from router '%d' and router '%d' using the XY-Routing algorithm.", noc_model.get_noc_router_id(src_router), noc_model.get_noc_router_id(sink_router));
        }
    }

    return;

}

// check whether x or y correspond to horizontal and vertical directions
RouteDirection XYRouting::get_direction_to_travel(int sink_router_x_position, int sink_router_y_position, int curr_router_x_position, int curr_router_y_position) {

    // initialize to an arbitrary value
    RouteDirection direction_to_travel = RouteDirection::DOWN;


    /**
     * Check the horizontal direction first. If the current x position is greater than the destination, we need to move left.
     * If the current x position is less than the destination, we need to move right. Once the current position is horizontally
     * aligned with the destination, we then need to align in the vertical direction. If the current vertical position is below
     * the destination, then we need to move up. Similarly, if the current y position is above the destination, we need to move
     * down.
     *
     * */
    if (curr_router_x_position > sink_router_x_position){
        direction_to_travel = RouteDirection::LEFT;
    }
    else if (curr_router_x_position < sink_router_x_position){
        direction_to_travel = RouteDirection::RIGHT;
    }
    else if (curr_router_y_position < sink_router_y_position){
        direction_to_travel = RouteDirection::UP;
    }
    else if (curr_router_y_position > sink_router_y_position){
        direction_to_travel = RouteDirection::DOWN;
    }

    return direction_to_travel;

}

bool XYRouting::move_to_next_router(NocRouterId& current_router, std::vector<NocLinkId>& route_link_list, RouteDirection next_step_direction, const NocStorage& noc_model) {

    // Initially set to a non-existent router id
    NocRouterId next_router(-1);

    // keeps track of whether a router was found that we can move to
    bool found_next_router = false;

    int current_router_x_position = noc_model.get_noc_router_grid_position_x(current_router);
    int current_router_y_position = noc_model.get_noc_router_grid_position_y(current_router);

    std::vector<NocLinkId> router_connections = noc_model.get_noc_router_connections(current_router);

    for (auto connecting_link = router_connections.begin(); connecting_link != router_connections.end(); connecting_link++){

        next_router = noc_model.get_noc_link_sink_router(*connecting_link);

        int next_router_x_position = noc_model.get_noc_router_grid_position_x(next_router);
        int next_router_y_position = noc_model.get_noc_router_grid_position_y(next_router);

        switch (next_step_direction) {
            case RouteDirection::LEFT:
                if (next_router_x_position < current_router_x_position){
                    found_next_router = true;
                }
                break;
            case RouteDirection::RIGHT:
                if (next_router_x_position > current_router_x_position){
                    found_next_router = true;
                }
                break;
            case RouteDirection::UP:
                if (next_router_y_position > current_router_y_position){
                    found_next_router = true;
                }
                break;
            case RouteDirection::DOWN:
                if (next_router_y_position < current_router_y_position){
                    found_next_router = true;
                }
                break;
            default:
                break;
        }

        if (found_next_router){
            route_link_list.push_back(*connecting_link);
            current_router = next_router;
            break;
        }
    }

    return found_next_router;
}
