#include "median_move_generator.h"
#include "globals.h"
#include <algorithm>
#include "place_constraints.h"
#include "placer_globals.h"
#include "move_utils.h"

static bool get_bb_incrementally(ClusterNetId net_id,
                                 t_bb& bb_coord_new,
                                 int xold,
                                 int yold,
                                 int layer_old,
                                 int xnew,
                                 int ynew,
                                 int layer_new);

static void get_bb_from_scratch_excluding_block(ClusterNetId net_id, t_bb& bb_coord_new, ClusterBlockId block_id, bool& skip_net);

e_create_move MedianMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected, t_propose_action& proposed_action, float rlim, const t_placer_opts& placer_opts, const PlacerCriticalities* /*criticalities*/) {
    //Find a movable block based on blk_type
    ClusterBlockId b_from = propose_block_to_move(placer_opts,
                                                  proposed_action.logical_blk_type_index,
                                                  false,
                                                  nullptr,
                                                  nullptr);
    VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "Median Move Choose Block %d - rlim %f\n", size_t(b_from), rlim);

    if (!b_from) { //No movable block found
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\tNo movable block found\n");
        return e_create_move::ABORT;
    }

    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();
    auto& place_move_ctx = g_placer_ctx.mutable_move();

    const int num_layers = device_ctx.grid.get_num_layers();


    t_pl_loc from = place_ctx.block_locs[b_from].loc;
    int from_layer = from.layer;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = g_vpr_ctx.device().grid.get_physical_type({from.x, from.y, from_layer});
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));

    /* Calculate the median region */
    t_pl_loc to;

    t_bb coords(OPEN, OPEN, OPEN, OPEN, OPEN, OPEN);
    t_bb limit_coords;
    ClusterBlockId bnum;
    int pnum, xnew, xold, ynew, yold, layer_new, layer_old;

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
            if (skip_net)
                continue;
        } else {
            t_bb union_bb;
            const bool& cube_bb = g_vpr_ctx.placement().cube_bb;
            if (!cube_bb) {
                union_bb = union_2d_bb(place_move_ctx.layer_bb_coords[net_id]);
            }

            const auto& net_bb_coords = cube_bb ? place_move_ctx.bb_coords[net_id] : union_bb;
            //use the incremental update of the bb
            bnum = cluster_ctx.clb_nlist.pin_block(pin_id);
            pnum = tile_pin_index(pin_id);
            VTR_ASSERT(pnum >= 0);
            xold = place_ctx.block_locs[bnum].loc.x + physical_tile_type(bnum)->pin_width_offset[pnum];
            yold = place_ctx.block_locs[bnum].loc.y + physical_tile_type(bnum)->pin_height_offset[pnum];
            layer_old = place_ctx.block_locs[bnum].loc.layer;

            xold = std::max(std::min(xold, (int)device_ctx.grid.width() - 2), 1);  //-2 for no perim channels
            yold = std::max(std::min(yold, (int)device_ctx.grid.height() - 2), 1); //-2 for no perim channels
            layer_old = std::max(std::min(layer_old, (int)device_ctx.grid.get_num_layers() - 1), 0);

            //To calulate the bb incrementally while excluding the moving block
            //assume that the moving block is moved to a non-critical coord of the bb
            if (net_bb_coords.xmin == xold) {
                xnew = net_bb_coords.xmax;
            } else {
                xnew = net_bb_coords.xmin;
            }

            if (net_bb_coords.ymin == yold) {
                ynew = net_bb_coords.ymax;
            } else {
                ynew = net_bb_coords.ymin;
            }

            if (net_bb_coords.layer_min == layer_old) {
                layer_new = net_bb_coords.layer_max;
            } else {
                layer_new = net_bb_coords.layer_min;
            }
            
            // If the mvoing block is on the border of the bounding box, we cannot get
            // the bounding box incrementatlly. In that case, bounding box should be calculated 
            // from scratch.
            if (!get_bb_incrementally(net_id,
                                      coords,
                                      xold,
                                      yold,
                                      layer_old,
                                      xnew,
                                      ynew,
                                      layer_new)) {
                get_bb_from_scratch_excluding_block(net_id, coords, b_from, skip_net);
                if (skip_net)
                    continue;
            }
        }
        //push the calculated coorinates into X,Y coord vectors
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

    limit_coords.xmin = place_move_ctx.X_coord[floor((place_move_ctx.X_coord.size() - 1) / 2)];
    limit_coords.xmax = place_move_ctx.X_coord[floor((place_move_ctx.X_coord.size() - 1) / 2) + 1];

    limit_coords.ymin = place_move_ctx.Y_coord[floor((place_move_ctx.Y_coord.size() - 1) / 2)];
    limit_coords.ymax = place_move_ctx.Y_coord[floor((place_move_ctx.Y_coord.size() - 1) / 2) + 1];

    limit_coords.layer_min = place_move_ctx.layer_coord[floor((place_move_ctx.layer_coord.size() - 1) / 2)];
    limit_coords.layer_max = place_move_ctx.layer_coord[floor((place_move_ctx.layer_coord.size() - 1) / 2) + 1];

    //arrange the different range limiters
    t_range_limiters range_limiters{rlim,
                                    place_move_ctx.first_rlim,
                                    placer_opts.place_dm_rlim};

    //find a location in a range around the center of median region
    t_pl_loc median_point;
    median_point.x = (limit_coords.xmin + limit_coords.xmax) / 2;
    median_point.y = (limit_coords.ymin + limit_coords.ymax) / 2;
    median_point.layer = (limit_coords.layer_min + limit_coords.layer_max) / 2;

    if (!find_to_loc_centroid(cluster_from_type, from, median_point, range_limiters, to, b_from)) {
        return e_create_move::ABORT;
    }

    e_create_move create_move = ::create_move(blocks_affected, b_from, to);

    //Check that all the blocks affected by the move would still be in a legal floorplan region after the swap
    if (!floorplan_legal(blocks_affected)) {
        return e_create_move::ABORT;
    }

    return create_move;
}

/* Finds the bounding box of a net and stores its coordinates in the  *
 * bb_coord_new data structure. It excludes the moving block sent in  *
 * function arguments in block_id. It also returns whether this net   *
 * should be excluded from median calculation or not.                 *
 * This routine should only be called for small nets, since it does   *
 * not determine enough information for the bounding box to be        *
 * updated incrementally later.                                       *
 * Currently assumes channels on both sides of the CLBs forming the   *
 * edges of the bounding box can be used.  Essentially, I am assuming *
 * the pins always lie on the outside of the bounding box.            */
static void get_bb_from_scratch_excluding_block(ClusterNetId net_id, t_bb& bb_coord_new, ClusterBlockId block_id, bool& skip_net) {
    //TODO: account for multiple physical pin instances per logical pin

    skip_net = true;

    int xmin = OPEN;
    int xmax = OPEN;
    int ymin = OPEN;
    int ymax = OPEN;

    int layer_min = OPEN;
    int layer_max = OPEN;

    int pnum;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();

    ClusterBlockId bnum = cluster_ctx.clb_nlist.net_driver_block(net_id);
    bool first_block = false;

    if (bnum != block_id) {
        skip_net = false;
        pnum = net_pin_to_tile_pin_index(net_id, 0);
        int src_x = place_ctx.block_locs[bnum].loc.x + physical_tile_type(bnum)->pin_width_offset[pnum];
        int src_y = place_ctx.block_locs[bnum].loc.y + physical_tile_type(bnum)->pin_height_offset[pnum];
        int src_layer = place_ctx.block_locs[bnum].loc.layer;

        xmin = src_x;
        ymin = src_y;
        xmax = src_x;
        ymax = src_y;
        layer_min = src_layer;
        layer_max = src_layer;
        first_block = true;
    }

    for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
        bnum = cluster_ctx.clb_nlist.pin_block(pin_id);
        pnum = tile_pin_index(pin_id);
        if (bnum == block_id)
            continue;
        skip_net = false;
        const auto& block_loc = place_ctx.block_locs[bnum].loc;
        int x = block_loc.x + physical_tile_type(bnum)->pin_width_offset[pnum];
        int y = block_loc.y + physical_tile_type(bnum)->pin_height_offset[pnum];
        int layer = block_loc.layer;

        if (!first_block) {
            xmin = x;
            ymin = y;
            xmax = x;
            ymax = y;
            layer_max = layer;
            layer_min = layer;
            first_block = true;
            continue;
        }
        if (x < xmin) {
            xmin = x;
        } else if (x > xmax) {
            xmax = x;
        }

        if (y < ymin) {
            ymin = y;
        } else if (y > ymax) {
            ymax = y;
        }

        if (layer < layer_min) {
            layer_min = layer;
        } else if (layer > layer_max) {
            layer_max = layer;
        }
    }

    /* Now I've found the coordinates of the bounding box.  There are no *
     * channels beyond device_ctx.grid.width()-2 and                     *
     * device_ctx.grid.height() - 2, so I want to clip to that.  As well,*
     * since I'll always include the channel immediately below and the   *
     * channel immediately to the left of the bounding box, I want to    *
     * clip to 1 in both directions as well (since minimum channel index *
     * is 0).  See route_common.cpp for a channel diagram.               */
    bb_coord_new.xmin = std::max(std::min<int>(xmin, device_ctx.grid.width() - 2), 1);  //-2 for no perim channels
    bb_coord_new.ymin = std::max(std::min<int>(ymin, device_ctx.grid.height() - 2), 1); //-2 for no perim channels
    bb_coord_new.layer_min = std::max(std::min<int>(layer_min, device_ctx.grid.get_num_layers() - 1), 0);
    bb_coord_new.xmax = std::max(std::min<int>(xmax, device_ctx.grid.width() - 2), 1);  //-2 for no perim channels
    bb_coord_new.ymax = std::max(std::min<int>(ymax, device_ctx.grid.height() - 2), 1); //-2 for no perim channels
    bb_coord_new.layer_max = std::max(std::min<int>(layer_max, device_ctx.grid.get_num_layers() - 1), 0);
}

/*
 * Calculates the bounding box of a net by storing its coordinates    *
 * in the bb_coord_new data structure. It uses information from       *
 * PlaceMoveContext to calculate the bb incrementally. This routine   *
 * should only be called for large nets, since it has some overhead   *
 * relative to just doing a brute force bounding box calculation.     *
 * The bounding box coordinate and edge information for inet must be  *
 * valid before this routine is called.                               *
 * Currently assumes channels on both sides of the CLBs forming the   *
 * edges of the bounding box can be used. Essentially, I am assuming *
 * the pins always lie on the outside of the bounding box.            *
 * The x and y coordinates are the pin's x and y coordinates.         */
/* IO blocks are considered to be one cell in for simplicity.         */
static bool get_bb_incrementally(ClusterNetId net_id,
                                 t_bb& bb_coord_new,
                                 int xold,
                                 int yold,
                                 int layer_old,
                                 int xnew,
                                 int ynew,
                                 int layer_new) {
    //TODO: account for multiple physical pin instances per logical pin

    auto& device_ctx = g_vpr_ctx.device();
    auto& place_move_ctx = g_placer_ctx.move();

    xnew = std::max(std::min<int>(xnew, device_ctx.grid.width() - 2), 1);  //-2 for no perim channels
    ynew = std::max(std::min<int>(ynew, device_ctx.grid.height() - 2), 1); //-2 for no perim channels
    layer_new = std::max(std::min<int>(layer_new, device_ctx.grid.get_num_layers() - 1), 0);

    xold = std::max(std::min<int>(xold, device_ctx.grid.width() - 2), 1);  //-2 for no perim channels
    yold = std::max(std::min<int>(yold, device_ctx.grid.height() - 2), 1); //-2 for no perim channels
    layer_old = std::max(std::min<int>(layer_old, device_ctx.grid.get_num_layers() - 1), 0);

    t_bb union_bb_edge;
    t_bb union_bb;
    const bool& cube_bb = g_vpr_ctx.placement().cube_bb;
    /* Calculating per-layer bounding box is more time consuming compared to cube bounding box. To speed up
    * this move, the bounding box used for this move is of the type cube bounding box even if the per-layer
    * bounding box is used by placement SA engine. 
    * If per-layer bounding box is used, we take a union of boundinx boxes on each layer to make a cube bounding box.
    * For example, the xmax of this cube boundix box is determined by the maximim x coordinate across all blocks on all layers.
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

    if (xnew < xold) { /* Move to left. */

        /* Update the xmax fields for coordinates and number of edges first. */

        if (xold == curr_bb_coord.xmax) { /* Old position at xmax. */
            if (curr_bb_edge.xmax == 1) {
                return false;
            } else {
                bb_coord_new.xmax = curr_bb_coord.xmax;
            }
        } else { /* Move to left, old postion was not at xmax. */
            bb_coord_new.xmax = curr_bb_coord.xmax;
        }

        /* Now do the xmin fields for coordinates and number of edges. */

        if (xnew < curr_bb_coord.xmin) { /* Moved past xmin */
            bb_coord_new.xmin = xnew;
        } else if (xnew == curr_bb_coord.xmin) { /* Moved to xmin */
            bb_coord_new.xmin = xnew;
        } else { /* Xmin unchanged. */
            bb_coord_new.xmin = curr_bb_coord.xmin;
        }
        /* End of move to left case. */

    } else if (xnew > xold) { /* Move to right. */

        /* Update the xmin fields for coordinates and number of edges first. */

        if (xold == curr_bb_coord.xmin) { /* Old position at xmin. */
            if (curr_bb_edge.xmin == 1) {
                return false;
            } else {
                bb_coord_new.xmin = curr_bb_coord.xmin;
            }
        } else { /* Move to right, old position was not at xmin. */
            bb_coord_new.xmin = curr_bb_coord.xmin;
        }
        /* Now do the xmax fields for coordinates and number of edges. */

        if (xnew > curr_bb_coord.xmax) { /* Moved past xmax. */
            bb_coord_new.xmax = xnew;
        } else if (xnew == curr_bb_coord.xmax) { /* Moved to xmax */
            bb_coord_new.xmax = xnew;
        } else { /* Xmax unchanged. */
            bb_coord_new.xmax = curr_bb_coord.xmax;
        }
        /* End of move to right case. */

    } else { /* xnew == xold -- no x motion. */
        bb_coord_new.xmin = curr_bb_coord.xmin;
        bb_coord_new.xmax = curr_bb_coord.xmax;
    }

    /* Now account for the y-direction motion. */

    if (ynew < yold) { /* Move down. */

        /* Update the ymax fields for coordinates and number of edges first. */

        if (yold == curr_bb_coord.ymax) { /* Old position at ymax. */
            if (curr_bb_edge.ymax == 1) {
                return false;
            } else {
                bb_coord_new.ymax = curr_bb_coord.ymax;
            }
        } else { /* Move down, old postion was not at ymax. */
            bb_coord_new.ymax = curr_bb_coord.ymax;
        }

        /* Now do the ymin fields for coordinates and number of edges. */

        if (ynew < curr_bb_coord.ymin) { /* Moved past ymin */
            bb_coord_new.ymin = ynew;
        } else if (ynew == curr_bb_coord.ymin) { /* Moved to ymin */
            bb_coord_new.ymin = ynew;
        } else { /* ymin unchanged. */
            bb_coord_new.ymin = curr_bb_coord.ymin;
        }
        /* End of move down case. */

    } else if (ynew > yold) { /* Moved up. */

        /* Update the ymin fields for coordinates and number of edges first. */

        if (yold == curr_bb_coord.ymin) { /* Old position at ymin. */
            if (curr_bb_edge.ymin == 1) {
                return false;
            } else {
                bb_coord_new.ymin = curr_bb_coord.ymin;
            }
        } else { /* Moved up, old position was not at ymin. */
            bb_coord_new.ymin = curr_bb_coord.ymin;
        }

        /* Now do the ymax fields for coordinates and number of edges. */

        if (ynew > curr_bb_coord.ymax) { /* Moved past ymax. */
            bb_coord_new.ymax = ynew;
        } else if (ynew == curr_bb_coord.ymax) { /* Moved to ymax */
            bb_coord_new.ymax = ynew;
        } else { /* ymax unchanged. */
            bb_coord_new.ymax = curr_bb_coord.ymax;
        }
        /* End of move up case. */

    } else { /* ynew == yold -- no y motion. */
        bb_coord_new.ymin = curr_bb_coord.ymin;
        bb_coord_new.ymax = curr_bb_coord.ymax;
    }

    if (layer_new < layer_old) {
        if (layer_old == curr_bb_coord.layer_max) {
            if (curr_bb_edge.layer_max == 1) {
                return false;
            } else {
                bb_coord_new.layer_max = curr_bb_coord.layer_max;
            }
        } else {
            bb_coord_new.layer_max = curr_bb_coord.layer_max;
        }

        if (layer_new < curr_bb_coord.layer_min) {
            bb_coord_new.layer_min = layer_new;
        } else if (layer_new == curr_bb_coord.layer_min) {
            bb_coord_new.layer_min = layer_new;
        } else {
            bb_coord_new.layer_min = curr_bb_coord.layer_min;
        }

    } else if (layer_new > layer_old) {
        if (layer_old == curr_bb_coord.layer_min) {
            if (curr_bb_edge.layer_min == 1) {
                return false;
            } else {
                bb_coord_new.layer_min = curr_bb_coord.layer_min;
            }
        } else {
            bb_coord_new.layer_min = curr_bb_coord.layer_min;
        }

        if (layer_new > curr_bb_coord.layer_max) {
            bb_coord_new.layer_max = layer_new;
        } else if (layer_new == curr_bb_coord.layer_max) {
            bb_coord_new.layer_max = layer_new;
        } else {
            bb_coord_new.layer_max = curr_bb_coord.layer_max;
        }
    } else {
        bb_coord_new.layer_min = curr_bb_coord.layer_min;
        bb_coord_new.layer_max = curr_bb_coord.layer_max;
    }
    return true;
}
