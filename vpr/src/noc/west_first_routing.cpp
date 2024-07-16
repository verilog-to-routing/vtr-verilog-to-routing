#include "west_first_routing.h"

WestFirstRouting::~WestFirstRouting() = default;

const std::vector<TurnModelRouting::Direction>& WestFirstRouting::get_legal_directions(NocRouterId /*src_router_id*/,
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

    /* In west-first routing, the two turns to the west are prohibited.
     * Therefore, if the destination is at the west of the current router,
     * we must travel in that direction until we reach the same x-coordinate
     * as the destination router. Otherwise, we can move south, north,
     * and east adaptively.
     */
    if (dst_router_pos.x < curr_router_pos.x) {
        returned_legal_direction.push_back(TurnModelRouting::Direction::LEFT);
    } else { // to the east or the same column
        if (dst_router_pos.x > curr_router_pos.x) { // not the same column
            returned_legal_direction.push_back(TurnModelRouting::Direction::RIGHT);
        }

        if (dst_router_pos.y > curr_router_pos.y) {
            returned_legal_direction.push_back(TurnModelRouting::Direction::UP);
        } else if (dst_router_pos.y < curr_router_pos.y) {
            returned_legal_direction.push_back(TurnModelRouting::Direction::DOWN);
        }
    }

    return returned_legal_direction;
}

TurnModelRouting::Direction WestFirstRouting::select_next_direction(const std::vector<TurnModelRouting::Direction>& legal_directions,
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

    /*
     * If the function has not already returned,
     * the destination router is either to NE or SE of
     * the current router. Therefore, we have two
     * directions to choose from. A hash value is generated
     * based on the source, current, and destination router IDs
     * along with the traffic flow ID. Then, we select one direction
     * by flipping a biased coin. For example if
     * 1) dst_router_pos.x - curr_router_pos.x = 2
     * 2) dst_router_pos.y - curr_router_pos.y = 8
     * We take the UP direction with 80% chance, and RIGHT with 20%.
     */

    // compute the hash value
    uint32_t hash_val = get_hash_value(src_router_id, dst_router_id, curr_router_id, traffic_flow_id);
    // get the maximum value that can be represented by size_t
    const uint32_t max_uint32_t_val = std::numeric_limits<uint32_t>::max();

    // get the distance from the current router to the destination in each coordination
    int delta_x = abs(dst_router_pos.x - curr_router_pos.x);
    int delta_y = abs(dst_router_pos.y - curr_router_pos.y);

    // compute the probability of going to the right direction
    uint32_t east_probability = delta_x * (max_uint32_t_val / (delta_x + delta_y));

    TurnModelRouting::Direction selected_direction = TurnModelRouting::Direction::INVALID;

    if (hash_val < east_probability) { // sometimes turn right
        selected_direction = TurnModelRouting::Direction::RIGHT;
    } else { // if turning right was rejected, take the other option (north or south)
        selected_direction = select_direction_other_than(legal_directions, TurnModelRouting::Direction::RIGHT);
    }

    return selected_direction;
}

bool WestFirstRouting::is_turn_legal(const std::array<std::reference_wrapper<const NocRouter>, 3>& noc_routers) const {
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

    /* In the west-first routing algorithm, once the traffic flow
     * moved in a vertical direction, it is no longer allowed to move
     * towards west. Therefore, if the first link was travelling in a
     * vertical direction, the second link can't move towards left.
     */
    if (y2 != y1 && x3 < x2) {
        return false;
    }

    return true;
}
