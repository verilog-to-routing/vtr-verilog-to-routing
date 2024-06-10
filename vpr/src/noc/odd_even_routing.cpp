
#include "odd_even_routing.h"
#include "globals.h"
#include "move_utils.h"

OddEvenRouting::~OddEvenRouting() = default;

const std::vector<TurnModelRouting::Direction>& OddEvenRouting::get_legal_directions(NocRouterId src_router_id,
                                                                                     NocRouterId curr_router_id,
                                                                                     NocRouterId dst_router_id,
                                                                                     const NocStorage& noc_model) {
    // used to access NoC compressed grid
    auto& place_ctx = g_vpr_ctx.placement();
    // used to get NoC logical block type
    auto& cluster_ctx = g_vpr_ctx.clustering();
    // used to get the clustered block ID of a NoC router
    auto& noc_ctx = g_vpr_ctx.noc();
    // get number of layers
    const int num_layers = g_vpr_ctx.device().grid.get_num_layers();

    // Get the logical block type for router
    const auto router_block_type = cluster_ctx.clb_nlist.block_type(noc_ctx.noc_traffic_flows_storage.get_router_clusters_in_netlist()[0]);

    // Get the compressed grid for NoC
    const auto& compressed_noc_grid = place_ctx.compressed_block_grids[router_block_type->index];

    // get source, current, and destination NoC routers
    const auto& src_router = noc_model.get_single_noc_router(src_router_id);
    const auto& curr_router = noc_model.get_single_noc_router(curr_router_id);
    const auto& dst_router = noc_model.get_single_noc_router(dst_router_id);

    // get the position of source, current, and destination NoC routers
    const auto src_router_pos = src_router.get_router_physical_location();
    const auto curr_router_pos = curr_router.get_router_physical_location();
    const auto dst_router_pos = dst_router.get_router_physical_location();

    /* get the compressed location for source, current, and destination NoC routers
     * Odd-even routing algorithm restricts turn based on whether the current NoC router
     * in an odd or even NoC column. This information can be extracted from the NoC compressed grid.
     */
    auto compressed_src_loc = get_compressed_loc_approx(compressed_noc_grid, t_pl_loc{src_router_pos, 0}, num_layers)[src_router_pos.layer_num];
    auto compressed_curr_loc = get_compressed_loc_approx(compressed_noc_grid, t_pl_loc{curr_router_pos, 0}, num_layers)[curr_router_pos.layer_num];
    auto compressed_dst_loc = get_compressed_loc_approx(compressed_noc_grid, t_pl_loc{dst_router_pos, 0}, num_layers)[dst_router_pos.layer_num];

    // clear returned legal directions from the previous call
    returned_legal_direction.clear();

    // calculate the distance between the current router and the destination
    const int diff_x = compressed_dst_loc.x - compressed_curr_loc.x;
    const int diff_y = compressed_dst_loc.y - compressed_curr_loc.y;

    /* The implementation below is a carbon copy of the Fig. 2 in the following paper
     * Chiu GM. The odd-even turn model for adaptive routing.
     * IEEE Transactions on parallel and distributed systems. 2000 Jul;11(7):729-38.
     * In summary, the odd-even algorithm forbids NW and SW turns in odd columns,
     * while EN and ES turns are forbidden in even columns.
     */
    if (diff_x == 0) { // the same column as the destination. Only north or south are allowed
        if (diff_y > 0) {
            returned_legal_direction.push_back(TurnModelRouting::Direction::UP);
        } else {
            returned_legal_direction.push_back(TurnModelRouting::Direction::DOWN);
        }
    } else { // currently in a different column than the destination
        if (diff_x > 0) { // eastbound message
            if (diff_y == 0) { // already in the same row as the destination. Just move to the east
                returned_legal_direction.push_back(TurnModelRouting::Direction::RIGHT);
            } else {
                /* Since EN and ES turns are forbidden in even columns, we move along the vertical
                 * direction only in we are in an odd column. */
                if (is_odd(compressed_curr_loc.x) || compressed_curr_loc.x == compressed_src_loc.x) {
                    if (diff_y > 0) {
                        returned_legal_direction.push_back(TurnModelRouting::Direction::UP);
                    } else {
                        returned_legal_direction.push_back(TurnModelRouting::Direction::DOWN);
                    }
                }
                // the destination column is odd and there are more than 1 column left to destination
                if (is_odd(compressed_dst_loc.x) || diff_x != 1) {
                    returned_legal_direction.push_back(TurnModelRouting::Direction::RIGHT);
                }
            }
        } else { // westbound message
            returned_legal_direction.push_back(TurnModelRouting::Direction::LEFT);
            /* Since NW and SW turns are forbidden in odd columns, we allow
             * moving along vertical axis only in even columns */
            if (is_even(compressed_curr_loc.x)) {
                if (diff_y > 0) {
                    returned_legal_direction.push_back(TurnModelRouting::Direction::UP);
                } else {
                    returned_legal_direction.push_back(TurnModelRouting::Direction::DOWN);
                }
            }
        }
    }

    return returned_legal_direction;
}

TurnModelRouting::Direction OddEvenRouting::select_next_direction(const std::vector<TurnModelRouting::Direction>& legal_directions,
                                                                  NocRouterId src_router_id,
                                                                  NocRouterId dst_router_id,
                                                                  NocRouterId curr_router_id,
                                                                  NocTrafficFlowId traffic_flow_id,
                                                                  const NocStorage& noc_model) {
    // if there is only one legal direction, take it
    if (legal_directions.size() == 1) {
        return legal_directions[0];
    }

    // get current and destination NoC routers
    const auto& curr_router = noc_model.get_single_noc_router(curr_router_id);
    const auto& dst_router = noc_model.get_single_noc_router(dst_router_id);

    // get the position of current and destination NoC routers
    const auto curr_router_pos = curr_router.get_router_physical_location();
    const auto dst_router_pos = dst_router.get_router_physical_location();

    /* get_legal_directions() proposes legal directions for minimal routing.
     * Therefore, if the function reaches this point, there are two available
     * directions: a vertical (N/S) and a horizontal (W/E) direction.
     * The direction is chosen based on the distance to the destination
     * in each co-ordinate.
     */

    // compute the hash value
    uint32_t hash_val = get_hash_value(src_router_id, dst_router_id, curr_router_id, traffic_flow_id);
    // get the maximum value that can be represented by size_t
    const uint32_t max_uint32_t_val = std::numeric_limits<uint32_t>::max();

    // get the distance from the current router to the destination in each coordination
    int delta_x = abs(dst_router_pos.x - curr_router_pos.x);
    int delta_y = abs(dst_router_pos.y - curr_router_pos.y);

    // compute the probability of choosing the vertical direction
    uint32_t vertical_probability = delta_y * (max_uint32_t_val / (delta_x + delta_y));

    TurnModelRouting::Direction selected_direction = TurnModelRouting::Direction::INVALID;

    // choose whether move vertically or horizontally
    if (hash_val < vertical_probability) {
        selected_direction = select_vertical_direction(legal_directions);
    } else {
        selected_direction = select_horizontal_direction(legal_directions);
    }

    return selected_direction;
}

bool OddEvenRouting::is_odd(int number) {
    return (number % 2) == 1;
}

bool OddEvenRouting::is_even(int number) {
    return (number % 2) == 0;
}

bool OddEvenRouting::is_turn_legal(const std::array<std::reference_wrapper<const NocRouter>, 3>& noc_routers) const {
    // used to access NoC compressed grid
    const auto& place_ctx = g_vpr_ctx.placement();
    // used to get NoC logical block type
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    // used to get the clustered block ID of a NoC router
    auto& noc_ctx = g_vpr_ctx.noc();
    // get number of layers
    const int num_layers = g_vpr_ctx.device().grid.get_num_layers();

    const int x1 = noc_routers[0].get().get_router_grid_position_x();
    const int y1 = noc_routers[0].get().get_router_grid_position_y();

    const int x2 = noc_routers[1].get().get_router_grid_position_x();
    const int y2 = noc_routers[1].get().get_router_grid_position_y();

    const int x3 = noc_routers[2].get().get_router_grid_position_x();
    const int y3 = noc_routers[2].get().get_router_grid_position_y();

    // check if the given routers can be traversed one after another
    VTR_ASSERT(x2 == x1 || y2 == y1);
    VTR_ASSERT(x3 == x2 || y3 == y2);

    // get the position of the second NoC routers
    const auto router2_pos = noc_routers[1].get().get_router_physical_location();

    // Get the logical block type for router
    const auto router_block_type = cluster_ctx.clb_nlist.block_type(noc_ctx.noc_traffic_flows_storage.get_router_clusters_in_netlist()[0]);

    // Get the compressed grid for NoC
    const auto& compressed_noc_grid = place_ctx.compressed_block_grids[router_block_type->index];

    // get the compressed location of the second NoC router
    auto compressed_2_loc = get_compressed_loc(compressed_noc_grid, t_pl_loc{router2_pos, 0}, num_layers)[router2_pos.layer_num];

    // going back to the first router is not allowed
    if (x1 == x3 && y1 == y3) {
        return false;
    }

    // check if the turn is compatible with odd-even routing algorithm turn restrictions
    if (is_odd(compressed_2_loc.x)) {
        if (y2 != y1 && x3 < x2) {
            return false;
        }
    } else { // even column
        if (x2 > x1 && y2 != y3) {
            return false;
        }
    }

    return true;
}










