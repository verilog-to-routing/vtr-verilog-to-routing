#include "median_move_generator.h"

#include "globals.h"
#include "physical_types_util.h"
#include "place_constraints.h"
#include "place_macro.h"
#include "placer_state.h"
#include "move_utils.h"

#include <algorithm>

MedianMoveGenerator::MedianMoveGenerator(PlacerState& placer_state,
                                         e_reward_function reward_function,
                                         vtr::RngContainer& rng)
    : MoveGenerator(placer_state, reward_function, rng) {}

e_create_move MedianMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected,
                                                t_propose_action& proposed_action,
                                                float rlim,
                                                const PlaceMacros& place_macros,
                                                const t_placer_opts& placer_opts,
                                                const PlacerCriticalities* /*criticalities*/) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& device_ctx = g_vpr_ctx.device();
    auto& placer_state = placer_state_.get();
    auto& place_move_ctx = placer_state.mutable_move();
    const auto& block_locs = placer_state.block_locs();
    const auto& blk_loc_registry = placer_state.blk_loc_registry();

    //Find a movable block based on blk_type
    ClusterBlockId b_from = propose_block_to_move(placer_opts,
                                                  proposed_action.logical_blk_type_index,
                                                  /*highly_crit_block=*/false,
                                                  /*placer_criticalities=*/nullptr,
                                                  /*net_from=*/nullptr,
                                                  /*pin_from=*/nullptr,
                                                  placer_state,
                                                  rng_);

    VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "Median Move Choose Block %d - rlim %f\n", size_t(b_from), rlim);

    if (!b_from) { //No movable block found
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\tNo movable block found\n");
        return e_create_move::ABORT;
    }

    const int num_layers = device_ctx.grid.get_num_layers();


    t_pl_loc from = block_locs[b_from].loc;
    int from_layer = from.layer;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = g_vpr_ctx.device().grid.get_physical_type({from.x, from.y, from_layer});
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));

    /* Calculate the median region */
    t_pl_loc to;

    t_bb coords(OPEN, OPEN, OPEN, OPEN, OPEN, OPEN);
    t_bb limit_coords;

    //clear the vectors that saves X & Y coords
    //reused to save allocation time
    place_move_ctx.X_coord.clear();
    place_move_ctx.Y_coord.clear();
    place_move_ctx.layer_coord.clear();
    std::vector<int> layer_blk_cnt(num_layers, 0);

    //true if the net is a feedback from the block to itself
    bool skip_net;

    //iterate over block pins
    for (ClusterPinId pin_id : cluster_ctx.clb_nlist.block_pins(b_from)) {
        ClusterNetId net_id = cluster_ctx.clb_nlist.pin_net(pin_id);
        if (cluster_ctx.clb_nlist.net_is_ignored(net_id))
            continue;
        /* To speed up the calculation, we found it is useful to ignore high fanout nets.
         * Especially that in most cases, these high fanout nets are scattered in many locations of
         * the device and don't guide to a specific location. We also assured these assumptions experimentally.
         */
        if (int(cluster_ctx.clb_nlist.net_pins(net_id).size()) > placer_opts.place_high_fanout_net)
            continue;
        if (cluster_ctx.clb_nlist.net_sinks(net_id).size() < SMALL_NET) {
            //calculate the bb from scratch
            get_bb_from_scratch_excluding_block(net_id, coords, b_from, skip_net);
            if (skip_net) {
                continue;
            }
        } else {
            t_bb union_bb;
            const bool cube_bb = g_vpr_ctx.placement().cube_bb;
            if (!cube_bb) {
                union_bb = union_2d_bb(place_move_ctx.layer_bb_coords[net_id]);
            }

            const auto& net_bb_coords = cube_bb ? place_move_ctx.bb_coords[net_id] : union_bb;
            t_physical_tile_loc old_pin_loc = blk_loc_registry.get_coordinate_of_pin(pin_id);

            t_physical_tile_loc new_pin_loc;
            //To calculate the bb incrementally while excluding the moving block
            //assume that the moving block is moved to a non-critical coord of the bb
            if (net_bb_coords.xmin == old_pin_loc.x) {
                new_pin_loc.x = net_bb_coords.xmax;
            } else {
                new_pin_loc.x = net_bb_coords.xmin;
            }

            if (net_bb_coords.ymin == old_pin_loc.y) {
                new_pin_loc.y = net_bb_coords.ymax;
            } else {
                new_pin_loc.y = net_bb_coords.ymin;
            }

            if (net_bb_coords.layer_min == old_pin_loc.layer_num) {
                new_pin_loc.layer_num = net_bb_coords.layer_max;
            } else {
                new_pin_loc.layer_num = net_bb_coords.layer_min;
            }
            
            // If the moving block is on the border of the bounding box, we cannot get
            // the bounding box incrementally. In that case, bounding box should be calculated
            // from scratch.
            if (!get_bb_incrementally(net_id, coords, old_pin_loc, new_pin_loc)) {
                get_bb_from_scratch_excluding_block(net_id, coords, b_from, skip_net);
                if (skip_net)
                    continue;
            }
        }
        //push the calculated coordinates into X,Y coord vectors
        place_move_ctx.X_coord.push_back(coords.xmin);
        place_move_ctx.X_coord.push_back(coords.xmax);
        place_move_ctx.Y_coord.push_back(coords.ymin);
        place_move_ctx.Y_coord.push_back(coords.ymax);
        place_move_ctx.layer_coord.push_back(coords.layer_min);
        place_move_ctx.layer_coord.push_back(coords.layer_max);
    }

    if ((place_move_ctx.X_coord.empty()) || (place_move_ctx.Y_coord.empty()) || (place_move_ctx.layer_coord.empty())) {
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\tMove aborted - X_coord or y_coord or layer_coord are empty\n");
        return e_create_move::ABORT;
    }

    //calculate the median region
    std::stable_sort(place_move_ctx.X_coord.begin(), place_move_ctx.X_coord.end());
    std::stable_sort(place_move_ctx.Y_coord.begin(), place_move_ctx.Y_coord.end());
    std::stable_sort(place_move_ctx.layer_coord.begin(), place_move_ctx.layer_coord.end());

    limit_coords.xmin = place_move_ctx.X_coord[((place_move_ctx.X_coord.size() - 1) / 2)];
    limit_coords.xmax = place_move_ctx.X_coord[((place_move_ctx.X_coord.size() - 1) / 2) + 1];

    limit_coords.ymin = place_move_ctx.Y_coord[((place_move_ctx.Y_coord.size() - 1) / 2)];
    limit_coords.ymax = place_move_ctx.Y_coord[((place_move_ctx.Y_coord.size() - 1) / 2) + 1];

    limit_coords.layer_min = place_move_ctx.layer_coord[((place_move_ctx.layer_coord.size() - 1) / 2)];
    limit_coords.layer_max = place_move_ctx.layer_coord[((place_move_ctx.layer_coord.size() - 1) / 2) + 1];

    //arrange the different range limiters
    t_range_limiters range_limiters{rlim,
                                    place_move_ctx.first_rlim,
                                    placer_opts.place_dm_rlim};

    //find a location in a range around the center of median region
    t_pl_loc median_point;
    median_point.x = (limit_coords.xmin + limit_coords.xmax) / 2;
    median_point.y = (limit_coords.ymin + limit_coords.ymax) / 2;
    median_point.layer = (limit_coords.layer_min + limit_coords.layer_max) / 2;

    if (!find_to_loc_centroid(cluster_from_type, from, median_point, range_limiters, to, b_from, blk_loc_registry, rng_)) {
        return e_create_move::ABORT;
    }

    e_create_move create_move = ::create_move(blocks_affected, b_from, to, blk_loc_registry, place_macros);

    //Check that all the blocks affected by the move would still be in a legal floorplan region after the swap
    if (!floorplan_legal(blocks_affected)) {
        return e_create_move::ABORT;
    }

    return create_move;
}

void MedianMoveGenerator::get_bb_from_scratch_excluding_block(ClusterNetId net_id,
                                                              t_bb& bb_coord_new,
                                                              ClusterBlockId moving_block_id,
                                                              bool& skip_net) {
    //TODO: account for multiple physical pin instances per logical pin
    const auto& blk_loc_registry = placer_state_.get().blk_loc_registry();
    const auto& cluster_ctx = g_vpr_ctx.clustering();

    /* If the net is only connected to the moving block, it should be skipped.
     * Let's initially assume that the net is only connected to the moving block.
     * When going through the driver and sink blocks, we check if they are the same
     * as the moving block. If not, we set this flag to false. */
    skip_net = true;

    int xmin = OPEN;
    int xmax = OPEN;
    int ymin = OPEN;
    int ymax = OPEN;
    int layer_min = OPEN;
    int layer_max = OPEN;

    ClusterBlockId driver_block_id = cluster_ctx.clb_nlist.net_driver_block(net_id);
    bool first_block = false;

    if (driver_block_id != moving_block_id) {
        skip_net = false;

        // get the source pin's location
        ClusterPinId source_pin_id = cluster_ctx.clb_nlist.net_pin(net_id, 0);
        t_physical_tile_loc source_pin_loc = blk_loc_registry.get_coordinate_of_pin(source_pin_id);

        xmin = source_pin_loc.x;
        ymin = source_pin_loc.y;
        xmax = source_pin_loc.x;
        ymax = source_pin_loc.y;
        layer_min = source_pin_loc.layer_num;
        layer_max = source_pin_loc.layer_num;
        first_block = true;
    }

    for (ClusterPinId pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
        ClusterBlockId sink_block_id = cluster_ctx.clb_nlist.pin_block(pin_id);

        if (sink_block_id == moving_block_id) {
            continue;
        }

        skip_net = false;

        t_physical_tile_loc pin_loc = blk_loc_registry.get_coordinate_of_pin(pin_id);

        if (!first_block) {
            xmin = pin_loc.x;
            ymin = pin_loc.y;
            xmax = pin_loc.x;
            ymax = pin_loc.y;
            layer_max = pin_loc.layer_num;
            layer_min = pin_loc.layer_num;
            first_block = true;
            continue;
        }

        if (pin_loc.x < xmin) {
            xmin = pin_loc.x;
        } else if (pin_loc.x > xmax) {
            xmax = pin_loc.x;
        }

        if (pin_loc.y < ymin) {
            ymin = pin_loc.y;
        } else if (pin_loc.y > ymax) {
            ymax = pin_loc.y;
        }

        if (pin_loc.layer_num < layer_min) {
            layer_min = pin_loc.layer_num;
        } else if (pin_loc.layer_num > layer_max) {
            layer_max = pin_loc.layer_num;
        }
    }

    bb_coord_new.xmin = xmin;
    bb_coord_new.ymin = ymin;
    bb_coord_new.layer_min = layer_min;
    bb_coord_new.xmax = xmax;
    bb_coord_new.ymax = ymax;
    bb_coord_new.layer_max = layer_max;
}

bool MedianMoveGenerator::get_bb_incrementally(ClusterNetId net_id,
                                               t_bb& bb_coord_new,
                                               t_physical_tile_loc old_pin_loc,
                                               t_physical_tile_loc new_pin_loc) {
    //TODO: account for multiple physical pin instances per logical pin
    const auto& place_move_ctx = placer_state_.get().move();

    t_bb union_bb_edge;
    t_bb union_bb;
    const bool cube_bb = g_vpr_ctx.placement().cube_bb;
    /* Calculating per-layer bounding box is more time-consuming compared to cube bounding box. To speed up
    * this move, the bounding box used for this move is of the type cube bounding box even if the per-layer
    * bounding box is used by placement SA engine. 
    * If per-layer bounding box is used, we take a union of bounding boxes on each layer to make a cube bounding box.
    * For example, the xmax of this cube bounding box is determined by the maximum x coordinate across all blocks on all layers.
    */
    if (!cube_bb) {
        std::tie(union_bb_edge, union_bb) = union_2d_bb_incr(place_move_ctx.layer_bb_num_on_edges[net_id],
                                                             place_move_ctx.layer_bb_coords[net_id]);
    }

    /* In this move, we use a 3D bounding box. Thus, if per-layer BB is used by placer, we need to take a union of BBs and use that for the rest of
     * operations in this move
     */
    const t_bb& curr_bb_edge = cube_bb ? place_move_ctx.bb_num_on_edges[net_id] : union_bb_edge;
    const t_bb& curr_bb_coord = cube_bb ? place_move_ctx.bb_coords[net_id] : union_bb;

    /* Check if I can update the bounding box incrementally. */

    if (new_pin_loc.x < old_pin_loc.x) { /* Move to left. */

        /* Update the xmax fields for coordinates and number of edges first. */

        if (old_pin_loc.x == curr_bb_coord.xmax) { /* Old position at xmax. */
            if (curr_bb_edge.xmax == 1) {
                return false;
            } else {
                bb_coord_new.xmax = curr_bb_coord.xmax;
            }
        } else { /* Move to left, old postion was not at xmax. */
            bb_coord_new.xmax = curr_bb_coord.xmax;
        }

        /* Now do the xmin fields for coordinates and number of edges. */

        if (new_pin_loc.x < curr_bb_coord.xmin) { /* Moved past xmin */
            bb_coord_new.xmin = new_pin_loc.x;
        } else if (new_pin_loc.x == curr_bb_coord.xmin) { /* Moved to xmin */
            bb_coord_new.xmin = new_pin_loc.x;
        } else { /* Xmin unchanged. */
            bb_coord_new.xmin = curr_bb_coord.xmin;
        }
        /* End of move to left case. */

    } else if (new_pin_loc.x > old_pin_loc.x) { /* Move to right. */

        /* Update the xmin fields for coordinates and number of edges first. */

        if (old_pin_loc.x == curr_bb_coord.xmin) { /* Old position at xmin. */
            if (curr_bb_edge.xmin == 1) {
                return false;
            } else {
                bb_coord_new.xmin = curr_bb_coord.xmin;
            }
        } else { /* Move to right, old position was not at xmin. */
            bb_coord_new.xmin = curr_bb_coord.xmin;
        }
        /* Now do the xmax fields for coordinates and number of edges. */

        if (new_pin_loc.x > curr_bb_coord.xmax) { /* Moved past xmax. */
            bb_coord_new.xmax = new_pin_loc.x;
        } else if (new_pin_loc.x == curr_bb_coord.xmax) { /* Moved to xmax */
            bb_coord_new.xmax = new_pin_loc.x;
        } else { /* Xmax unchanged. */
            bb_coord_new.xmax = curr_bb_coord.xmax;
        }
        /* End of move to right case. */

    } else { /* new_pin_loc.x == old_pin_loc.x -- no x motion. */
        bb_coord_new.xmin = curr_bb_coord.xmin;
        bb_coord_new.xmax = curr_bb_coord.xmax;
    }

    /* Now account for the y-direction motion. */

    if (new_pin_loc.y < old_pin_loc.y) { /* Move down. */

        /* Update the ymax fields for coordinates and number of edges first. */

        if (old_pin_loc.y == curr_bb_coord.ymax) { /* Old position at ymax. */
            if (curr_bb_edge.ymax == 1) {
                return false;
            } else {
                bb_coord_new.ymax = curr_bb_coord.ymax;
            }
        } else { /* Move down, old postion was not at ymax. */
            bb_coord_new.ymax = curr_bb_coord.ymax;
        }

        /* Now do the ymin fields for coordinates and number of edges. */

        if (new_pin_loc.y < curr_bb_coord.ymin) { /* Moved past ymin */
            bb_coord_new.ymin = new_pin_loc.y;
        } else if (new_pin_loc.y == curr_bb_coord.ymin) { /* Moved to ymin */
            bb_coord_new.ymin = new_pin_loc.y;
        } else { /* ymin unchanged. */
            bb_coord_new.ymin = curr_bb_coord.ymin;
        }
        /* End of move down case. */

    } else if (new_pin_loc.y > old_pin_loc.y) { /* Moved up. */

        /* Update the ymin fields for coordinates and number of edges first. */

        if (old_pin_loc.y == curr_bb_coord.ymin) { /* Old position at ymin. */
            if (curr_bb_edge.ymin == 1) {
                return false;
            } else {
                bb_coord_new.ymin = curr_bb_coord.ymin;
            }
        } else { /* Moved up, old position was not at ymin. */
            bb_coord_new.ymin = curr_bb_coord.ymin;
        }

        /* Now do the ymax fields for coordinates and number of edges. */

        if (new_pin_loc.y > curr_bb_coord.ymax) { /* Moved past ymax. */
            bb_coord_new.ymax = new_pin_loc.y;
        } else if (new_pin_loc.y == curr_bb_coord.ymax) { /* Moved to ymax */
            bb_coord_new.ymax = new_pin_loc.y;
        } else { /* ymax unchanged. */
            bb_coord_new.ymax = curr_bb_coord.ymax;
        }
        /* End of move up case. */

    } else { /* new_pin_loc.y == old_pin_loc.y -- no y motion. */
        bb_coord_new.ymin = curr_bb_coord.ymin;
        bb_coord_new.ymax = curr_bb_coord.ymax;
    }

    if (new_pin_loc.layer_num < old_pin_loc.layer_num) {
        if (old_pin_loc.layer_num == curr_bb_coord.layer_max) {
            if (curr_bb_edge.layer_max == 1) {
                return false;
            } else {
                bb_coord_new.layer_max = curr_bb_coord.layer_max;
            }
        } else {
            bb_coord_new.layer_max = curr_bb_coord.layer_max;
        }

        if (new_pin_loc.layer_num < curr_bb_coord.layer_min) {
            bb_coord_new.layer_min = new_pin_loc.layer_num;
        } else if (new_pin_loc.layer_num == curr_bb_coord.layer_min) {
            bb_coord_new.layer_min = new_pin_loc.layer_num;
        } else {
            bb_coord_new.layer_min = curr_bb_coord.layer_min;
        }

    } else if (new_pin_loc.layer_num > old_pin_loc.layer_num) {
        if (old_pin_loc.layer_num == curr_bb_coord.layer_min) {
            if (curr_bb_edge.layer_min == 1) {
                return false;
            } else {
                bb_coord_new.layer_min = curr_bb_coord.layer_min;
            }
        } else {
            bb_coord_new.layer_min = curr_bb_coord.layer_min;
        }

        if (new_pin_loc.layer_num > curr_bb_coord.layer_max) {
            bb_coord_new.layer_max = new_pin_loc.layer_num;
        } else if (new_pin_loc.layer_num == curr_bb_coord.layer_max) {
            bb_coord_new.layer_max = new_pin_loc.layer_num;
        } else {
            bb_coord_new.layer_max = curr_bb_coord.layer_max;
        }
    } else {
        bb_coord_new.layer_min = curr_bb_coord.layer_min;
        bb_coord_new.layer_max = curr_bb_coord.layer_max;
    }
    return true;
}
