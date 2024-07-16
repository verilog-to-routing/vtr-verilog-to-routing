/**
 * @file place_util.cpp
 * @brief Definitions of structure methods and routines declared in place_util.h.
 *        These are mainly utility functions used by the placer.
 */

#include "place_util.h"
#include "globals.h"
#include "draw_global.h"
#include "place_constraints.h"

/**
 * @brief Initialize `grid_blocks`, the inverse structure of `block_locs`.
 *
 * The container at each grid block location should have a length equal to the
 * subtile capacity of that block. Unused subtile would be marked EMPTY_BLOCK_ID.
 */
static GridBlock init_grid_blocks();

void init_placement_context() {
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    /* Intialize the lookup of CLB block positions */
    place_ctx.block_locs.clear();
    place_ctx.block_locs.resize(cluster_ctx.clb_nlist.blocks().size());

    /* Initialize the reverse lookup of CLB block positions */
    place_ctx.grid_blocks = init_grid_blocks();
}

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

t_placer_costs& t_placer_costs::operator+=(const NocCostTerms& noc_delta_cost) {
    noc_cost_terms += noc_delta_cost;

    return *this;
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

bool t_annealing_state::outer_loop_update(float success_rate,
                                          const t_placer_costs& costs,
                                          const t_placer_opts& placer_opts,
                                          const t_annealing_sched& annealing_sched) {
#ifndef NO_GRAPHICS
    t_draw_state* draw_state = get_draw_state_vars();
    if (!draw_state->list_of_breakpoints.empty()) {
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

void t_annealing_state::update_rlim(float success_rate) {
    rlim *= (1. - 0.44 + success_rate);
    rlim = std::min(rlim, UPPER_RLIM);
    rlim = std::max(rlim, FINAL_RLIM);
}

void t_annealing_state::update_crit_exponent(const t_placer_opts& placer_opts) {
    /* If rlim == FINAL_RLIM, then scale == 0. */
    float scale = 1 - (rlim - FINAL_RLIM) * INVERSE_DELTA_RLIM;

    /* Apply the scaling factor on crit_exponent. */
    crit_exponent = scale * (placer_opts.td_place_exp_last - placer_opts.td_place_exp_first)
                    + placer_opts.td_place_exp_first;
}

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

                for (const auto& sub_tile : tile->sub_tiles) {
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
                        // Otherwise, don't, so large blocks aren't added multiple times.
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
         * since overused locations will be spread by legalizer afterward.
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

NocCostTerms::NocCostTerms(double agg_bw, double lat, double lat_overrun, double congest)
    : aggregate_bandwidth(agg_bw)
    , latency(lat)
    , latency_overrun(lat_overrun)
    , congestion(congest) {}

NocCostTerms::NocCostTerms()
    : aggregate_bandwidth(0)
    , latency(0)
    , latency_overrun(0)
    , congestion(0) {}

NocCostTerms& NocCostTerms::operator+=(const NocCostTerms& noc_delta_cost) {
    aggregate_bandwidth += noc_delta_cost.aggregate_bandwidth;
    latency += noc_delta_cost.latency;
    latency_overrun += noc_delta_cost.latency_overrun;
    congestion += noc_delta_cost.congestion;

    return *this;
}
