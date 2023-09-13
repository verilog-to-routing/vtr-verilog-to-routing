
#include "north_last_routing.h"

NorthLastRouting::~NorthLastRouting() = default;

NorthLastRouting::NorthLastRouting(const NocStorage& noc_model, const std::optional<std::reference_wrapper<const NocVirtualBlockStorage>>& noc_virtual_blocks)
    : TurnModelRouting(noc_model, noc_virtual_blocks) {
}

const std::vector<TurnModelRouting::Direction>& NorthLastRouting::get_legal_directions(NocRouterId /*src_router_id*/,
                                                                                       NocRouterId curr_router_id,
                                                                                       NocRouterId dst_router_id) {
    // get current and destination NoC routers
    const auto& curr_router = noc_model_.get_single_noc_router(curr_router_id);
    const auto& dst_router = noc_model_.get_single_noc_router(dst_router_id);

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
                                                                    NocTrafficFlowId traffic_flow_id) {
    // get current and destination NoC routers
    const auto& curr_router = noc_model_.get_single_noc_router(curr_router_id);
    const auto& dst_router = noc_model_.get_single_noc_router(dst_router_id);

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

bool NorthLastRouting::routability_early_check(NocRouterId src_router_id, NocRouterId virt_router_id, NocRouterId dst_router_id) {
    // if virtual blocks are not enabled, each (src, dst) pair are routable
    if (!noc_virtual_blocks_) {
        return true;
    }

    // get source, virtual and destination NoC routers
    const auto& src_router = noc_model_.get_single_noc_router(src_router_id);
    const auto& virt_router = noc_model_.get_single_noc_router(virt_router_id);
    const auto& dst_router = noc_model_.get_single_noc_router(dst_router_id);

    // get source, virtual, and destination router locations
    const auto src_router_pos = src_router.get_router_physical_location();
    const auto virt_router_pos = virt_router.get_router_physical_location();
    const auto dst_router_pos = dst_router.get_router_physical_location();

    /* As the name implies, in north-last algorithm, north direction, if taken,
     * is the last direction to be chosen. No turns are allowed after moving in
     * north direction. If the virtual block is located at the north of the source,
     * we need to take north as the last direction. Since we aren't allowed to
     * change direction afterward, the destination router should be in the
     * same column as the virtual block and to its north for a route to exist.
     */
    if (virt_router_pos.y > src_router_pos.y) {
        // the destination router is in the same column as the virtual block and to its north
        if (dst_router_pos.x == virt_router_pos.x && dst_router_pos.y >= virt_router_pos.y) {
            return true;
        } else { // we need to turn after taking north direction, which is forbidden
            return false;
        }
    }

    /* Reaching this point means that the virtual block is either at the same row
     * as the source or to its south. In both cases, we don't need to move towards
     * north to reach the virtual block. Therefore, the virt-->dst route can take
     * a turn towards as the last turn.
     */
    return true;
}