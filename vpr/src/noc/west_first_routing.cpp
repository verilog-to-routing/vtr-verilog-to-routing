#include "west_first_routing.h"

WestFirstRouting::~WestFirstRouting() = default;

const std::vector<TurnModelRouting::Direction>& WestFirstRouting::get_legal_directions(NocRouterId /*src_router_id*/,
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

    /* In west-first routing, the two turns to the west are prohibited.
     * Therefore, if the destination is at the west of the current router,
     * we must travel in that direction until we reach the same x-coordinate
     * as the destination router. Otherwise, we can move south, north,
     * and east adaptively.
     */
    if (noc_model.is_noc_3d()) {
        if (dst_router_pos.x < curr_router_pos.x) {
            returned_legal_direction.push_back(TurnModelRouting::Direction::WEST);
        }

        if (dst_router_pos.y < curr_router_pos.y) {
            returned_legal_direction.push_back(TurnModelRouting::Direction::SOUTH);
        }

        if (returned_legal_direction.empty()) {
            if (dst_router_pos.x > curr_router_pos.x) {
                returned_legal_direction.push_back(TurnModelRouting::Direction::EAST);
            }

            if (dst_router_pos.y > curr_router_pos.y) {
                returned_legal_direction.push_back(TurnModelRouting::Direction::NORTH);
            }

            if (dst_router_pos.layer_num > curr_router_pos.layer_num) {
                returned_legal_direction.push_back(TurnModelRouting::Direction::UP);
            } else if (dst_router_pos.layer_num < curr_router_pos.layer_num) {
                returned_legal_direction.push_back(TurnModelRouting::Direction::DOWN);
            }
        }
    } else {    // 2D NoC
        if (dst_router_pos.x < curr_router_pos.x) {
            returned_legal_direction.push_back(TurnModelRouting::Direction::WEST);
        } else { // to the east or the same column
            if (dst_router_pos.x > curr_router_pos.x) { // not the same column
                returned_legal_direction.push_back(TurnModelRouting::Direction::EAST);
            }

            if (dst_router_pos.y > curr_router_pos.y) {
                returned_legal_direction.push_back(TurnModelRouting::Direction::NORTH);
            } else if (dst_router_pos.y < curr_router_pos.y) {
                returned_legal_direction.push_back(TurnModelRouting::Direction::SOUTH);
            }
        }
    }

    return returned_legal_direction;
}

bool WestFirstRouting::is_turn_legal(const std::array<std::reference_wrapper<const NocRouter>, 3>& noc_routers,
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


    if (noc_model.is_noc_3d()) {
        if ((z2 > z1 && x3 < x2) || (z2 < z1 && x3 < x2) || (z2 > z1 && y3 < y2) ||
            (z2 < z1 && y3 < y2) || (y2 > y1 && x3 < x2) || (x2 > x1 && y3 > y2)) {
            return false;
        }
    } else {    // 2D NoC
        /* In the west-first routing algorithm, once the traffic flow
         * moved in a vertical direction, it is no longer allowed to move
         * towards west. Therefore, if the first link was travelling in a
         * vertical direction, the second link can't move towards left.
         */
        if ((y2 > y1 && x3 < x2) || (y2 < y1 && x3 < x2)) {
            return false;
        }
    }


    return true;
}
