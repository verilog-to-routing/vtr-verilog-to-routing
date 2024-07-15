
#include <vector>

#include "xy_routing.h"
#include "vtr_util.h"

XYRouting::~XYRouting() = default;

const std::vector<TurnModelRouting::Direction>& XYRouting::get_legal_directions(NocRouterId /*src_router_id*/,
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

    /* In XY-routing, we first move along the X-axis until
     * the current router has the same x-coordinate as the
     * destination. Then we start moving along the y-axis.
     * Finally, we move along the z-axis.
    */

    if (dst_router_pos.x > curr_router_pos.x) {
        return east_direction;
    } else if (dst_router_pos.x < curr_router_pos.x) {
        return west_direction;
    } else if (dst_router_pos.y > curr_router_pos.y) {
        return north_direction;
    } else if (dst_router_pos.y < curr_router_pos.y) {
        return south_direction;
    } else if (dst_router_pos.layer_num > curr_router_pos.layer_num) {
        return up_direction;
    } else if (dst_router_pos.layer_num < curr_router_pos.layer_num) {
        return down_direction;
    } else {
        return no_direction;
    }
}

TurnModelRouting::Direction XYRouting::select_next_direction(const std::vector<TurnModelRouting::Direction>& legal_directions,
                                                             NocRouterId /*src_router_id*/,
                                                             NocRouterId /*dst_router_id*/,
                                                             NocRouterId /*curr_router_id*/,
                                                             NocTrafficFlowId /*traffic_flow_id*/,
                                                             const NocStorage& /*noc_model*/) {

    if (legal_directions.size() == 1) {
        return legal_directions[0];
    }

    return TurnModelRouting::Direction::INVALID;
}

bool XYRouting::is_turn_legal(const std::array<std::reference_wrapper<const NocRouter>, 3>& noc_routers,
                              const NocStorage& /*noc_model*/) const {
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

    // if the first move is vertical, the second one can't be horizontal
    if (y1 != y2 && x2 != x3) {
        return false;
    }

    // if the first move is along z axis, the second move can't be along x or y axes
    if (z1 != z2 && (x2 != x3 || y2 != y3)) {
        return false;
    }

    return true;
}