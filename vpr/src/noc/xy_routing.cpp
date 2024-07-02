
#include <vector>

#include "xy_routing.h"

XYRouting::~XYRouting() = default;

const std::vector<TurnModelRouting::Direction>& XYRouting::get_legal_directions(NocRouterId /*src_router_id*/,
                                                                                NocRouterId curr_router_id,
                                                                                NocRouterId dst_router_id,
                                                                                const NocStorage& noc_model) {
    // get current and destination NoC routers
    const auto& curr_router = noc_model.get_single_noc_router(curr_router_id);
    const auto& dst_router = noc_model.get_single_noc_router(dst_router_id);

    // get the position of current and destination NoC routers
    const auto curr_router_pos = curr_router.get_router_physical_location();
    const auto dst_router_pos = dst_router.get_router_physical_location();

    /* In XY-routing, we first move along the X-axis until
     * the current router has the same x-coordinate as the
     * destination. Then we start moving along the y-axis.
    */
    if (curr_router_pos.x != dst_router_pos.x) {
        return x_axis_directions;
    } else {
        return y_axis_directions;
    }
}

TurnModelRouting::Direction XYRouting::select_next_direction(const std::vector<TurnModelRouting::Direction>& legal_directions,
                                                             NocRouterId /*src_router_id*/,
                                                             NocRouterId dst_router_id,
                                                             NocRouterId curr_router_id,
                                                             NocTrafficFlowId /*traffic_flow_id*/,
                                                             const NocStorage& noc_model) {
    // get current and destination NoC routers
    const auto& curr_router = noc_model.get_single_noc_router(curr_router_id);
    const auto& dst_router = noc_model.get_single_noc_router(dst_router_id);

    // get the position of current and destination NoC routers
    const auto curr_router_pos = curr_router.get_router_physical_location();
    const auto dst_router_pos = dst_router.get_router_physical_location();

    // indicates whether the next direction that moves us
    // closer to the destination router is found among
    // legal directions
    bool found_next_direction = false;

    // Iterate over legal directions and find the one that moves us closer to the destination
    for (const auto& direction : legal_directions) {
        switch (direction) {
            case TurnModelRouting::Direction::LEFT:
                if (dst_router_pos.x < curr_router_pos.x) {
                    found_next_direction = true;
                }
                break;
            case TurnModelRouting::Direction::RIGHT:
                if (dst_router_pos.x > curr_router_pos.x) {
                    found_next_direction = true;
                }
                break;
            case TurnModelRouting::Direction::UP:
                if (dst_router_pos.y > curr_router_pos.y) {
                    found_next_direction = true;
                }
                break;
            case TurnModelRouting::Direction::DOWN:
                if (dst_router_pos.y < curr_router_pos.y) {
                    found_next_direction = true;
                }
                break;
            default:
                break;
        }

        if (found_next_direction) {
            return direction;
        }
    }

    return TurnModelRouting::Direction::INVALID;
}

bool XYRouting::is_turn_legal(const std::array<std::reference_wrapper<const NocRouter>, 3>& noc_routers) const {
    const int x1 = noc_routers[0].get().get_router_grid_position_x();
    const int y1 = noc_routers[0].get().get_router_grid_position_y();

    const int x2 = noc_routers[1].get().get_router_grid_position_x();
    const int y2 = noc_routers[1].get().get_router_grid_position_y();

    const int x3 = noc_routers[2].get().get_router_grid_position_x();
    const int y3 = noc_routers[2].get().get_router_grid_position_y();

    // check if the given routers can be traversed one after another
    VTR_ASSERT(x2 == x1 || y2 == y1);
    VTR_ASSERT(x3 == x2 || y3 == y2);

    // going back to the first router is not allowed
    if (x1 == x3 && y1 == y3) {
        return false;
    }

    // if the first move is vertical, the second one can't be horizontal
    if (y1 != y2 && x2 != x3) {
        return false;
    }

    return true;
}