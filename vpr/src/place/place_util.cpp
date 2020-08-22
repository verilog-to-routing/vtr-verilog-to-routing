#include "place_util.h"
#include "globals.h"

static vtr::Matrix<t_grid_blocks> init_grid_blocks();

void init_placement_context() {
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    place_ctx.block_locs.clear();
    place_ctx.block_locs.resize(cluster_ctx.clb_nlist.blocks().size());

    place_ctx.grid_blocks = init_grid_blocks();
}

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
