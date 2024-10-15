
#include "annealer.h"

#include <algorithm>
#include <cmath>

#include "globals.h"
#include "draw_global.h"
#include "vpr_types.h"
#include "place_util.h"

///@brief Constructor: Initialize all annealing state variables and macros.
t_annealing_state::t_annealing_state(const t_annealing_sched& annealing_sched,
                                     float first_t,
                                     float first_rlim,
                                     int first_move_lim,
                                     float first_crit_exponent) {
    num_temps = 0;
    alpha = annealing_sched.alpha_min;
    t = first_t;
    restart_t = first_t;
    rlim = first_rlim;
    move_lim_max = first_move_lim;
    crit_exponent = first_crit_exponent;

    /* Determine the current move_lim based on the schedule type */
    if (annealing_sched.type == e_sched_type::DUSTY_SCHED) {
        move_lim = std::max(1, (int)(move_lim_max * annealing_sched.success_target));
    } else {
        move_lim = move_lim_max;
    }

    /* Store this inverse value for speed when updating crit_exponent. */
    INVERSE_DELTA_RLIM = 1 / (first_rlim - FINAL_RLIM);

    /* The range limit cannot exceed the largest grid size. */
    const auto& grid = g_vpr_ctx.device().grid;
    UPPER_RLIM = std::max(grid.width() - 1, grid.height() - 1);
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

    if (annealing_sched.type == e_sched_type::USER_SCHED) {
        /* Update t with user specified alpha. */
        t *= annealing_sched.alpha_t;

        /* Check if the exit criterion is met. */
        bool exit_anneal = t >= annealing_sched.exit_t;

        return exit_anneal;
    }

    /* Automatically determine exit temperature. */
    auto& cluster_ctx = g_vpr_ctx.clustering();
    float t_exit = 0.005 * costs.cost / cluster_ctx.clb_nlist.nets().size();

    if (annealing_sched.type == e_sched_type::DUSTY_SCHED) {
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
        VTR_ASSERT_SAFE(annealing_sched.type == e_sched_type::AUTO_SCHED);
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