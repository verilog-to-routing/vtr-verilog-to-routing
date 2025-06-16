#include "weighted_median_move_generator.h"

#include "globals.h"
#include "physical_types_util.h"
#include "place_constraints.h"
#include "place_macro.h"
#include "placer_state.h"
#include "move_utils.h"

#include <algorithm>
#include <cmath>

#define CRIT_MULT_FOR_W_MEDIAN 10

WeightedMedianMoveGenerator::WeightedMedianMoveGenerator(PlacerState& placer_state,
                                                         const PlaceMacros& place_macros,
                                                         const NetCostHandler& net_cost_handler,
                                                         e_reward_function reward_function,
                                                         vtr::RngContainer& rng)
    : MoveGenerator(placer_state, place_macros, net_cost_handler, reward_function, rng) {}

e_create_move WeightedMedianMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected,
                                                        t_propose_action& proposed_action,
                                                        float rlim,
                                                        const t_placer_opts& placer_opts,
                                                        const PlacerCriticalities* criticalities) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& placer_state = placer_state_.get();
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

    VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "Weighted Median Move Choose Block %d - rlim %f\n", size_t(b_from), rlim);

    if (!b_from) { //No movable block found
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\tNo movable block found\n");
        return e_create_move::ABORT;
    }

    int num_layers = g_vpr_ctx.device().grid.get_num_layers();

    t_pl_loc from = block_locs[b_from].loc;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = g_vpr_ctx.device().grid.get_physical_type({from.x, from.y, from.layer});
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));

    /* Calculate the Edge weighted median region */
    t_pl_loc to;

    t_bb_cost coords;
    t_bb limit_coords;

    //clear the vectors that saves X & Y coords
    //reused to save allocation time
    X_coord.clear();
    Y_coord.clear();
    layer_coord.clear();
    std::vector<int> layer_blk_cnt(num_layers, 0);

    //iterate over block pins
    for (ClusterPinId pin_id : cluster_ctx.clb_nlist.block_pins(b_from)) {
        ClusterNetId net_id = cluster_ctx.clb_nlist.pin_net(pin_id);
        if (cluster_ctx.clb_nlist.net_is_ignored(net_id))
            continue;
        if (int(cluster_ctx.clb_nlist.net_pins(net_id).size()) > placer_opts.place_high_fanout_net)
            continue;
        /**
         * Calculate the bounding box edges and the cost of each edge.
         *
         * Note: skip_net returns true if this net should be skipped. Currently, the only case to skip a net is the feedback nets
         *       (a net that all its terminals connected to the same block). Logically, this net should be neglected as it is only connected 
         *       to the moving block. Experimentally, we found that including these nets into calculations badly affect the overall placement
         *       solution especillay for some large designs.
         *
         * Note: skip_net true if the net is a feedback from the block to itself (all the net terminals are connected to the same block)
         */
        bool skip_net = get_bb_cost_for_net_excluding_block(net_id, pin_id, criticalities, &coords);
        if (skip_net) {
            continue;
        }

        // We need to insert the calculated edges in the X,Y vectors multiple times based on the criticality of the pin that caused each of them.
        // As all the criticalities are [0,1], we map it to [0,CRIT_MULT_FOR_W_MEDIAN] inserts in the vectors for each edge
        // by multiplying each edge's criticality by CRIT_MULT_FOR_W_MEDIAN
        X_coord.insert(X_coord.end(), ceil(coords.xmin.criticality * CRIT_MULT_FOR_W_MEDIAN), coords.xmin.edge);
        X_coord.insert(X_coord.end(), ceil(coords.xmax.criticality * CRIT_MULT_FOR_W_MEDIAN), coords.xmax.edge);
        Y_coord.insert(Y_coord.end(), ceil(coords.ymin.criticality * CRIT_MULT_FOR_W_MEDIAN), coords.ymin.edge);
        Y_coord.insert(Y_coord.end(), ceil(coords.ymax.criticality * CRIT_MULT_FOR_W_MEDIAN), coords.ymax.edge);
        layer_coord.insert(layer_coord.end(), ceil(coords.layer_min.criticality * CRIT_MULT_FOR_W_MEDIAN), coords.layer_min.edge);
        layer_coord.insert(layer_coord.end(), ceil(coords.layer_max.criticality * CRIT_MULT_FOR_W_MEDIAN), coords.layer_max.edge);
    }

    if ((X_coord.empty()) || (Y_coord.empty()) || (layer_coord.empty())) {
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\tMove aborted - X_coord or y_coord or layer_coord are empty\n");
        return e_create_move::ABORT;
    }

    //calculate the weighted median region
    std::stable_sort(X_coord.begin(), X_coord.end());
    std::stable_sort(Y_coord.begin(), Y_coord.end());
    std::stable_sort(layer_coord.begin(), layer_coord.end());

    if (X_coord.size() == 1) {
        limit_coords.xmin = X_coord[0];
        limit_coords.xmax = limit_coords.xmin;
    } else {
        limit_coords.xmin = X_coord[((X_coord.size() - 1) / 2)];
        limit_coords.xmax = X_coord[((X_coord.size() - 1) / 2) + 1];
    }

    if (Y_coord.size() == 1) {
        limit_coords.ymin = Y_coord[0];
        limit_coords.ymax = limit_coords.ymin;
    } else {
        limit_coords.ymin = Y_coord[((Y_coord.size() - 1) / 2)];
        limit_coords.ymax = Y_coord[((Y_coord.size() - 1) / 2) + 1];
    }

    if (layer_coord.size() == 1) {
        limit_coords.layer_min = layer_coord[0];
        limit_coords.layer_max = limit_coords.layer_min;
    } else {
        limit_coords.layer_min = layer_coord[((layer_coord.size() - 1) / 2)];
        limit_coords.layer_max = layer_coord[((layer_coord.size() - 1) / 2) + 1];
    }

    t_range_limiters range_limiters{rlim,
                                    first_rlim,
                                    placer_opts.place_dm_rlim};

    t_pl_loc w_median_point;
    w_median_point.x = (limit_coords.xmin + limit_coords.xmax) / 2;
    w_median_point.y = (limit_coords.ymin + limit_coords.ymax) / 2;
    w_median_point.layer = ((limit_coords.layer_min + limit_coords.layer_max) / 2);

    if (!find_to_loc_centroid(cluster_from_type, from, w_median_point, range_limiters, to, b_from, blk_loc_registry, rng_)) {
        return e_create_move::ABORT;
    }

    e_create_move create_move = ::create_move(blocks_affected, b_from, to, blk_loc_registry, place_macros_);

    //Check that all the blocks affected by the move would still be in a legal floorplan region after the swap
    if (!floorplan_legal(blocks_affected)) {
        return e_create_move::ABORT;
    }

    return create_move;
}

bool WeightedMedianMoveGenerator::get_bb_cost_for_net_excluding_block(ClusterNetId net_id,
                                                                      ClusterPinId moving_pin_id,
                                                                      const PlacerCriticalities* criticalities,
                                                                      t_bb_cost* coords) {
    const auto& blk_loc_registry = placer_state_.get().blk_loc_registry();
    const auto& cluster_ctx = g_vpr_ctx.clustering();

    bool skip_net = true;

    int xmin = 0;
    int xmax = 0;
    int ymin = 0;
    int ymax = 0;
    int layer_min = 0;
    int layer_max = 0;

    float xmin_cost = 0.f;
    float xmax_cost = 0.f;
    float ymin_cost = 0.f;
    float ymax_cost = 0.f;
    float layer_min_cost = 0.f;
    float layer_max_cost = 0.f;

    bool is_first_block = true;
    for (ClusterPinId pin_id : cluster_ctx.clb_nlist.net_pins(net_id)) {
        if (pin_id != moving_pin_id) {
            skip_net = false;
            /**
             * Calculates the pin index of the correct pin to calculate the required connection
             *
             * if the current pin is the driver, we only care about one sink (the moving pin)
             * else if the current pin is a sink, calculate the criticality of itself
             */
            int ipin;
            if (cluster_ctx.clb_nlist.pin_type(pin_id) == PinType::DRIVER) {
                ipin = cluster_ctx.clb_nlist.pin_net_index(moving_pin_id);
            } else {
                ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
            }
            float cost = criticalities->criticality(net_id, ipin);

            t_physical_tile_loc pin_loc = blk_loc_registry.get_coordinate_of_pin(pin_id);

            if (is_first_block) {
                xmin = pin_loc.x;
                xmin_cost = cost;
                ymin = pin_loc.y;
                ymin_cost = cost;
                xmax = pin_loc.x;
                xmax_cost = cost;
                ymax = pin_loc.y;
                ymax_cost = cost;
                layer_min = pin_loc.layer_num;
                layer_min_cost = cost;
                layer_max = pin_loc.layer_num;
                layer_max_cost = cost;
                is_first_block = false;
            } else {
                if (pin_loc.x < xmin) {
                    xmin = pin_loc.x;
                    xmin_cost = cost;
                } else if (pin_loc.x > xmax) {
                    xmax = pin_loc.x;
                    xmax_cost = cost;
                }

                if (pin_loc.y < ymin) {
                    ymin = pin_loc.y;
                    ymin_cost = cost;
                } else if (pin_loc.y > ymax) {
                    ymax = pin_loc.y;
                    ymax_cost = cost;
                }

                if (pin_loc.layer_num < layer_min) {
                    layer_min = pin_loc.layer_num;
                    layer_min_cost = cost;
                } else if (pin_loc.layer_num > layer_max) {
                    layer_max = pin_loc.layer_num;
                    layer_max_cost = cost;
                } else if (pin_loc.layer_num == layer_min) {
                    if (cost > layer_min_cost)
                        layer_min_cost = cost;
                } else if (pin_loc.layer_num == layer_max) {
                    if (cost > layer_max_cost)
                        layer_max_cost = cost;
                }
            }
        }
    }

    // Copy the bounding box edges and corresponding criticalities into the proper structure
    coords->xmin = {xmin, xmin_cost};
    coords->xmax = {xmax, xmax_cost};
    coords->ymin = {ymin, ymin_cost};
    coords->ymax = {ymax, ymax_cost};
    coords->layer_min = {layer_min, layer_min_cost};
    coords->layer_max = {layer_max, layer_max_cost};

    return skip_net;
}
