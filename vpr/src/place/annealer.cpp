
#include "annealer.h"

#include <algorithm>
#include <cmath>

#include "globals.h"
#include "draw_global.h"
#include "place_macro.h"
#include "vpr_types.h"
#include "place_util.h"
#include "placer_state.h"
#include "move_utils.h"
#include "noc_place_utils.h"
#include "NetPinTimingInvalidator.h"
#include "place_timing_update.h"
#include "read_place.h"
#include "placer_breakpoint.h"
#include "RL_agent_util.h"
#include "PlacerSetupSlacks.h"
#include "PlacerCriticalities.h"

/**************************************************************************/
/*************** Static Function Declarations *****************************/
/**************************************************************************/

/**
 * @brief Check if the setup slack has gotten better or worse due to block swap.
 *
 * Get all the modified slack values via the PlacerSetupSlacks class, and compare
 * then with the original values at these connections. Sort them and compare them
 * one by one, and return the difference of the first different pair.
 *
 * If the new slack value is larger(better), than return a negative value so that
 * the move will be accepted. If the new slack value is smaller(worse), return a
 * positive value so that the move will be rejected.
 *
 * If no slack values have changed, then return an arbitrary positive number. A
 * move resulting in no change in the slack values should probably be unnecessary.
 *
 * The sorting is needed to prevent in the unlikely circumstance that a bad slack
 * value suddenly got very good due to the block move, while a good slack value
 * got very bad, perhaps even worse than the original worse slack value.
 */
static float analyze_setup_slack_cost(const PlacerSetupSlacks* setup_slacks,
                                      const PlacerState& placer_state);

/*************************************************************************/
/*************** Static Function Definitions *****************************/
/*************************************************************************/

static float analyze_setup_slack_cost(const PlacerSetupSlacks* setup_slacks,
                                      const PlacerState& placer_state) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& clb_nlist = cluster_ctx.clb_nlist;

    const auto& p_timing_ctx = placer_state.timing();
    const auto& connection_setup_slack = p_timing_ctx.connection_setup_slack;

    //Find the original/proposed setup slacks of pins with modified values
    std::vector<float> original_setup_slacks, proposed_setup_slacks;

    auto clb_pins_modified = setup_slacks->pins_with_modified_setup_slack();
    for (ClusterPinId clb_pin : clb_pins_modified) {
        ClusterNetId net_id = clb_nlist.pin_net(clb_pin);
        size_t ipin = clb_nlist.pin_net_index(clb_pin);

        original_setup_slacks.push_back(connection_setup_slack[net_id][ipin]);
        proposed_setup_slacks.push_back(setup_slacks->setup_slack(net_id, ipin));
    }

    //Sort in ascending order, from the worse slack value to the best
    std::stable_sort(original_setup_slacks.begin(), original_setup_slacks.end());
    std::stable_sort(proposed_setup_slacks.begin(), proposed_setup_slacks.end());

    //Check the first pair of slack values that are different
    //If found, return their difference
    for (size_t idiff = 0; idiff < original_setup_slacks.size(); ++idiff) {
        float slack_diff = original_setup_slacks[idiff] - proposed_setup_slacks[idiff];

        if (slack_diff != 0) {
            return slack_diff;
        }
    }

    //If all slack values are identical (or no modified slack values),
    //reject this move by returning an arbitrary positive number as cost.
    return 1;
}

/**************************************************************************************/
/*************** Member Function Definitions for t_annealing_state ********************/
/**************************************************************************************/

///@brief Constructor: Initialize all annealing state variables and macros.
t_annealing_state::t_annealing_state(float first_t,
                                     float first_rlim,
                                     int first_move_lim,
                                     float first_crit_exponent) {
    num_temps = 0;
    alpha = 1.f;
    t = first_t;
    rlim = first_rlim;
    move_lim_max = first_move_lim;
    crit_exponent = first_crit_exponent;
    move_lim = move_lim_max;

    /* Store this inverse value for speed when updating crit_exponent. */
    INVERSE_DELTA_RLIM = 1 / (first_rlim - FINAL_RLIM);

    /* The range limit cannot exceed the largest grid size. */
    const auto& grid = g_vpr_ctx.device().grid;
    UPPER_RLIM = std::max(grid.width() - 1, grid.height() - 1);
}

bool t_annealing_state::outer_loop_update(float success_rate,
                                          const t_placer_costs& costs,
                                          const t_placer_opts& placer_opts) {
#ifndef NO_GRAPHICS
    t_draw_state* draw_state = get_draw_state_vars();
    if (!draw_state->list_of_breakpoints.empty()) {
        // Update temperature in the current information variable.
        get_bp_state_globals()->get_glob_breakpoint_state()->temp_count++;
    }
#endif

    if (placer_opts.anneal_sched.type == e_sched_type::USER_SCHED) {
        // Update t with user specified alpha.
        t *= placer_opts.anneal_sched.alpha_t;

        // Check if the exit criterion is met.
        bool exit_anneal = t >= placer_opts.anneal_sched.exit_t;

        return exit_anneal;
    }

    // Automatically determine exit temperature.
    auto& cluster_ctx = g_vpr_ctx.clustering();
    float t_exit = 0.005 * costs.cost / cluster_ctx.clb_nlist.nets().size();


    VTR_ASSERT_SAFE(placer_opts.anneal_sched.type == e_sched_type::AUTO_SCHED);
    // Automatically adjust alpha according to success rate.
    if (success_rate > 0.96) {
        alpha = 0.5;
    } else if (success_rate > 0.8) {
        alpha = 0.9;
    } else if (success_rate > 0.15 || rlim > 1.) {
        alpha = 0.95;
    } else {
        alpha = 0.8;
    }
    // Update temp.
    t *= alpha;
    // Must be duplicated to retain previous behavior.
    if (t < t_exit || std::isnan(t_exit)) {
        return false;
    }

    // Update the range limiter.
    update_rlim(success_rate);

    // If using timing driven algorithm, update the crit_exponent.
    if (placer_opts.place_algorithm.is_timing_driven()) {
        update_crit_exponent(placer_opts);
    }

    // Continues the annealing.
    return true;
}

void t_annealing_state::update_rlim(float success_rate) {
    rlim *= (1. - 0.44 + success_rate);
    rlim = std::min(rlim, UPPER_RLIM);
    rlim = std::max(rlim, FINAL_RLIM);
}

void t_annealing_state::update_crit_exponent(const t_placer_opts& placer_opts) {
    // If rlim == FINAL_RLIM, then scale == 0.
    float scale = 1 - (rlim - FINAL_RLIM) * INVERSE_DELTA_RLIM;

    // Apply the scaling factor on crit_exponent.
    crit_exponent = scale * (placer_opts.td_place_exp_last - placer_opts.td_place_exp_first)
                    + placer_opts.td_place_exp_first;
}

/**************************************************************************************/
/*************** Member Function Definitions for PlacementAnnealer ********************/
/**************************************************************************************/

PlacementAnnealer::PlacementAnnealer(const t_placer_opts& placer_opts,
                                     PlacerState& placer_state,
                                     const PlaceMacros& place_macros,
                                     t_placer_costs& costs,
                                     NetCostHandler& net_cost_handler,
                                     std::optional<NocCostHandler>& noc_cost_handler,
                                     const t_noc_opts& noc_opts,
                                     vtr::RngContainer& rng,
                                     std::unique_ptr<MoveGenerator>&& move_generator_1,
                                     std::unique_ptr<MoveGenerator>&& move_generator_2,
                                     const PlaceDelayModel* delay_model,
                                     PlacerCriticalities* criticalities,
                                     PlacerSetupSlacks* setup_slacks,
                                     SetupTimingInfo* timing_info,
                                     NetPinTimingInvalidator* pin_timing_invalidator,
                                     int move_lim)
    : placer_opts_(placer_opts)
    , placer_state_(placer_state)
    , place_macros_(place_macros)
    , costs_(costs)
    , net_cost_handler_(net_cost_handler)
    , noc_cost_handler_(noc_cost_handler)
    , noc_opts_(noc_opts)
    , rng_(rng)
    , move_generator_1_(std::move(move_generator_1))
    , move_generator_2_(std::move(move_generator_2))
    , manual_move_generator_(placer_state, rng)
    , agent_state_(e_agent_state::EARLY_IN_THE_ANNEAL)
    , delay_model_(delay_model)
    , criticalities_(criticalities)
    , setup_slacks_(setup_slacks)
    , timing_info_(timing_info)
    , pin_timing_invalidator_(pin_timing_invalidator)
    , move_stats_file_(nullptr, vtr::fclose)
    , outer_crit_iter_count_(1)
    , blocks_affected_(placer_state.block_locs().size())
    , quench_started_(false)
{
    const auto& device_ctx = g_vpr_ctx.device();

    float first_crit_exponent;
    if (placer_opts.place_algorithm.is_timing_driven()) {
        first_crit_exponent = placer_opts.td_place_exp_first; /*this will be modified when rlim starts to change */
    } else {
        first_crit_exponent = 0.f;
    }

    int first_move_lim = get_initial_move_lim(placer_opts, placer_opts_.anneal_sched);

    if (placer_opts.inner_loop_recompute_divider != 0) {
        inner_recompute_limit_ = static_cast<int>(0.5 + (float)first_move_lim / (float)placer_opts.inner_loop_recompute_divider);
    } else {
        // don't do an inner recompute
        inner_recompute_limit_ = first_move_lim + 1;
    }

    /* calculate the number of moves in the quench that we should recompute timing after based on the value of *
     * the commandline option quench_recompute_divider                                                         */
    if (placer_opts.quench_recompute_divider != 0) {
        quench_recompute_limit_ = static_cast<int>(0.5 + (float)move_lim / (float)placer_opts.quench_recompute_divider);
    } else {
        // don't do an quench recompute
        quench_recompute_limit_ = first_move_lim + 1;
    }

    moves_since_cost_recompute_ = 0;
    tot_iter_ = 0;

    // Get the first range limiter
    placer_state_.mutable_move().first_rlim = (float)std::max(device_ctx.grid.width() - 1, device_ctx.grid.height() - 1);

    annealing_state_ = t_annealing_state(EPSILON,    // Set the temperature low to ensure that initial placement quality will be preserved
                                         placer_state_.move().first_rlim,
                                         first_move_lim,
                                         first_crit_exponent);

    if (!placer_opts.move_stats_file.empty()) {
        move_stats_file_ = std::unique_ptr<FILE, decltype(&vtr::fclose)>(
            vtr::fopen(placer_opts.move_stats_file.c_str(), "w"),
            vtr::fclose);
        LOG_MOVE_STATS_HEADER();
    }

    //allocate move type statistics vectors
    move_type_stats_.blk_type_moves.resize({device_ctx.logical_block_types.size(), (int)e_move_type::NUMBER_OF_AUTO_MOVES}, 0);
    move_type_stats_.accepted_moves.resize({device_ctx.logical_block_types.size(), (int)e_move_type::NUMBER_OF_AUTO_MOVES}, 0);
    move_type_stats_.rejected_moves.resize({device_ctx.logical_block_types.size(), (int)e_move_type::NUMBER_OF_AUTO_MOVES}, 0);

    // Update the starting temperature for placement annealing to a more appropriate value
    annealing_state_.t = estimate_starting_temperature_();
}

float PlacementAnnealer::estimate_starting_temperature_() {
    if (placer_opts_.anneal_sched.type == e_sched_type::USER_SCHED) {
        return placer_opts_.anneal_sched.init_t;
    }

    const auto& cluster_ctx = g_vpr_ctx.clustering();

    // Use to calculate the average of cost when swap is accepted.
    int num_accepted = 0;

    // Use double types to avoid round off.
    double av = 0., sum_of_squares = 0.;

    // Determines the block swap loop count.
    int move_lim = std::min(annealing_state_.move_lim_max, (int)cluster_ctx.clb_nlist.blocks().size());

    bool manual_move_enabled = false;

    for (int i = 0; i < move_lim; i++) {
#ifndef NO_GRAPHICS
        // Checks manual move flag for manual move feature
        t_draw_state* draw_state = get_draw_state_vars();
        if (draw_state->show_graphics) {
            manual_move_enabled = manual_move_is_selected();
        }
#endif /*NO_GRAPHICS*/

        // Will not deploy setup slack analysis, so omit crit_exponenet and setup_slack
        e_move_result swap_result = try_swap_(*move_generator_1_, placer_opts_.place_algorithm, manual_move_enabled);

        if (swap_result == e_move_result::ACCEPTED) {
            num_accepted++;
            av += costs_.cost;
            sum_of_squares += costs_.cost * costs_.cost;
            swap_stats_.num_swap_accepted++;
        } else if (swap_result == e_move_result::ABORTED) {
            swap_stats_.num_swap_aborted++;
        } else {
            swap_stats_.num_swap_rejected++;
        }
    }

    // Take the average of the accepted swaps' cost values.
    av = num_accepted > 0 ? (av / num_accepted) : 0.;

    // Get the standard deviation.
    double std_dev = get_std_dev(num_accepted, sum_of_squares, av);

    // Print warning if not all swaps are accepted.
    if (num_accepted != move_lim) {
        VTR_LOG_WARN("Starting t: %d of %d configurations accepted.\n",
                     num_accepted, move_lim);
    }

    // Improved initial placement uses a fast SA for NoC routers and centroid placement
    // for other blocks. The temperature is reduced to prevent SA from destroying the initial placement
    float init_temp = std_dev / 64;

    return init_temp;
}

e_move_result PlacementAnnealer::try_swap_(MoveGenerator& move_generator,
                                           const t_place_algorithm& place_algorithm,
                                           bool manual_move_enabled) {
    /* Picks some block and moves it to another spot.  If this spot is
     * occupied, switch the blocks.  Assess the change in cost function.
     * rlim is the range limiter.
     * Returns whether the swap is accepted, rejected or aborted.
     * Passes back the new value of the cost functions.
     */
    auto& blk_loc_registry = placer_state_.mutable_blk_loc_registry();

    // increment the call counter
    swap_stats_.num_ts_called++;

    PlaceCritParams crit_params{annealing_state_.crit_exponent,
                                placer_opts_.place_crit_limit};

    // move type and block type chosen by the agent
    t_propose_action proposed_action{e_move_type::UNIFORM, -1};

    MoveOutcomeStats move_outcome_stats;

    /* I'm using negative values of proposed_net_cost as a flag,
     * so DO NOT use cost functions that can go negative. */
    double delta_c = 0;        //Change in cost due to this swap.
    double bb_delta_c = 0;     //Change in the bounding box (wiring) cost.
    double timing_delta_c = 0; //Change in the timing cost (delay * criticality).


    /* Allow some fraction of moves to not be restricted by rlim,
     * in the hopes of better escaping local minima. */
    float rlim;
    if (placer_opts_.rlim_escape_fraction > 0. && rng_.frand() < placer_opts_.rlim_escape_fraction) {
        rlim = std::numeric_limits<float>::infinity();
    } else {
        rlim = annealing_state_.rlim;
    }

    e_create_move create_move_outcome = e_create_move::ABORT;

    // Determine whether we need to force swap two NoC router blocks
    bool router_block_move = false;
    if (noc_opts_.noc) {
        router_block_move = check_for_router_swap(noc_opts_.noc_swap_percentage, rng_);
    }

    //When manual move toggle button is active, the manual move window asks the user for input.
    if (manual_move_enabled) {
#ifndef NO_GRAPHICS
        create_move_outcome = manual_move_display_and_propose(manual_move_generator_, blocks_affected_,
                                                              proposed_action.move_type, rlim, place_macros_,
                                                              placer_opts_, criticalities_);
#endif //NO_GRAPHICS
    } else if (router_block_move) {
        // generate a move where two random router blocks are swapped
        create_move_outcome = propose_router_swap(blocks_affected_, rlim, blk_loc_registry, place_macros_, rng_);
        proposed_action.move_type = e_move_type::UNIFORM;
    } else {
        //Generate a new move (perturbation) used to explore the space of possible placements
        create_move_outcome = move_generator.propose_move(blocks_affected_, proposed_action, rlim, place_macros_, placer_opts_, criticalities_);
    }

    move_type_stats_.incr_blk_type_moves(proposed_action);

    if constexpr (VTR_ENABLE_DEBUG_LOGGING_CONST_EXPR) LOG_MOVE_STATS_PROPOSED();

    VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug,
                   "\t\tBefore move Place cost %e, bb_cost %e, timing cost %e\n",
                   costs_.cost, costs_.bb_cost, costs_.timing_cost);

    e_move_result move_outcome = e_move_result::ABORTED;

    if (create_move_outcome == e_create_move::ABORT) {
        if constexpr (VTR_ENABLE_DEBUG_LOGGING_CONST_EXPR) {
            LOG_MOVE_STATS_OUTCOME(std::numeric_limits<double>::quiet_NaN(),
                                   std::numeric_limits<double>::quiet_NaN(),
                                   std::numeric_limits<double>::quiet_NaN(), "ABORTED",
                                   "illegal move");
        }

        move_outcome = e_move_result::ABORTED;

    } else {
        VTR_ASSERT(create_move_outcome == e_create_move::VALID);

        /* To make evaluating the move simpler (e.g. calculating changed bounding box),
         * we first move the blocks to their new locations (apply the move to
         * blk_loc_registry.block_locs) and then compute the change in cost. If the move
         * is accepted, the inverse look-up in blk_loc_registry.grid_blocks is updated
         * (committing the move). If the move is rejected, the blocks are returned to
         * their original positions (reverting blk_loc_registry.block_locs to its original state).
         *
         * Note that the inverse look-up blk_loc_registry.grid_blocks is only updated after
         * move acceptance is determined, so it should not be used when evaluating a move.
         */

        // Update the block positions
        blk_loc_registry.apply_move_blocks(blocks_affected_);

        /* Find all the nets affected by this swap and update the wiring costs.
         * This cost value doesn't depend on the timing info.
         * Also find all the pins affected by the swap, and calculates new connection
         * delays and timing costs and store them in proposed_* data structures.
         */
        net_cost_handler_.find_affected_nets_and_update_costs(delay_model_, criticalities_, blocks_affected_,
                                                              bb_delta_c, timing_delta_c);

        if (place_algorithm == e_place_algorithm::CRITICALITY_TIMING_PLACE) {
            /* Take delta_c as a combination of timing and wiring cost. In
             * addition to `timing_tradeoff`, we normalize the cost values.
             * CRITICALITY_TIMING_PLACE algorithm works with somewhat stale
             * timing information to save CPU time.
             */
            VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug,
                           "\t\tMove bb_delta_c %e, bb_cost_norm %e, timing_tradeoff %f, "
                           "timing_delta_c %e, timing_cost_norm %e\n",
                           bb_delta_c,
                           costs_.bb_cost_norm,
                           placer_opts_.timing_tradeoff,
                           timing_delta_c,
                           costs_.timing_cost_norm);
            delta_c = (1 - placer_opts_.timing_tradeoff) * bb_delta_c * costs_.bb_cost_norm
                      + placer_opts_.timing_tradeoff * timing_delta_c * costs_.timing_cost_norm;
        } else if (place_algorithm == e_place_algorithm::SLACK_TIMING_PLACE) {
            /* For setup slack analysis, we first do a timing analysis to get the newest
             * slack values resulted from the proposed block moves. If the move turns out
             * to be accepted, we keep the updated slack values and commit the block moves.
             * If rejected, we reject the proposed block moves and revert this timing analysis.
             *
             * It should be noted that when SLACK_TIMING_PLACE algorithm is used, proposed moves
             * are evaluated with up-to-date timing information, which is more expensive but more
             * accurate.
             */

            // Invalidates timing of modified connections for incremental timing updates.
            pin_timing_invalidator_->invalidate_affected_connections(blocks_affected_);

            /* Update the connection_timing_cost and connection_delay
             * values from the temporary values. */
            placer_state_.mutable_timing().commit_td_cost(blocks_affected_);

            /* Update timing information. Since we are analyzing setup slacks,
             * we only update those values and keep the criticalities stale
             * so as not to interfere with the original timing driven algorithm.
             *
             * Note: the timing info must be updated after applying block moves
             * and committing the timing driven delays and costs.
             * If we wish to revert this timing update due to move rejection,
             * we need to revert block moves and restore the timing values. */
            criticalities_->disable_update();
            setup_slacks_->enable_update();
            update_timing_classes(crit_params, timing_info_, criticalities_,
                                  setup_slacks_, pin_timing_invalidator_);

            /* Get the setup slack analysis cost */
            //TODO: calculate a weighted average of the slack cost and wiring cost
            delta_c = analyze_setup_slack_cost(setup_slacks_, placer_state_) * costs_.timing_cost_norm;
        } else {
            VTR_ASSERT_SAFE(place_algorithm == e_place_algorithm::BOUNDING_BOX_PLACE);
            VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug,
                           "\t\tMove bb_delta_c %e, bb_cost_norm %e\n",
                           bb_delta_c,
                           costs_.bb_cost_norm);
            delta_c = bb_delta_c * costs_.bb_cost_norm;
        }

        NocCostTerms noc_delta_c; // change in NoC cost
        /* Update the NoC data structure and costs*/
        if (noc_opts_.noc) {
            VTR_ASSERT_SAFE(noc_cost_handler_.has_value());
            noc_cost_handler_->find_affected_noc_routers_and_update_noc_costs(blocks_affected_, noc_delta_c);

            // Include the NoC delta costs in the total cost change for this swap
            delta_c += calculate_noc_cost(noc_delta_c, costs_.noc_cost_norm_factors, noc_opts_);
        }

        // determine whether the move is accepted or rejected
        move_outcome = assess_swap_(delta_c, annealing_state_.t);

        //Updates the manual_move_state members and displays costs to the user to decide whether to ACCEPT/REJECT manual move.
#ifndef NO_GRAPHICS
        if (manual_move_enabled) {
            move_outcome = pl_do_manual_move(delta_c, timing_delta_c, bb_delta_c, move_outcome);
        }
#endif //NO_GRAPHICS

        if (move_outcome == e_move_result::ACCEPTED) {
            costs_.cost += delta_c;
            costs_.bb_cost += bb_delta_c;

            if (place_algorithm == e_place_algorithm::CRITICALITY_TIMING_PLACE) {
                costs_.timing_cost += timing_delta_c;

                /* Invalidates timing of modified connections for incremental
                 * timing updates. These invalidations are accumulated for a
                 * big timing update in the outer loop. */
                pin_timing_invalidator_->invalidate_affected_connections(blocks_affected_);

                /* Update the connection_timing_cost and connection_delay
                 * values from the temporary values. */
                placer_state_.mutable_timing().commit_td_cost(blocks_affected_);

            } else if (place_algorithm == e_place_algorithm::SLACK_TIMING_PLACE) {
                // Update the timing driven cost as usual
                costs_.timing_cost += timing_delta_c;

                // Commit the setup slack information
                // The timing delay and cost values should be committed already
                commit_setup_slacks(setup_slacks_, placer_state_);
            }

            // Update net cost functions and reset flags.
            net_cost_handler_.update_move_nets();

            // Update clb data structures since we kept the move.
            blk_loc_registry.commit_move_blocks(blocks_affected_);

            if (noc_opts_.noc){
                noc_cost_handler_->commit_noc_costs();
                costs_ += noc_delta_c;
            }

            //Highlights the new block when manual move is selected.
#ifndef NO_GRAPHICS
            if (manual_move_enabled) {
                manual_move_highlight_new_block_location();
            }
#endif //NO_GRAPHICS

        } else {
            VTR_ASSERT_SAFE(move_outcome == e_move_result::REJECTED);

            // Reset the net cost function flags first.
            net_cost_handler_.reset_move_nets();

            // Restore the blk_loc_registry.block_locs data structures to their state before the move.
            blk_loc_registry.revert_move_blocks(blocks_affected_);

            if (place_algorithm == e_place_algorithm::CRITICALITY_TIMING_PLACE) {
                // Un-stage the values stored in proposed_* data structures
                placer_state_.mutable_timing().revert_td_cost(blocks_affected_);
            } else if (place_algorithm == e_place_algorithm::SLACK_TIMING_PLACE) {
                /* Revert the timing delays and costs to pre-update values.
                 * These routines must be called after reverting the block moves.
                 */
                //TODO: make this process incremental
                comp_td_connection_delays(delay_model_, placer_state_);
                comp_td_costs(delay_model_, *criticalities_, placer_state_, &costs_.timing_cost);

                /* Re-invalidate the affected sink pins since the proposed
                 * move is rejected, and the same blocks are reverted to
                 * their original positions. */
                pin_timing_invalidator_->invalidate_affected_connections(blocks_affected_);

                // Revert the timing update
                update_timing_classes(crit_params, timing_info_, criticalities_,
                                      setup_slacks_, pin_timing_invalidator_);

                VTR_ASSERT_SAFE_MSG(
                    verify_connection_setup_slacks(setup_slacks_, placer_state_),
                    "The current setup slacks should be identical to the values before the try swap timing info update.");
            }

            // Revert the traffic flow routes within the NoC
            if (noc_opts_.noc) {
                noc_cost_handler_->revert_noc_traffic_flow_routes(blocks_affected_);
            }
        }

        move_type_stats_.incr_accept_reject(proposed_action, move_outcome);

        move_outcome_stats.delta_cost_norm = delta_c;
        move_outcome_stats.delta_bb_cost_norm = bb_delta_c * costs_.bb_cost_norm;
        move_outcome_stats.delta_timing_cost_norm = timing_delta_c * costs_.timing_cost_norm;

        move_outcome_stats.delta_bb_cost_abs = bb_delta_c;
        move_outcome_stats.delta_timing_cost_abs = timing_delta_c;

        if constexpr (VTR_ENABLE_DEBUG_LOGGING_CONST_EXPR) {
            LOG_MOVE_STATS_OUTCOME(delta_c, bb_delta_c, timing_delta_c, (move_outcome == e_move_result::ACCEPTED ? "ACCEPTED" : "REJECTED"), "");
        }
    }
    move_outcome_stats.outcome = move_outcome;

    // If we force a router block move then it was not proposed by the
    // move generator, so we should not calculate the reward and update
    // the move generators status since this outcome is not a direct
    // consequence of the move generator
    if (!router_block_move) {
        move_generator.calculate_reward_and_process_outcome(move_outcome_stats, delta_c, REWARD_BB_TIMING_RELATIVE_WEIGHT);
    }

#ifndef NO_GRAPHICS
    stop_placement_and_check_breakpoints(blocks_affected_, move_outcome, delta_c, bb_delta_c, timing_delta_c);
#endif


    // Clear the data structure containing block move info
    blocks_affected_.clear_move_blocks();

    VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug,
                   "\t\tAfter move Place cost %e, bb_cost %e, timing cost %e\n",
                   costs_.cost, costs_.bb_cost, costs_.timing_cost);
    return move_outcome;
}

void PlacementAnnealer::outer_loop_update_timing_info() {
    if (placer_opts_.place_algorithm.is_timing_driven()) {
        /* At each temperature change we update these values to be used
         * for normalizing the tradeoff between timing and wirelength (bb) */
        if (outer_crit_iter_count_ >= placer_opts_.recompute_crit_iter ||
            placer_opts_.inner_loop_recompute_divider != 0) {

            PlaceCritParams crit_params{annealing_state_.crit_exponent,
                                        placer_opts_.place_crit_limit};

            // Update all timing related classes
            perform_full_timing_update(crit_params, delay_model_, criticalities_, setup_slacks_,
                                       pin_timing_invalidator_, timing_info_, &costs_, placer_state_);

            outer_crit_iter_count_ = 0;
        }
        outer_crit_iter_count_++;
    }

    // Update the cost normalization factors
    costs_.update_norm_factors();

    // update the current total placement cost
    costs_.cost = costs_.get_total_cost(placer_opts_, noc_opts_);
}

void PlacementAnnealer::placement_inner_loop() {
    // How many times have we dumped placement to a file this temperature?
    int inner_placement_save_count = 0;

    placer_stats_.reset();

    bool manual_move_enabled = false;

    MoveGenerator& move_generator = select_move_generator(move_generator_1_, move_generator_2_, agent_state_,
                                                          placer_opts_, quench_started_);

    // Inner loop begins
    for (int inner_iter = 0, inner_crit_iter_count = 1; inner_iter < annealing_state_.move_lim; inner_iter++) {
        e_move_result swap_result = try_swap_(move_generator, placer_opts_.place_algorithm, manual_move_enabled);

        if (swap_result == e_move_result::ACCEPTED) {
            // Move was accepted.  Update statistics that are useful for the annealing schedule.
            placer_stats_.single_swap_update(costs_);
            swap_stats_.num_swap_accepted++;
        } else if (swap_result == e_move_result::ABORTED) {
            swap_stats_.num_swap_aborted++;
        } else { // swap_result == REJECTED
            swap_stats_.num_swap_rejected++;
        }

        if (placer_opts_.place_algorithm.is_timing_driven()) {
            /* Do we want to re-timing analyze the circuit to get updated slack and criticality values?
             * We do this only once in a while, since it is expensive.
             */
            const int recompute_limit = quench_started_ ? quench_recompute_limit_ : inner_recompute_limit_;
            // on last iteration don't recompute
            if (inner_crit_iter_count >= recompute_limit && inner_iter != annealing_state_.move_lim - 1) {

                inner_crit_iter_count = 0;

                PlaceCritParams crit_params{annealing_state_.crit_exponent,
                                            placer_opts_.place_crit_limit};

                // Update all timing related classes
                perform_full_timing_update(crit_params, delay_model_, criticalities_,
                                           setup_slacks_, pin_timing_invalidator_,
                                           timing_info_, &costs_, placer_state_);
            }
            inner_crit_iter_count++;
        }

        /* Lines below prevent too much round-off error from accumulating
         * in the cost over many iterations (due to incremental updates).
         * This round-off can lead to error checks failing because the cost
         * is different from what you get when you recompute from scratch.
         */
        moves_since_cost_recompute_++;
        if (moves_since_cost_recompute_ > MAX_MOVES_BEFORE_RECOMPUTE) {
            net_cost_handler_.recompute_costs_from_scratch(delay_model_, criticalities_, costs_);

            if (noc_cost_handler_.has_value()) {
                noc_cost_handler_->recompute_costs_from_scratch(noc_opts_, costs_);
            }

            moves_since_cost_recompute_ = 0;
        }

        if (placer_opts_.placement_saves_per_temperature >= 1 && inner_iter > 0
            && (inner_iter + 1) % (annealing_state_.move_lim / placer_opts_.placement_saves_per_temperature) == 0) {
            std::string filename = vtr::string_fmt("placement_%03d_%03d.place",
                                                   annealing_state_.num_temps + 1, inner_placement_save_count);
            VTR_LOG("Saving placement to file at temperature move %d / %d: %s\n",
                    inner_iter, annealing_state_.move_lim, filename.c_str());
            print_place(nullptr, nullptr, filename.c_str(), placer_state_.block_locs());
            ++inner_placement_save_count;
        }
    }

    // Calculate the success_rate and std_dev of the costs.
    placer_stats_.calc_iteration_stats(costs_, annealing_state_.move_lim);

    // update the RL agent's state
    if (!quench_started_) {
        if (placer_opts_.place_algorithm.is_timing_driven() &&
            placer_opts_.place_agent_multistate &&
            agent_state_ == e_agent_state::EARLY_IN_THE_ANNEAL) {
            if (annealing_state_.alpha < 0.85 && annealing_state_.alpha > 0.6) {
                agent_state_ = e_agent_state::LATE_IN_THE_ANNEAL;
                VTR_LOG("Agent's 2nd state: \n");
            }
        }
    }

    tot_iter_ += annealing_state_.move_lim;
    ++annealing_state_.num_temps;
}


int PlacementAnnealer::get_total_iteration() const {
    return tot_iter_;
}

e_agent_state PlacementAnnealer::get_agent_state() const {
    return agent_state_;
}

const t_annealing_state& PlacementAnnealer::get_annealing_state() const {
    return annealing_state_;
}

bool PlacementAnnealer::outer_loop_update_state() {
    return annealing_state_.outer_loop_update(placer_stats_.success_rate, costs_, placer_opts_);
}

void PlacementAnnealer::start_quench() {
    quench_started_ = true;

    // Freeze out: only accept solutions that improve placement.
    annealing_state_.t = 0;

    // Revert the move limit to initial value.
    annealing_state_.move_lim = annealing_state_.move_lim_max;
}

std::tuple<const t_swap_stats&, const MoveTypeStat&, const t_placer_statistics&> PlacementAnnealer::get_stats() const {
    return {swap_stats_, move_type_stats_, placer_stats_};
}

const MoveAbortionLogger& PlacementAnnealer::get_move_abortion_logger() const {
    return blocks_affected_.move_abortion_logger;
}

void PlacementAnnealer::LOG_MOVE_STATS_HEADER() {
    if constexpr (VTR_ENABLE_DEBUG_LOGGING_CONST_EXPR) {
        if (move_stats_file_) {
            fprintf(move_stats_file_.get(),
                    "temp,from_blk,to_blk,from_type,to_type,"
                    "blk_count,"
                    "delta_cost,delta_bb_cost,delta_td_cost,"
                    "outcome,reason\n");
        }
    } else {
        if (move_stats_file_) {
            fprintf(move_stats_file_.get(),
                    "VTR_ENABLE_DEBUG_LOGGING disabled "
                    "-- No move stats recorded\n");
        }
    }
}

void PlacementAnnealer::LOG_MOVE_STATS_PROPOSED() {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& grid_blocks = placer_state_.grid_blocks();

    if (move_stats_file_) {
        ClusterBlockId b_from = blocks_affected_.moved_blocks[0].block_num;

        t_pl_loc to = blocks_affected_.moved_blocks[0].new_loc;
        ClusterBlockId b_to = grid_blocks.block_at_location(to);

        t_logical_block_type_ptr from_type = cluster_ctx.clb_nlist.block_type(b_from);
        t_logical_block_type_ptr to_type = nullptr;
        if (b_to) {
            to_type = cluster_ctx.clb_nlist.block_type(b_to);
        }

        fprintf(move_stats_file_.get(),
                "%g,"
                "%d,%d,"
                "%s,%s,"
                "%d,",
                annealing_state_.t,
                int(b_from), int(b_to),
                from_type->name.c_str(),
                to_type ? to_type->name.c_str() : "EMPTY",
                (int)blocks_affected_.moved_blocks.size());
    }
}

void PlacementAnnealer::LOG_MOVE_STATS_OUTCOME(double delta_cost, double delta_bb_cost, double delta_td_cost,
                                               const char* outcome, const char* reason) {
    if (move_stats_file_) {
        fprintf(move_stats_file_.get(),
                "%g,%g,%g,"
                "%s,%s\n",
                delta_cost, delta_bb_cost, delta_td_cost,
                outcome, reason);
    }
}

e_move_result PlacementAnnealer::assess_swap_(double delta_c, double t) {
    /* Returns: 1 -> move accepted, 0 -> rejected. */
    VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\tTemperature is: %e delta_c is %e\n", t, delta_c);
    if (delta_c <= 0) {
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\t\tMove is accepted(delta_c < 0)\n");
        return e_move_result::ACCEPTED;
    }

    if (t == 0.) {
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\t\tMove is rejected(t == 0)\n");
        return e_move_result::REJECTED;
    }

    float fnum = rng_.frand();
    float prob_fac = std::exp(-delta_c / t);
    if (prob_fac > fnum) {
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\t\tMove is accepted(hill climbing)\n");
        return e_move_result::ACCEPTED;
    }
    VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\t\tMove is rejected(hill climbing)\n");
    return e_move_result::REJECTED;
}
