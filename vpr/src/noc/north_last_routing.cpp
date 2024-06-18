
#include "north_last_routing.h"

NorthLastRouting::~NorthLastRouting() = default;

const std::vector<TurnModelRouting::Direction>& NorthLastRouting::get_legal_directions(NocRouterId /*src_router_id*/,
                                                                                       NocRouterId curr_router_id,
                                                                                       NocRouterId dst_router_id,
                                                                                       const NocStorage& noc_model) {
    // get current and destination NoC routers
    const auto& curr_router = noc_model.get_single_noc_router(curr_router_id);
    const auto& dst_router = noc_model.get_single_noc_router(dst_router_id);

    // get the position of current and destination NoC routers
    const auto curr_router_pos = curr_router.get_router_physical_location();
    const auto dst_router_pos = dst_router.get_router_physical_location();

    // clear returned legal directions from the previous call
    returned_legal_direction.clear();

    /* In north-last routing, when we start moving in the north direction
     * we can no longer turn. Therefore, moving toward north is permissible
     * only when by keeping moving northward we arrive at the destination.
     * Therefore, moving north is legal only when we are at same column
     * as the destination and the destination router is located above
     * the current router. To find legal directions, we first check
     * whether moving east, west, or south moves us closer to the
     * destination (i.e. keeps us on a minimal route). If so, moving
     * north is illegal. Otherwise, travelling northward is the only
     * legal option.
     */

    // check if the destination is at the west/east of the current router
    if (dst_router_pos.x < curr_router_pos.x) {
        returned_legal_direction.push_back(TurnModelRouting::Direction::LEFT);
    } else if (dst_router_pos.x > curr_router_pos.x) {
        returned_legal_direction.push_back(TurnModelRouting::Direction::RIGHT);
    }

    // check if the destination router is at the south of the current router
    if (dst_router_pos.y < curr_router_pos.y) {
        returned_legal_direction.push_back(TurnModelRouting::Direction::DOWN);
    }

    // consider north only when none of other directions are legal
    if (returned_legal_direction.empty() && dst_router_pos.y > curr_router_pos.y) {
        returned_legal_direction.push_back(TurnModelRouting::Direction::UP);
    }

    return returned_legal_direction;
}

TurnModelRouting::Direction NorthLastRouting::select_next_direction(const std::vector<TurnModelRouting::Direction>& legal_directions,
                                                                    NocRouterId src_router_id,
                                                                    NocRouterId dst_router_id,
                                                                    NocRouterId curr_router_id,
                                                                    NocTrafficFlowId traffic_flow_id,
                                                                    const NocStorage& noc_model) {
    // get current and destination NoC routers
    const auto& curr_router = noc_model.get_single_noc_router(curr_router_id);
    const auto& dst_router = noc_model.get_single_noc_router(dst_router_id);

    // get the position of current and destination NoC routers
    const auto curr_router_pos = curr_router.get_router_physical_location();
    const auto dst_router_pos = dst_router.get_router_physical_location();

    // if there is only one legal direction, take it
    if (legal_directions.size() == 1) {
        return legal_directions[0];
    }

    /* If the function reaches this point,
     * the destination routed is located at SE or SW of
     * the current router. We adopt a similar approach to
     * WestFirstRouting to select between south and west/east.
     */

    // compute the hash value
    uint32_t hash_val = get_hash_value(src_router_id, dst_router_id, curr_router_id, traffic_flow_id);
    // get the maximum value that can be represented by size_t
    const uint32_t max_uint32_t_val = std::numeric_limits<uint32_t>::max();

    // get the distance from the current router to the destination in each coordination
    int delta_x = abs(dst_router_pos.x - curr_router_pos.x);
    int delta_y = abs(dst_router_pos.y - curr_router_pos.y);

    // compute the probability of going to the down (south) direction
    uint32_t south_probability = delta_y * (max_uint32_t_val / (delta_x + delta_y));

    TurnModelRouting::Direction selected_direction = TurnModelRouting::Direction::INVALID;

    if (hash_val < south_probability) { // sometimes turn south
        selected_direction = TurnModelRouting::Direction::DOWN;
    } else { // if turning south was rejected, take the other option (east/west)
        selected_direction = select_direction_other_than(legal_directions, TurnModelRouting::Direction::DOWN);
    }

    return selected_direction;
}

bool NorthLastRouting::is_turn_legal(const std::array<std::reference_wrapper<const NocRouter>, 3>& noc_routers) const {
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

    /* In the north-last algorithm, once the north direction is taken, no other
     * direction can be followed. Therefore, if the first link moves upward, the
     * second one cannot move horizontally. The case where the second link goes
     * back to the first router was checked in the previous if statement.
     */
    if (y2 > y1 && x2 != x3) {
        return false;
    }

    return true;
}
