/**
 * @file place_util.cpp
 * @brief Definitions of structure methods and routines declared in place_util.h.
 *        These are mainly utility functions used by the placer.
 */

#include "place_util.h"
#include "globals.h"
#include "draw_global.h"
#include "place_constraints.h"

/* Expected crossing counts for nets with different #'s of pins.  From *
 * ICCAD 94 pp. 690 - 695 (with linear interpolation applied by me).   *
 * Multiplied to bounding box of a net to better estimate wire length  *
 * for higher fanout nets. Each entry is the correction factor for the *
 * fanout index-1                                                      */
static const float cross_count[50] = {/* [0..49] */ 1.0, 1.0, 1.0, 1.0828,
                                      1.1536, 1.2206, 1.2823, 1.3385, 1.3991, 1.4493, 1.4974, 1.5455, 1.5937,
                                      1.6418, 1.6899, 1.7304, 1.7709, 1.8114, 1.8519, 1.8924, 1.9288, 1.9652,
                                      2.0015, 2.0379, 2.0743, 2.1061, 2.1379, 2.1698, 2.2016, 2.2334, 2.2646,
                                      2.2958, 2.3271, 2.3583, 2.3895, 2.4187, 2.4479, 2.4772, 2.5064, 2.5356,
                                      2.5610, 2.5864, 2.6117, 2.6371, 2.6625, 2.6887, 2.7148, 2.7410, 2.7671,
                                      2.7933};

/* The arrays below are used to precompute the inverse of the average   *
 * number of tracks per channel between [subhigh] and [sublow].  Access *
 * them as chan?_place_cost_fac[subhigh][sublow].  They are used to     *
 * speed up the computation of the cost function that takes the length  *
 * of the net bounding box in each dimension, divided by the average    *
 * number of tracks in that direction; for other cost functions they    *
 * will never be used.                                                  *
 */
static vtr::NdMatrix<float, 2> chanx_place_cost_fac({0, 0}); //[0...device_ctx.grid.width()-2]
static vtr::NdMatrix<float, 2> chany_place_cost_fac({0, 0}); //[0...device_ctx.grid.height()-2]

/* File-scope routines */
static GridBlock init_grid_blocks();

/**
 * @brief Initialize the placer's block-grid dual direction mapping.
 *
 * Forward direction - block to grid: place_ctx.block_locs.
 * Reverse direction - grid to block: place_ctx.grid_blocks.
 *
 * Initialize both of them to empty states.
 */
void init_placement_context() {
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    /* Intialize the lookup of CLB block positions */
    place_ctx.block_locs.clear();
    place_ctx.block_locs.resize(cluster_ctx.clb_nlist.blocks().size());

    /* Initialize the reverse lookup of CLB block positions */
    place_ctx.grid_blocks = init_grid_blocks();
}

/**
 * @brief Initialize `grid_blocks`, the inverse structure of `block_locs`.
 *
 * The container at each grid block location should have a length equal to the
 * subtile capacity of that block. Unused subtile would be marked EMPTY_BLOCK_ID.
 */
static GridBlock init_grid_blocks() {
    auto& device_ctx = g_vpr_ctx.device();
    int num_layers = device_ctx.grid.get_num_layers();

    /* Structure should have the same dimensions as the grid. */
    auto grid_blocks = GridBlock(device_ctx.grid.width(), device_ctx.grid.height(), num_layers);

    for (int layer_num = 0; layer_num < num_layers; ++layer_num) {
        for (int x = 0; x < (int)device_ctx.grid.width(); ++x) {
            for (int y = 0; y < (int)device_ctx.grid.height(); ++y) {
                auto type = device_ctx.grid.get_physical_type({x, y, layer_num});
                grid_blocks.initialized_grid_block_at_location({x, y, layer_num}, type->capacity);
            }
        }
    }
    return grid_blocks;
}

/**
 * @brief Mutator: updates the norm factors in the outer loop iteration.
 *
 * At each temperature change we update these values to be used
 * for normalizing the trade-off between timing and wirelength (bb)
 */
void t_placer_costs::update_norm_factors() {
    if (place_algorithm.is_timing_driven()) {
        bb_cost_norm = 1 / bb_cost;
        //Prevent the norm factor from going to infinity
        timing_cost_norm = std::min(1 / timing_cost, MAX_INV_TIMING_COST);
    } else {
        VTR_ASSERT_SAFE(place_algorithm == BOUNDING_BOX_PLACE);
        bb_cost_norm = 1 / bb_cost; //Upading the normalization factor in bounding box mode since the cost in this mode is determined after normalizing the wirelength cost
    }
}

///@brief Constructor: Initialize all annealing state variables and macros.
t_annealing_state::t_annealing_state(const t_annealing_sched& annealing_sched,
                                     float first_t,
                                     float first_rlim,
                                     int first_move_lim,
                                     float first_crit_exponent,
                                     int num_laters) {
    num_temps = 0;
    alpha = annealing_sched.alpha_min;
    t = first_t;
    restart_t = first_t;
    rlim = first_rlim;
    move_lim_max = first_move_lim;
    crit_exponent = first_crit_exponent;

    /* Determine the current move_lim based on the schedule type */
    if (annealing_sched.type == DUSTY_SCHED) {
        move_lim = std::max(1, (int)(move_lim_max * annealing_sched.success_target));
    } else {
        move_lim = move_lim_max;
    }

    NUM_LAYERS = num_laters;

    /* Store this inverse value for speed when updating crit_exponent. */
    INVERSE_DELTA_RLIM = 1 / (first_rlim - FINAL_RLIM);

    /* The range limit cannot exceed the largest grid size. */
    auto& grid = g_vpr_ctx.device().grid;
    UPPER_RLIM = std::max(grid.width() - 1, grid.height() - 1);
}

/**
 * @brief Get the initial limit for inner loop block move attempt limit.
 *
 * There are two ways to scale the move limit.
 * e_place_effort_scaling::CIRCUIT
 *      scales the move limit proportional to num_blocks ^ (4/3)
 * e_place_effort_scaling::DEVICE_CIRCUIT
 *      scales the move limit proportional to device_size ^ (2/3) * num_blocks ^ (2/3)
 *
 * The second method is almost identical to the first one when the device
 * is highly utilized (device_size ~ num_blocks). For low utilization devices
 * (device_size >> num_blocks), the search space is larger, so the second method
 * performs more moves to ensure better optimization.
 */
int get_initial_move_lim(const t_placer_opts& placer_opts, const t_annealing_sched& annealing_sched) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& cluster_ctx = g_vpr_ctx.clustering();

    float device_size = device_ctx.grid.width() * device_ctx.grid.height();
    size_t num_blocks = cluster_ctx.clb_nlist.blocks().size();

    int move_lim;
    if (placer_opts.effort_scaling == e_place_effort_scaling::CIRCUIT) {
        move_lim = int(annealing_sched.inner_num * pow(num_blocks, 1.3333));
    } else {
        VTR_ASSERT(placer_opts.effort_scaling == e_place_effort_scaling::DEVICE_CIRCUIT);
        move_lim = int(annealing_sched.inner_num * pow(device_size, 2. / 3.) * pow(num_blocks, 2. / 3.));
    }

    /* Avoid having a non-positive move_lim */
    move_lim = std::max(move_lim, 1);

    VTR_LOG("Moves per temperature: %d\n", move_lim);

    return move_lim;
}

/**
 * @brief Update the annealing state according to the annealing schedule selected.
 *
 *   USER_SCHED:  A manual fixed schedule with fixed alpha and exit criteria.
 *   AUTO_SCHED:  A more sophisticated schedule where alpha varies based on success ratio.
 *   DUSTY_SCHED: This schedule jumps backward and slows down in response to success ratio.
 *                See doc/src/vpr/dusty_sa.rst for more details.
 *
 * @return True->continues the annealing. False->exits the annealing.
 */
bool t_annealing_state::outer_loop_update(float success_rate,
                                          const t_placer_costs& costs,
                                          const t_placer_opts& placer_opts,
                                          const t_annealing_sched& annealing_sched) {
#ifndef NO_GRAPHICS
    t_draw_state* draw_state = get_draw_state_vars();
    if (draw_state->list_of_breakpoints.size() != 0) {
        /* Update temperature in the current information variable. */
        get_bp_state_globals()->get_glob_breakpoint_state()->temp_count++;
    }
#endif

    if (annealing_sched.type == USER_SCHED) {
        /* Update t with user specified alpha. */
        t *= annealing_sched.alpha_t;

        /* Check if the exit criterion is met. */
        bool exit_anneal = t >= annealing_sched.exit_t;

        return exit_anneal;
    }

    /* Automatically determine exit temperature. */
    auto& cluster_ctx = g_vpr_ctx.clustering();
    float t_exit = 0.005 * costs.cost / cluster_ctx.clb_nlist.nets().size();

    if (annealing_sched.type == DUSTY_SCHED) {
        /* May get nan if there are no nets */
        bool restart_temp = t < t_exit || std::isnan(t_exit);

        /* If the success rate or the temperature is *
         * too low, reset the temperature and alpha. */
        if (success_rate < annealing_sched.success_min || restart_temp) {
            /* Only exit anneal when alpha gets too large. */
            if (alpha > annealing_sched.alpha_max) {
                return false;
            }
            /* Take a half step from the restart temperature. */
            t = restart_t / sqrt(alpha);
            /* Update alpha. */
            alpha = 1.0 - ((1.0 - alpha) * annealing_sched.alpha_decay);
        } else {
            /* If the success rate is promising, next time   *
             * reset t to the current annealing temperature. */
            if (success_rate > annealing_sched.success_target) {
                restart_t = t;
            }
            /* Update t. */
            t *= alpha;
        }

        /* Update move lim. */
        update_move_lim(annealing_sched.success_target, success_rate);
    } else {
        VTR_ASSERT_SAFE(annealing_sched.type == AUTO_SCHED);
        /* Automatically adjust alpha according to success rate. */
        if (success_rate > 0.96) {
            alpha = 0.5;
        } else if (success_rate > 0.8) {
            alpha = 0.9;
        } else if (success_rate > 0.15 || rlim > 1.) {
            alpha = 0.95;
        } else {
            alpha = 0.8;
        }
        /* Update temp. */
        t *= alpha;
        /* Must be duplicated to retain previous behavior. */
        if (t < t_exit || std::isnan(t_exit)) {
            return false;
        }
    }

    /* Update the range limiter. */
    update_rlim(success_rate);

    /* If using timing driven algorithm, update the crit_exponent. */
    if (placer_opts.place_algorithm.is_timing_driven()) {
        update_crit_exponent(placer_opts);
    }

    /* Continues the annealing. */
    return true;
}

/**
 * @brief Update the range limiter to keep acceptance prob. near 0.44.
 *
 * Use a floating point rlim to allow gradual transitions at low temps.
 * The range is bounded by 1 (FINAL_RLIM) and the grid size (UPPER_RLIM).
 */
void t_annealing_state::update_rlim(float success_rate) {
    rlim *= (1. - 0.44 + success_rate);
    rlim = std::min(rlim, UPPER_RLIM);
    rlim = std::max(rlim, FINAL_RLIM);
}

/**
 * @brief Update the criticality exponent.
 *
 * When rlim shrinks towards the FINAL_RLIM value (indicating
 * that we are fine-tuning a more optimized placement), we can
 * focus more on a smaller number of critical connections.
 * To achieve this, we make the crit_exponent sharper, so that
 * critical connections would become more critical than before.
 *
 * We calculate how close rlim is to its final value comparing
 * to its initial value. Then, we apply the same scaling factor
 * on the crit_exponent so that it lands on the suitable value
 * between td_place_exp_first and td_place_exp_last. The scaling
 * factor is calculated and applied linearly.
 */
void t_annealing_state::update_crit_exponent(const t_placer_opts& placer_opts) {
    /* If rlim == FINAL_RLIM, then scale == 0. */
    float scale = 1 - (rlim - FINAL_RLIM) * INVERSE_DELTA_RLIM;

    /* Apply the scaling factor on crit_exponent. */
    crit_exponent = scale * (placer_opts.td_place_exp_last - placer_opts.td_place_exp_first)
                    + placer_opts.td_place_exp_first;
}

/**
 * @brief Update the move limit based on the success rate.
 *
 * The value is bounded between 1 and move_lim_max.
 */
void t_annealing_state::update_move_lim(float success_target, float success_rate) {
    move_lim = move_lim_max * (success_target / success_rate);
    move_lim = std::min(move_lim, move_lim_max);
    move_lim = std::max(move_lim, 1);
}

///@brief Clear all data fields.
void t_placer_statistics::reset() {
    av_cost = 0.;
    av_bb_cost = 0.;
    av_timing_cost = 0.;
    sum_of_squares = 0.;
    success_sum = 0;
    success_rate = 0.;
    std_dev = 0.;
}

///@brief Calculate placer success rate and cost std_dev for this iteration.
void t_placer_statistics::single_swap_update(const t_placer_costs& costs) {
    success_sum++;
    av_cost += costs.cost;
    av_bb_cost += costs.bb_cost;
    av_timing_cost += costs.timing_cost;
    sum_of_squares += (costs.cost) * (costs.cost);
}

///@brief Update stats when a single swap move has been accepted.
void t_placer_statistics::calc_iteration_stats(const t_placer_costs& costs, int move_lim) {
    if (success_sum == 0) {
        av_cost = costs.cost;
        av_bb_cost = costs.bb_cost;
        av_timing_cost = costs.timing_cost;
    } else {
        av_cost /= success_sum;
        av_bb_cost /= success_sum;
        av_timing_cost /= success_sum;
    }
    success_rate = success_sum / float(move_lim);
    std_dev = get_std_dev(success_sum, sum_of_squares, av_cost);
}

/**
 * @brief Returns the standard deviation of data set x.
 *
 * There are n sample points, sum_x_squared is the summation over n of x^2 and av_x
 * is the average x. All operations are done in double precision, since round off
 * error can be a problem in the initial temp. std_dev calculation for big circuits.
 */
double get_std_dev(int n, double sum_x_squared, double av_x) {
    double std_dev;
    if (n <= 1) {
        std_dev = 0.;
    } else {
        std_dev = (sum_x_squared - n * av_x * av_x) / (double)(n - 1);
    }

    /* Very small variances sometimes round negative. */
    return (std_dev > 0.) ? sqrt(std_dev) : 0.;
}

void load_grid_blocks_from_block_locs() {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    auto& device_ctx = g_vpr_ctx.device();

    zero_initialize_grid_blocks();

    auto blocks = cluster_ctx.clb_nlist.blocks();
    for (auto blk_id : blocks) {
        t_pl_loc location;
        location = place_ctx.block_locs[blk_id].loc;

        VTR_ASSERT(location.x < (int)device_ctx.grid.width());
        VTR_ASSERT(location.y < (int)device_ctx.grid.height());

        place_ctx.grid_blocks.set_block_at_location(location, blk_id);
        place_ctx.grid_blocks.set_usage({location.x, location.y, location.layer},
                                        place_ctx.grid_blocks.get_usage({location.x, location.y, location.layer}) + 1);
    }
}

void zero_initialize_grid_blocks() {
    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    /* Initialize all occupancy to zero. */

    for (int layer_num = 0; layer_num < (int)device_ctx.grid.get_num_layers(); layer_num++) {
        for (int i = 0; i < (int)device_ctx.grid.width(); i++) {
            for (int j = 0; j < (int)device_ctx.grid.height(); j++) {
                place_ctx.grid_blocks.set_usage({i, j, layer_num}, 0);
                auto tile = device_ctx.grid.get_physical_type({i, j, layer_num});

                for (auto sub_tile : tile->sub_tiles) {
                    auto capacity = sub_tile.capacity;

                    for (int k = 0; k < capacity.total(); k++) {
                        if (place_ctx.grid_blocks.block_at_location({i, j, k + capacity.low, layer_num}) != INVALID_BLOCK_ID) {
                            place_ctx.grid_blocks.set_block_at_location({i, j, k + capacity.low, layer_num}, EMPTY_BLOCK_ID);
                        }
                    }
                }
            }
        }
    }
}

/**
 * @brief Builds (alloc and load) legal_pos that holds all the legal locations for placement
 *
 *   @param legal_pos
 *              a lookup of all subtiles by sub_tile type
 *              legal_pos[0..device_ctx.num_block_types-1][0..num_sub_tiles - 1] = std::vector<t_pl_loc> of all the legal locations 
 *              of the proper tile type and sub_tile type
 *
 */
void alloc_and_load_legal_placement_locations(std::vector<std::vector<std::vector<t_pl_loc>>>& legal_pos) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.placement();

    //alloc the legal placement positions
    int num_tile_types = device_ctx.physical_tile_types.size();
    legal_pos.resize(num_tile_types);

    for (const auto& type : device_ctx.physical_tile_types) {
        legal_pos[type.index].resize(type.sub_tiles.size());
    }

    //load the legal placement positions
    for (int layer_num = 0; layer_num < device_ctx.grid.get_num_layers(); layer_num++) {
        for (int i = 0; i < (int)device_ctx.grid.width(); i++) {
            for (int j = 0; j < (int)device_ctx.grid.height(); j++) {
                auto tile = device_ctx.grid.get_physical_type({i, j, layer_num});

                for (const auto& sub_tile : tile->sub_tiles) {
                    auto capacity = sub_tile.capacity;

                    for (int k = 0; k < capacity.total(); k++) {
                        if (place_ctx.grid_blocks.block_at_location({i, j, k + capacity.low, layer_num}) == INVALID_BLOCK_ID) {
                            continue;
                        }
                        // If this is the anchor position of a block, add it to the legal_pos.
                        // Otherwise don't, so large blocks aren't added multiple times.
                        if (device_ctx.grid.get_width_offset({i, j, layer_num}) == 0 && device_ctx.grid.get_height_offset({i, j, layer_num}) == 0) {
                            int itype = tile->index;
                            int isub_tile = sub_tile.index;
                            t_pl_loc temp_loc;
                            temp_loc.x = i;
                            temp_loc.y = j;
                            temp_loc.sub_tile = k + capacity.low;
                            temp_loc.layer = layer_num;
                            legal_pos[itype][isub_tile].push_back(temp_loc);
                        }
                    }
                }
            }
        }
    }
    //avoid any memory waste
    legal_pos.shrink_to_fit();
}

void set_block_location(ClusterBlockId blk_id, const t_pl_loc& location) {
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    const std::string& block_name = cluster_ctx.clb_nlist.block_name(blk_id);

    //Check if block location is out of range of grid dimensions
    if (location.x < 0 || location.x > int(device_ctx.grid.width() - 1)
        || location.y < 0 || location.y > int(device_ctx.grid.height() - 1)) {
        VPR_THROW(VPR_ERROR_PLACE, "Block %s with ID %d is out of range at location (%d, %d). \n", block_name.c_str(), blk_id, location.x, location.y);
    }

    //Set the location of the block
    place_ctx.block_locs[blk_id].loc = location;

    //Check if block is at an illegal location
    auto physical_tile = device_ctx.grid.get_physical_type({location.x, location.y, location.layer});
    auto logical_block = cluster_ctx.clb_nlist.block_type(blk_id);

    if (location.sub_tile >= physical_tile->capacity || location.sub_tile < 0) {
        VPR_THROW(VPR_ERROR_PLACE, "Block %s subtile number (%d) is out of range. \n", block_name.c_str(), location.sub_tile);
    }

    if (!is_sub_tile_compatible(physical_tile, logical_block, place_ctx.block_locs[blk_id].loc.sub_tile)) {
        VPR_THROW(VPR_ERROR_PLACE, "Attempt to place block %s with ID %d at illegal location (%d,%d,%d). \n",
                  block_name.c_str(),
                  blk_id,
                  location.x,
                  location.y,
                  location.layer);
    }

    //Mark the grid location and usage of the block
    place_ctx.grid_blocks.set_block_at_location(location, blk_id);
    place_ctx.grid_blocks.set_usage({location.x, location.y, location.layer},
                                    place_ctx.grid_blocks.get_usage({location.x, location.y, location.layer}) + 1);
    place_sync_external_block_connections(blk_id);
}

bool macro_can_be_placed(t_pl_macro pl_macro, t_pl_loc head_pos, bool check_all_legality) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    //Get block type of head member
    ClusterBlockId blk_id = pl_macro.members[0].blk_index;
    auto block_type = cluster_ctx.clb_nlist.block_type(blk_id);

    // Every macro can be placed until proven otherwise
    bool mac_can_be_placed = true;

    // Check whether all the members can be placed
    for (size_t imember = 0; imember < pl_macro.members.size(); imember++) {
        t_pl_loc member_pos = head_pos + pl_macro.members[imember].offset;

        //Check that the member location is on the grid
        if (!is_loc_on_chip({member_pos.x, member_pos.y, member_pos.layer})) {
            mac_can_be_placed = false;
            break;
        }

        /*
         * analytical placement approach do not need to make sure whether location could accommodate more blocks
         * since overused locations will be spreaded by legalizer afterward.
         * floorplan constraint is not supported by analytical placement yet, 
         * hence, if macro_can_be_placed is called from analytical placer, no further actions are required. 
         */
        if (check_all_legality) {
            continue;
        }

        //Check whether macro contains blocks with floorplan constraints
        bool macro_constrained = is_macro_constrained(pl_macro);

        /*
         * If the macro is constrained, check that the head member is in a legal position from
         * a floorplanning perspective. It is enough to do this check for the head member alone,
         * because constraints propagation was performed to calculate smallest floorplan region for the head
         * macro, based on the constraints on all of the blocks in the macro. So, if the head macro is in a
         * legal floorplan location, all other blocks in the macro will be as well.
         */
        if (macro_constrained && imember == 0) {
            bool member_loc_good = cluster_floorplanning_legal(pl_macro.members[imember].blk_index, member_pos);
            if (!member_loc_good) {
                mac_can_be_placed = false;
                break;
            }
        }

        // Check whether the location could accept block of this type
        // Then check whether the location could still accommodate more blocks
        // Also check whether the member position is valid, and the member_z is allowed at that location on the grid
        if (member_pos.x < int(device_ctx.grid.width()) && member_pos.y < int(device_ctx.grid.height())
            && is_tile_compatible(device_ctx.grid.get_physical_type({member_pos.x, member_pos.y, member_pos.layer}), block_type)
            && place_ctx.grid_blocks.block_at_location(member_pos) == EMPTY_BLOCK_ID) {
            // Can still accommodate blocks here, check the next position
            continue;
        } else {
            // Cant be placed here - skip to the next try
            mac_can_be_placed = false;
            break;
        }
    }

    return (mac_can_be_placed);
}

t_pl_atom_loc get_atom_loc (AtomBlockId atom) {
    const auto& atom_lookup = g_vpr_ctx.atom().lookup;
    ClusterBlockId cluster_blk = atom_lookup.atom_clb(atom);
    t_pl_loc cluster_loc = g_vpr_ctx.placement().block_locs[cluster_blk].loc;
    int primitive_id = atom_lookup.atom_pb_graph_node(atom)->primitive_num;

    return {primitive_id, cluster_loc.x, cluster_loc.y, cluster_loc.sub_tile, cluster_loc.layer};
}

void alloc_and_load_for_fast_cost_update(float place_cost_exp) {
    /* Allocates and loads the chanx_place_cost_fac and chany_place_cost_fac *
     * arrays with the inverse of the average number of tracks per channel   *
     * between [subhigh] and [sublow].  This is only useful for the cost     *
     * function that takes the length of the net bounding box in each        *
     * dimension divided by the average number of tracks in that direction.  *
     * For other cost functions, you don't have to bother calling this       *
     * routine; when using the cost function described above, however, you   *
     * must always call this routine after you call init_chan and before     *
     * you do any placement cost determination.  The place_cost_exp factor   *
     * specifies to what power the width of the channel should be taken --   *
     * larger numbers make narrower channels more expensive.                 */

    auto& device_ctx = g_vpr_ctx.device();

    /* Access arrays below as chan?_place_cost_fac[subhigh][sublow].  Since   *
     * subhigh must be greater than or equal to sublow, we only need to       *
     * allocate storage for the lower half of a matrix.                       */

    //chanx_place_cost_fac = new float*[(device_ctx.grid.height())];
    //for (size_t i = 0; i < device_ctx.grid.height(); i++)
    //    chanx_place_cost_fac[i] = new float[(i + 1)];

    //chany_place_cost_fac = new float*[(device_ctx.grid.width() + 1)];
    //for (size_t i = 0; i < device_ctx.grid.width(); i++)
    //    chany_place_cost_fac[i] = new float[(i + 1)];

    chanx_place_cost_fac.resize({device_ctx.grid.height(), device_ctx.grid.height() + 1});
    chany_place_cost_fac.resize({device_ctx.grid.width(), device_ctx.grid.width() + 1});

    /* First compute the number of tracks between channel high and channel *
     * low, inclusive, in an efficient manner.                             */

    chanx_place_cost_fac[0][0] = device_ctx.chan_width.x_list[0];

    for (size_t high = 1; high < device_ctx.grid.height(); high++) {
        chanx_place_cost_fac[high][high] = device_ctx.chan_width.x_list[high];
        for (size_t low = 0; low < high; low++) {
            chanx_place_cost_fac[high][low] = chanx_place_cost_fac[high - 1][low]
                                              + device_ctx.chan_width.x_list[high];
        }
    }

    /* Now compute the inverse of the average number of tracks per channel *
     * between high and low.  The cost function divides by the average     *
     * number of tracks per channel, so by storing the inverse I convert   *
     * this to a faster multiplication.  Take this final number to the     *
     * place_cost_exp power -- numbers other than one mean this is no      *
     * longer a simple "average number of tracks"; it is some power of     *
     * that, allowing greater penalization of narrow channels.             */

    for (size_t high = 0; high < device_ctx.grid.height(); high++)
        for (size_t low = 0; low <= high; low++) {
            /* Since we will divide the wiring cost by the average channel *
             * capacity between high and low, having only 0 width channels *
             * will result in infinite wiring capacity normalization       *
             * factor, and extremely bad placer behaviour. Hence we change *
             * this to a small (1 track) channel capacity instead.         */
            if (chanx_place_cost_fac[high][low] == 0.0f) {
                VTR_LOG_WARN("CHANX place cost fac is 0 at %d %d\n", high, low);
                chanx_place_cost_fac[high][low] = 1.0f;
            }

            chanx_place_cost_fac[high][low] = (high - low + 1.)
                                              / chanx_place_cost_fac[high][low];
            chanx_place_cost_fac[high][low] = pow(
                (double)chanx_place_cost_fac[high][low],
                (double)place_cost_exp);
        }

    /* Now do the same thing for the y-directed channels.  First get the  *
     * number of tracks between channel high and channel low, inclusive.  */

    chany_place_cost_fac[0][0] = device_ctx.chan_width.y_list[0];

    for (size_t high = 1; high < device_ctx.grid.width(); high++) {
        chany_place_cost_fac[high][high] = device_ctx.chan_width.y_list[high];
        for (size_t low = 0; low < high; low++) {
            chany_place_cost_fac[high][low] = chany_place_cost_fac[high - 1][low]
                                              + device_ctx.chan_width.y_list[high];
        }
    }

    /* Now compute the inverse of the average number of tracks per channel *
     * between high and low.  Take to specified power.                     */

    for (size_t high = 0; high < device_ctx.grid.width(); high++)
        for (size_t low = 0; low <= high; low++) {
            /* Since we will divide the wiring cost by the average channel *
             * capacity between high and low, having only 0 width channels *
             * will result in infinite wiring capacity normalization       *
             * factor, and extremely bad placer behaviour. Hence we change *
             * this to a small (1 track) channel capacity instead.         */
            if (chany_place_cost_fac[high][low] == 0.0f) {
                VTR_LOG_WARN("CHANY place cost fac is 0 at %d %d\n", high, low);
                chany_place_cost_fac[high][low] = 1.0f;
            }

            chany_place_cost_fac[high][low] = (high - low + 1.)
                                              / chany_place_cost_fac[high][low];
            chany_place_cost_fac[high][low] = pow(
                (double)chany_place_cost_fac[high][low],
                (double)place_cost_exp);
        }
}
