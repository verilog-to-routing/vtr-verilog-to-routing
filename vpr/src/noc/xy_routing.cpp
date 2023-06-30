
#include <vector>

#include "xy_routing.h"
#include "globals.h"
#include "vpr_error.h"

XYRouting::~XYRouting() {}

void XYRouting::route_flow(NocRouterId src_router_id, NocRouterId sink_router_id, std::vector<NocLinkId>& flow_route, const NocStorage& noc_model) {
    // keep track of whether a legal route exists between the two routers
    bool route_exists = true;

    // get the source router
    const NocRouter& src_router = noc_model.get_single_noc_router(src_router_id);

    // keep track of the last router in the route as we build it. Initially we are at the start router, so that will be the current router
    NocRouterId curr_router_id = src_router_id;

    // get the sink router
    const NocRouter& sink_router = noc_model.get_single_noc_router(sink_router_id);

    // get the position of the sink router
    int sink_router_x_position = sink_router.get_router_grid_position_x();
    int sink_router_y_position = sink_router.get_router_grid_position_y();

    /**
     * Keeps track of which routers have been reached already
     * while traversing the NoC. This variable will help determine
     * cases where a route could not be found and the algorithm is
     * stuck going back and forth between routers it has already
     * visited.
     * 
     */
    std::unordered_set<NocRouterId> visited_routers;

    // clear any previously stored route before finding the current route
    flow_route.clear();

    /*
     * If we are not at the sink router then run another iteration of the XY routing algorithm
     * to decide which link to take and also what the next router will be in the path.
     *
     * */
    while (curr_router_id != sink_router_id) {
        // store the router that is currently visited at each iteration of the algorithm
        const NocRouter& curr_router = noc_model.get_single_noc_router(curr_router_id);

        // get the position of the current router
        int curr_router_x_position = curr_router.get_router_grid_position_x();
        int curr_router_y_position = curr_router.get_router_grid_position_y();

        RouteDirection next_step_direction = get_direction_to_travel(sink_router_x_position, sink_router_y_position, curr_router_x_position, curr_router_y_position);

        // Move to the next router based on the previously determined direction
        route_exists = move_to_next_router(curr_router_id, curr_router_x_position, curr_router_y_position, next_step_direction, flow_route, visited_routers, noc_model);

        // if we didn't find a legal router to move to then throw an error that there is no path between the source and destination routers
        if (!route_exists) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER, "No route could be found from starting router with ID:'%d' and the destination router with ID:'%d' using the XY-Routing algorithm.", src_router.get_router_user_id(), sink_router.get_router_user_id());
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
    if (curr_router_x_position > sink_router_x_position) {
        direction_to_travel = RouteDirection::LEFT;
    } else if (curr_router_x_position < sink_router_x_position) {
        direction_to_travel = RouteDirection::RIGHT;
    } else if (curr_router_y_position < sink_router_y_position) {
        direction_to_travel = RouteDirection::UP;
    } else if (curr_router_y_position > sink_router_y_position) {
        direction_to_travel = RouteDirection::DOWN;
    } else { //case where the current router and the destination router are on the same location
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "XY router cannot determine direction to travel since the current router is already at the destination.");
    }

    return direction_to_travel;
}

bool XYRouting::move_to_next_router(NocRouterId& curr_router_id, int curr_router_x_position, int curr_router_y_position, RouteDirection next_step_direction, std::vector<NocLinkId>& flow_route, std::unordered_set<NocRouterId>& visited_routers, const NocStorage& noc_model) {
    // represents the router that will be visited when taking an outgoing link
    NocRouterId next_router_id(-1);

    // keeps track of whether a router was found that we can move to
    bool found_next_router = false;

    // When an acceptable link is found, this variable keeps track of whether the next router visited using the link was already visited or not.
    bool visited_next_router = false;

    // get all the outgoing links for the current router
    const std::vector<NocLinkId>& router_connections = noc_model.get_noc_router_connections(curr_router_id);

    // go through each outgoing link and determine whether the link leads towards the intended route direction
    for (auto connecting_link : router_connections) {
        // get the current outgoing link which is being processed
        const NocLink& curr_outgoing_link = noc_model.get_single_noc_link(connecting_link);

        // get the next router that we will visit if we travel across the current link
        next_router_id = curr_outgoing_link.get_sink_router();
        const NocRouter& next_router = noc_model.get_single_noc_router(next_router_id);

        // get the coordinates of the next router
        int next_router_x_position = next_router.get_router_grid_position_x();
        int next_router_y_position = next_router.get_router_grid_position_y();

        /*
         * Using the position of the next router we will visit if we take the current link, determine if the travel direction through the link matches the direction the algorithm determined we must travel in. If the directions do not match, then this link is not valid.
         */
        switch (next_step_direction) {
            case RouteDirection::LEFT:
                if (next_router_x_position < curr_router_x_position) {
                    found_next_router = true;
                }
                break;
            case RouteDirection::RIGHT:
                if (next_router_x_position > curr_router_x_position) {
                    found_next_router = true;
                }
                break;
            case RouteDirection::UP:
                if (next_router_y_position > curr_router_y_position) {
                    found_next_router = true;
                }
                break;
            case RouteDirection::DOWN:
                if (next_router_y_position < curr_router_y_position) {
                    found_next_router = true;
                }
                break;
            default:
                break;
        }
        // check whether the next router we will visit was already visited
        if (visited_routers.find(next_router_id) != visited_routers.end()) {
            visited_next_router = true;
        }

        // check if the current link was acceptable. If it is, then make sure that the next router was not previously visited.
        // If the next router was already visited, then this link is not valid, so indicate this and move onto processing the next link.
        if (found_next_router && !visited_next_router) {
            // if we are here then the link is legal to traverse, so add it to the found route and traverse the link by moving to the router connected by this link
            flow_route.push_back(connecting_link);
            curr_router_id = next_router_id;

            // we found a suitable router to visit next, so add it to the set of visited routers
            visited_routers.insert(next_router_id);

            break;
        } else {
            found_next_router = false;
            visited_next_router = false;
        }
    }

    return found_next_router;
}
