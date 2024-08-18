
#include "odd_even_routing.h"

#include "move_utils.h"

#include <set>
#include <map>

OddEvenRouting::OddEvenRouting(const NocStorage& noc_model) {
    // used to store unique x, y, and layer values in ascending order
    std::set<int> unique_x, unique_y, unique_z;

    for (const NocRouter& router : noc_model.get_noc_routers()) {
        t_physical_tile_loc loc = router.get_router_physical_location();
        unique_x.insert(loc.x);
        unique_y.insert(loc.y);
        unique_z.insert(loc.layer_num);
    }

    // maps values in unique_vals (stored in ascending order) to consecutive integer values starting at 0
    auto create_compressed_map = [](const std::set<int>& unique_vals) -> std::map<int, int> {
        std::map<int, int> compressed_map;
        int compressed_val = 0;
        for (int val : unique_vals) {
            compressed_map[val] = compressed_val++;
        }

        return compressed_map;
    };

    std::map<int, int> compressed_x_map = create_compressed_map(unique_x);
    std::map<int, int> compressed_y_map = create_compressed_map(unique_y);
    std::map<int, int> compressed_z_map = create_compressed_map(unique_z);

    compressed_noc_locs_.resize(noc_model.get_number_of_noc_routers());

    for (const auto& [id, router] : noc_model.get_noc_routers().pairs()) {
        t_physical_tile_loc loc = router.get_router_physical_location();

        t_physical_tile_loc compressed_loc{compressed_x_map[loc.x],
                                           compressed_y_map[loc.y],
                                           compressed_z_map[loc.layer_num]};

        compressed_noc_locs_[id] = compressed_loc;
    }
}

OddEvenRouting::~OddEvenRouting() = default;

const std::vector<TurnModelRouting::Direction>& OddEvenRouting::get_legal_directions(NocRouterId src_router_id,
                                                                                     NocRouterId curr_router_id,
                                                                                     NocRouterId dst_router_id,
                                                                                     TurnModelRouting::Direction prev_dir,
                                                                                     const NocStorage& noc_model) {
    /* get the compressed location for source, current, and destination NoC routers
     * Odd-even routing algorithm restricts turn based on whether the current NoC router
     * in an odd or even NoC column. This information can be extracted from the NoC compressed grid.
     */
    auto compressed_src_loc = compressed_noc_locs_[src_router_id];
    auto compressed_curr_loc = compressed_noc_locs_[curr_router_id];
    auto compressed_dst_loc = compressed_noc_locs_[dst_router_id];

    // clear returned legal directions from the previous call
    returned_legal_direction.clear();

    if (noc_model.is_noc_3d()) {
        determine_legal_directions_3d(compressed_src_loc, compressed_curr_loc, compressed_dst_loc, prev_dir);
    } else {    // 2D NoC
        determine_legal_directions_2d(compressed_src_loc, compressed_curr_loc, compressed_dst_loc, prev_dir);
    }

    return returned_legal_direction;
}

bool OddEvenRouting::is_odd(int number) {
    return (number % 2) == 1;
}

bool OddEvenRouting::is_even(int number) {
    return (number % 2) == 0;
}

bool OddEvenRouting::is_turn_legal(const std::array<std::reference_wrapper<const NocRouter>, 3>& noc_routers,
                                   const NocStorage& noc_model) const {
    const auto [x1, y1, z1] = noc_routers[0].get().get_router_physical_location();
    const auto [x2, y2, z2] = noc_routers[1].get().get_router_physical_location();
    const auto [x3, y3, z3] = noc_routers[2].get().get_router_physical_location();

    // check if the given routers can be traversed one after another
    VTR_ASSERT(vtr::exactly_k_conditions(2, x1 == x2, y1 == y2, z1 == z2));
    VTR_ASSERT(vtr::exactly_k_conditions(2, x2 == x3, y2 == y3, z2 == z3));

    // get the position of the second NoC routers
    const auto router2_pos = noc_routers[1].get().get_router_physical_location();

    NocRouterId router2_id = noc_model.get_router_at_grid_location({router2_pos, 0});

    // get the compressed location of the second NoC router
    auto compressed_2_loc = compressed_noc_locs_[router2_id];

    // going back to the first router is not allowed (180-degree turns)
    if (x1 == x3 && y1 == y3 && z1 == z3) {
        return false;
    }

    if (noc_model.is_noc_3d()) {
        /* A packet is not allowed to take any of the X+ --> YZ turns at a router
         * located in an even yz-place. */
        if (is_even(compressed_2_loc.x)) {
            if (x2 > x1 && (y3 != y2 || z3 != z2)) {
                return false;
            }
        }

        /* A packet is not allowed to take any of the YZ ---> X- turns at a router
         * located in an odd yz-plane. */
        if (is_odd(compressed_2_loc.x)) {
            if ((y1 != y2 || z1 != z2) && x2 < x1) {
                return false;
            }
        }


        // check if the turn is compatible with odd-even routing algorithm turn restrictions
        if (is_odd(compressed_2_loc.y)) {
            if ((z2 > z1 && y3 < y2) || (z2 < z1 && y3 < y2)) {
                return false;
            }
        } else { // y is even
            if ((y2 > y1 && z3 > z2) || (y2 > y1 && z3 < z2)) {
                return false;
            }
        }
    } else {  // 2D NoC
        if (is_odd(compressed_2_loc.x)) {
            if (y2 != y1 && x3 < x2) {
                return false;
            }
        } else { // even column
            if (x2 > x1 && y2 != y3) {
                return false;
            }
        }
    }

    return true;
}

void OddEvenRouting::determine_legal_directions_2d(t_physical_tile_loc comp_src_loc,
                                                   t_physical_tile_loc comp_curr_loc,
                                                   t_physical_tile_loc comp_dst_loc,
                                                   TurnModelRouting::Direction prev_dir) {

    // calculate the distance between the current router and the destination
    const int diff_x = comp_dst_loc.x - comp_curr_loc.x;
    const int diff_y = comp_dst_loc.y - comp_curr_loc.y;

    VTR_ASSERT_SAFE(comp_dst_loc.layer_num == comp_src_loc.layer_num);
    VTR_ASSERT_SAFE(comp_dst_loc.layer_num == comp_curr_loc.layer_num);

    if (diff_x == 0) { // the same column as the destination. Only north or south are allowed
        if (diff_y > 0) {
            returned_legal_direction.push_back(TurnModelRouting::Direction::NORTH);
        } else {
            returned_legal_direction.push_back(TurnModelRouting::Direction::SOUTH);
        }
    } else if (diff_x > 0) { // eastbound message
        if (diff_y == 0) {   // already in the same row as the destination. Just move to the east
            returned_legal_direction.push_back(TurnModelRouting::Direction::EAST);
        } else {
            /* Since EN and ES turns are forbidden in even columns, we move along the vertical
             * direction only in we are in an odd column. */
            if (is_odd(comp_curr_loc.x) || comp_curr_loc.x == comp_src_loc.x || prev_dir != TurnModelRouting::Direction::EAST) {
                if (diff_y > 0) {
                    returned_legal_direction.push_back(TurnModelRouting::Direction::NORTH);
                } else {
                    returned_legal_direction.push_back(TurnModelRouting::Direction::SOUTH);
                }
            }
            // the destination column is odd and there are more than 1 column left to destination
            if (is_odd(comp_dst_loc.x) || diff_x != 1) {
                returned_legal_direction.push_back(TurnModelRouting::Direction::EAST);
            }
        }
    } else { // westbound message
        returned_legal_direction.push_back(TurnModelRouting::Direction::WEST);
        /* Since NW and SW turns are forbidden in odd columns, we allow
         * moving along vertical axis only in even columns */
        if (is_even(comp_curr_loc.x)) {
            if (diff_y > 0) {
                returned_legal_direction.push_back(TurnModelRouting::Direction::NORTH);
            } else {
                returned_legal_direction.push_back(TurnModelRouting::Direction::SOUTH);
            }
        }
    }


}

void OddEvenRouting::determine_legal_directions_3d(t_physical_tile_loc comp_src_loc,
                                                   t_physical_tile_loc comp_curr_loc,
                                                   t_physical_tile_loc comp_dst_loc,
                                                   TurnModelRouting::Direction prev_dir) {
    // calculate the distance between the current router and the destination
    const int diff_x = comp_dst_loc.x - comp_curr_loc.x;
    const int diff_y = comp_dst_loc.y - comp_curr_loc.y;
    const int diff_z = comp_dst_loc.layer_num - comp_curr_loc.layer_num;

    if (diff_x > 0) {

        if (diff_y == 0 && diff_z == 0) {
            returned_legal_direction.push_back(TurnModelRouting::Direction::EAST);
            return;
        }

        if (is_odd(comp_dst_loc.x) || diff_x > 1) {
            returned_legal_direction.push_back(TurnModelRouting::Direction::EAST);
        }

        if (is_odd(comp_curr_loc.x) || comp_curr_loc.x == comp_src_loc.x || prev_dir != TurnModelRouting::Direction::EAST) {
            goto route_in_yz_plane;
        } else {
            return;
        }

    } else if (diff_x < 0) {
        returned_legal_direction.push_back(TurnModelRouting::Direction::WEST);

        if (is_even(comp_curr_loc.x)) {
            goto route_in_yz_plane;
        } else {
            return;
        }
    }


    route_in_yz_plane :

    if (diff_y == 0) { // the same column as the destination. Only north or south are allowed
        if (diff_z > 0) {
            returned_legal_direction.push_back(TurnModelRouting::Direction::UP);
        } else {
            returned_legal_direction.push_back(TurnModelRouting::Direction::DOWN);
        }
    } else if (diff_y > 0) { // eastbound message
        if (diff_z == 0) {   // already in the same row as the destination. Just move to the east
            returned_legal_direction.push_back(TurnModelRouting::Direction::NORTH);
        } else {
            /* Since EN and ES turns are forbidden in even columns, we move along the vertical
             * direction only in we are in an odd column. */
            if (is_odd(comp_curr_loc.y) || comp_curr_loc.y == comp_src_loc.y || prev_dir != TurnModelRouting::Direction::NORTH) {
                if (diff_z > 0) {
                    returned_legal_direction.push_back(TurnModelRouting::Direction::UP);
                } else {
                    returned_legal_direction.push_back(TurnModelRouting::Direction::DOWN);
                }
            }
            // the destination column is odd and there are more than 1 column left to destination
            if (is_odd(comp_dst_loc.y) || diff_y > 1) {
                returned_legal_direction.push_back(TurnModelRouting::Direction::NORTH);
            }
        }
    } else { // westbound message
        returned_legal_direction.push_back(TurnModelRouting::Direction::SOUTH);
        /* Since NW and SW turns are forbidden in odd columns, we allow
         * moving along vertical axis only in even columns */
        if (is_even(comp_curr_loc.y)) {
            if (diff_z > 0) {
                returned_legal_direction.push_back(TurnModelRouting::Direction::UP);
            } else {
                returned_legal_direction.push_back(TurnModelRouting::Direction::DOWN);
            }
        }
    }
}
