/**
 * @file place_util.cpp
 * @brief Definitions of structure routines declared in place_util.h.
 */

#include "place_util.h"
#include "globals.h"

///<File-scope routines.
static vtr::Matrix<t_grid_blocks> init_grid_blocks();
static void update_rlim(float* rlim, float success_rat, const DeviceGrid& grid);

///@brief Initialize the placement context.
void init_placement_context() {
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    /* Intialize the lookup of CLB block positions */
    place_ctx.block_locs.clear();
    place_ctx.block_locs.resize(cluster_ctx.clb_nlist.blocks().size());

    /* Initialize the reverse lookup of CLB block positions */
    place_ctx.grid_blocks = init_grid_blocks();
}

///@brief Initialize `grid_blocks`, the inverse structure of `block_locs`.
static vtr::Matrix<t_grid_blocks> init_grid_blocks() {
    auto& device_ctx = g_vpr_ctx.device();

    auto grid_blocks = vtr::Matrix<t_grid_blocks>({device_ctx.grid.width(), device_ctx.grid.height()});
    for (size_t x = 0; x < device_ctx.grid.width(); ++x) {
        for (size_t y = 0; y < device_ctx.grid.height(); ++y) {
            auto type = device_ctx.grid[x][y].type;

            int capacity = type->capacity;

            grid_blocks[x][y].blocks.resize(capacity, EMPTY_BLOCK_ID);
        }
    }

    return grid_blocks;
}

///@brief Constructor: stores current placer algorithm.
t_placer_costs::t_placer_costs(enum e_place_algorithm algo)
    : place_algorithm(algo) {
    if (place_algorithm != PATH_TIMING_DRIVEN_PLACE) {
        VTR_ASSERT_MSG(
            place_algorithm == BOUNDING_BOX_PLACE,
            "Must pass a valid placer algorithm into the placer cost structure.");
    }
}

/**
 * @brief Mutator: updates the norm factors in the outer loop iteration.
 *
 * At each temperature change we update these values to be used
 * for normalizing the trade-off between timing and wirelength (bb)
 */
void t_placer_costs::update_norm_factors() {
    if (place_algorithm == PATH_TIMING_DRIVEN_PLACE) {
        bb_cost_norm = 1 / bb_cost;
        //Prevent the norm factor from going to infinity
        timing_cost_norm = std::min(1 / timing_cost, MAX_INV_TIMING_COST);
        cost = 1;       //The value of cost will be reset to 1 if timing driven
    } else {            //place_algorithm == BOUNDING_BOX_PLACE
        cost = bb_cost; //The cost value should be identical to the wirelength cost
    }
}

///@brief Constructor: Initialize all annealing state variables.
t_annealing_state::t_annealing_state(const t_annealing_sched& annealing_sched,
                                     float first_t,
                                     float first_rlim,
                                     int first_move_lim,
                                     float first_crit_exponent) {
    alpha = annealing_sched.alpha_min;
    t = first_t;
    restart_t = first_t;
    rlim = first_rlim;
    inverse_delta_rlim = 1 / (first_rlim - FINAL_RLIM);
    move_lim_max = first_move_lim;
    crit_exponent = first_crit_exponent;

    //Determine the current move_lim based on the schedule type
    if (annealing_sched.type == DUSTY_SCHED) {
        move_lim = std::max(1, (int)(move_lim_max * annealing_sched.success_target));
    } else {
        move_lim = move_lim_max;
    }
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

    auto device_size = device_ctx.grid.width() * device_ctx.grid.height();
    auto num_blocks = cluster_ctx.clb_nlist.blocks().size();

    int move_lim;
    if (placer_opts.effort_scaling == e_place_effort_scaling::CIRCUIT) {
        move_lim = int(annealing_sched.inner_num * pow(num_blocks, 4. / 3.));
    } else {
        VTR_ASSERT_MSG(
            placer_opts.effort_scaling == e_place_effort_scaling::DEVICE_CIRCUIT,
            "Unrecognized placer effort scaling");

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
 * Returns true until the schedule is finished.
 */
bool update_annealing_state(t_annealing_state* state,
                            float success_rat,
                            const t_placer_costs& costs,
                            const t_placer_opts& placer_opts,
                            const t_annealing_sched& annealing_sched) {
    /* Return `false` when the exit criterion is met. */
    if (annealing_sched.type == USER_SCHED) {
        state->t *= annealing_sched.alpha_t;
        return state->t >= annealing_sched.exit_t;
    }

    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    /* Automatic annealing schedule */
    float t_exit = 0.005 * costs.cost / cluster_ctx.clb_nlist.nets().size();

    if (annealing_sched.type == DUSTY_SCHED) {
        bool restart_temp = state->t < t_exit || std::isnan(t_exit); //May get nan if there are no nets
        if (success_rat < annealing_sched.success_min || restart_temp) {
            if (state->alpha > annealing_sched.alpha_max) return false;
            state->t = state->restart_t / sqrt(state->alpha); // Take a half step from the restart temperature.
            state->alpha = 1.0 - ((1.0 - state->alpha) * annealing_sched.alpha_decay);
        } else {
            if (success_rat > annealing_sched.success_target) {
                state->restart_t = state->t;
            }
            state->t *= state->alpha;
        }
        state->move_lim = std::max(1, std::min(state->move_lim_max, (int)(state->move_lim_max * (annealing_sched.success_target / success_rat))));
    } else { /* annealing_sched.type == AUTO_SCHED */
        if (success_rat > 0.96) {
            state->alpha = 0.5;
        } else if (success_rat > 0.8) {
            state->alpha = 0.9;
        } else if (success_rat > 0.15 || state->rlim > 1.) {
            state->alpha = 0.95;
        } else {
            state->alpha = 0.8;
        }
        state->t *= state->alpha;

        // Must be duplicated to retain previous behavior
        if (state->t < t_exit || std::isnan(t_exit)) return false;
    }

    // Gradually changes from the initial crit_exponent to the final crit_exponent based on how much the range limit has shrunk.
    // The idea is that as the range limit shrinks (indicating we are fine-tuning a more optimized placement) we can focus more on a smaller number of critical connections, which a higher crit_exponent achieves.
    update_rlim(&state->rlim, success_rat, device_ctx.grid);

    if (placer_opts.place_algorithm == PATH_TIMING_DRIVEN_PLACE) {
        state->crit_exponent = (1 - (state->rlim - state->final_rlim()) * state->inverse_delta_rlim)
                                   * (placer_opts.td_place_exp_last - placer_opts.td_place_exp_first)
                               + placer_opts.td_place_exp_first;
    }

    return true;
}

/**
 * @brief Update the range limited to keep acceptance prob. near 0.44.
 *
 * Use a floating point rlim to allow gradual transitions at low temps.
 */
static void update_rlim(float* rlim, float success_rat, const DeviceGrid& grid) {
    float upper_lim = std::max(grid.width() - 1, grid.height() - 1);

    *rlim *= (1. - 0.44 + success_rat);
    *rlim = std::max(std::min(*rlim, upper_lim), 1.f);
}
