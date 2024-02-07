
#include "negative_first_routing.h"

NegativeFirstRouting::~NegativeFirstRouting() = default;
    
const std::vector<TurnModelRouting::Direction>& NegativeFirstRouting::get_legal_directions(NocRouterId curr_router_id,
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

    /* In negative-first algorithm, the packet first moved in negative directions
     * (west and south), then it moves toward positive ones (north and east). Once
     * the packet took a single step toward positive directions, it can no longer
     * move in negative directions. For minimal routing, this means that we first
     * need to check whether moving in negative directions keeps us on a minimal route.
     * If neither west nor south moves us closer to the destination, we can try moving
     * toward north and east.
     */

    // check whether moving west keeps us on a minimal route
    if (dst_router_pos.x < curr_router_pos.x) {
        returned_legal_direction.push_back(TurnModelRouting::Direction::LEFT);
    }

    // check whether moving south keeps us on a minimal route
    if (dst_router_pos.y < curr_router_pos.y) {
        returned_legal_direction.push_back(TurnModelRouting::Direction::DOWN);
    }

    // if at least one of the negative directions is legal,
    // we don't need to check the positive ones and can return
    if (!returned_legal_direction.empty()) {
        return returned_legal_direction;
    }

    /* If we reach this point in the function, it means that
     * none of negative directions were legal, therefore we need to
     * check the positive ones.
     */

    // check whether moving east keeps us on a minimal route
    if (dst_router_pos.x > curr_router_pos.x) {
        returned_legal_direction.push_back(TurnModelRouting::Direction::RIGHT);
    }

    // check whether moving north keeps us on a minimal route
    if (dst_router_pos.y > curr_router_pos.y) {
        returned_legal_direction.push_back(TurnModelRouting::Direction::UP);
    }

    return returned_legal_direction;
}

TurnModelRouting::Direction NegativeFirstRouting::select_next_direction(const std::vector<TurnModelRouting::Direction>& legal_directions,
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
     * the destination routed is located at SW or NE of
     * the current router. We adopt a similar approach to
     * WestFirstRouting to select between south/north and west/east.
     */

    // compute the hash value
    uint32_t hash_val = get_hash_value(src_router_id, dst_router_id, curr_router_id, traffic_flow_id);
    // get the maximum value that can be represented by size_t
    const uint32_t max_uint32_t_val = std::numeric_limits<uint32_t>::max();

    // get the distance from the current router to the destination in each coordination
    int delta_x = abs(dst_router_pos.x - curr_router_pos.x);
    int delta_y = abs(dst_router_pos.y - curr_router_pos.y);

    // compute the probability of going to north/south direction
    uint32_t ns_probability = delta_y * (max_uint32_t_val / (delta_x + delta_y));

    if (hash_val < ns_probability) {
        for (const auto& direction : legal_directions) {
            if (direction == TurnModelRouting::Direction::DOWN || direction == TurnModelRouting::Direction::UP) {
                return direction;
            }
        }
    } else {
        for (const auto& direction : legal_directions) {
            if (direction == TurnModelRouting::Direction::RIGHT || direction == TurnModelRouting::Direction::LEFT) {
                return direction;
            }
        }
    }

    return TurnModelRouting::Direction::INVALID;
}