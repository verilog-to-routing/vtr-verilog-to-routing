
#include "north_last_routing.h"

NorthLastRouting::~NorthLastRouting() = default;

const std::vector<TurnModelRouting::Direction>& NorthLastRouting::get_legal_directions(NocRouterId /*src_router_id*/,
                                                                                       NocRouterId curr_router_id,
                                                                                       NocRouterId dst_router_id,
                                                                                       TurnModelRouting::Direction /*prev_dir*/,
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
        returned_legal_direction.push_back(TurnModelRouting::Direction::WEST);
    } else if (dst_router_pos.x > curr_router_pos.x) {
        returned_legal_direction.push_back(TurnModelRouting::Direction::EAST);
    }

    // check if the destination router is at the south of the current router
    if (dst_router_pos.y < curr_router_pos.y) {
        returned_legal_direction.push_back(TurnModelRouting::Direction::SOUTH);
    }

    // check if the destination router is at the layer below the current router
    if (dst_router_pos.layer_num < curr_router_pos.layer_num) {
        returned_legal_direction.push_back(TurnModelRouting::Direction::DOWN);
    }

    // consider north and up only when none of other directions are legal
    if (returned_legal_direction.empty()) {
        if (dst_router_pos.y > curr_router_pos.y) {
            returned_legal_direction.push_back(TurnModelRouting::Direction::NORTH);
        }

        if (dst_router_pos.layer_num > curr_router_pos.layer_num) {
            returned_legal_direction.push_back(TurnModelRouting::Direction::UP);
        }
    }

    return returned_legal_direction;
}

bool NorthLastRouting::is_turn_legal(const std::array<std::reference_wrapper<const NocRouter>, 3>& noc_routers,
                                     const NocStorage& noc_model) const {
    const auto [x1, y1, z1] = noc_routers[0].get().get_router_physical_location();
    const auto [x2, y2, z2] = noc_routers[1].get().get_router_physical_location();
    const auto [x3, y3, z3] = noc_routers[2].get().get_router_physical_location();

    // check if the given routers can be traversed one after another
    VTR_ASSERT(vtr::exactly_k_conditions(2, x1 == x2, y1 == y2, z1 == z2));
    VTR_ASSERT(vtr::exactly_k_conditions(2, x2 == x3, y2 == y3, z2 == z3));


    // going back to the first router is not allowed
    if (x1 == x3 && y1 == y3 && z1 == z3) {
        return false;
    }

    // In north-last routing algorithm, these 6 90-degree turns are prohibited.
    if (noc_model.is_noc_3d()) {
        if ((z2 > z1 && x3 < x2) || (z2 > z1 && x3 > x2) || (z2 > z1 && y3 < y2) ||
            (y2 > y1 && z3 < z2) || (y2 > y1 && x3 < x2) || (y2 > y1 && x3 > x2)) {
            return false;
        }
    } else {
        if ((y2 > y1 && x3 > x2) || (y2 > y1 && x3 < x2)) {
            return false;
        }
    }


    return true;
}
