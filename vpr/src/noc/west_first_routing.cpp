#include "west_first_routing.h"

WestFirstRouting::WestFirstRouting(const NocStorage& noc_model, const std::optional<std::reference_wrapper<const NocVirtualBlockStorage>>& noc_virtual_blocks)
    : TurnModelRouting(noc_model, noc_virtual_blocks) {
}

WestFirstRouting::~WestFirstRouting() = default;

const std::vector<TurnModelRouting::Direction>& WestFirstRouting::get_legal_directions(NocRouterId /*src_router_id*/,
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

bool WestFirstRouting::routability_early_check(NocRouterId src_router_id, NocRouterId virt_router_id, NocRouterId dst_router_id) {
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

    /*
     * In the west-first algorithm, if the virtual block is to the west
     * of the source router, we must move westward until we reach the
     * same column as the virtual block. If the destination is to the
     * west of the virtual block, we need to move westward again.
     * However, we need to ensure that reaching the virtual block
     * didn't require moving south or north, meaning the virtual block
     * was at the same row as the source router.
     */

    if (virt_router_pos.x < src_router_pos.x) { // virtual block is to the west of the source router
        if (dst_router_pos.x < virt_router_pos.x) { // destination is to the west of the virtual block
            if (virt_router_pos.y == src_router_pos.y) { // we haven't taken any directions other than west
                return true;
            } else { // we needed to take north/south to reach the virtual block
                return false;
            }
        }
    } else if (virt_router_pos != src_router_pos) {
        // virtual block is neither to the west of source nor at the same location
        // therefore we don't move westward to reach the virtual block
        // since west is not the first direction to be taken, it is not allowed to
        // be selected after reaching the virtual block

        // we need to move westward but it is not the first direction to be chosen
        if (dst_router_pos.x < virt_router_pos.x) {
            return false;
        }
    }

    return true;
}
