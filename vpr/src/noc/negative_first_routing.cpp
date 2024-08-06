
#include "negative_first_routing.h"

NegativeFirstRouting::~NegativeFirstRouting() = default;

const std::vector<TurnModelRouting::Direction>& NegativeFirstRouting::get_legal_directions(NocRouterId /*src_router_id*/,
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
        returned_legal_direction.push_back(TurnModelRouting::Direction::WEST);
    }

    // check whether moving south keeps us on a minimal route
    if (dst_router_pos.y < curr_router_pos.y) {
        returned_legal_direction.push_back(TurnModelRouting::Direction::SOUTH);
    }

    // check whether moving below keeps us on a minimal route
    if (dst_router_pos.layer_num < curr_router_pos.layer_num) {
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
        returned_legal_direction.push_back(TurnModelRouting::Direction::EAST);
    }

    // check whether moving north keeps us on a minimal route
    if (dst_router_pos.y > curr_router_pos.y) {
        returned_legal_direction.push_back(TurnModelRouting::Direction::NORTH);
    }

    // check whether moving above keeps us on a minimal route
    if (dst_router_pos.layer_num > curr_router_pos.layer_num) {
        returned_legal_direction.push_back(TurnModelRouting::Direction::UP);
    }

    return returned_legal_direction;
}

bool NegativeFirstRouting::is_turn_legal(const std::array<std::reference_wrapper<const NocRouter>, 3>& noc_routers,
                                         const NocStorage& noc_model) const {
    const auto[x1, y1, z1] = noc_routers[0].get().get_router_physical_location();
    const auto[x2, y2, z2] = noc_routers[1].get().get_router_physical_location();
    const auto[x3, y3, z3] = noc_routers[2].get().get_router_physical_location();

    // check if the given routers can be traversed one after another
    VTR_ASSERT(vtr::exactly_k_conditions(2, x1 == x2, y1 == y2, z1 == z2));
    VTR_ASSERT(vtr::exactly_k_conditions(2, x2 == x3, y2 == y3, z2 == z3));

    // going back to the first router is not allowed
    if (x1 == x3 && y1 == y3 && z1 == z3) {
        return false;
    }

    // In negative-first routing algorithm, these 6 90-degree turns are prohibited.
    if (noc_model.is_noc_3d()) {
        if ((x2 > x1 && y3 < y2) || (y2 > y1 && x3 < x2) || (z2 > z1 && x3 < x2) ||
            (x2 > x1 && z3 < z2) || (z2 > z1 && y3 < y2) || (y2 > y1 && z3 < z2)) {
            return false;
        }
    } else {
        if ((x2 > x1 && y3 < y2) || (y2 > y1 && x3 < x2)) {
            return false;
        }
    }

    return true;
}
