/**
 * @file place_util.h
 * @brief Utility structures representing various states of the
 *        placement. Also contains declarations of related routines.
 */

#pragma once

struct t_placer_costs {
    //Although we do nost cost calculations with float's we
    //use doubles for the accumulated costs to avoid round-off,
    //particularly on large designs where the magnitude of a single
    //move's delta cost is small compared to the overall cost.
    double cost;
    double bb_cost;
    double timing_cost;
};

struct t_placer_prev_inverse_costs {
    double bb_cost;
    double timing_cost;
};

// Used by update_annealing_state()
struct t_annealing_state {
    float t;                  // Temperature
    float rlim;               // Range limit for swaps
    float inverse_delta_rlim; // used to calculate crit_exponent
    float alpha;              // Temperature decays by this factor each outer iteration
    float restart_t;          // Temperature used after restart due to minimum success ratio
    float crit_exponent;      // Used by timing-driven placement to "sharpen" timing criticality
    int move_lim_max;         // Maximum move limit
    int move_lim;             // Current move limit
};

//Initialize the placement context
void init_placement_context();
