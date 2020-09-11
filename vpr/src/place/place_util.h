/**
 * @file place_util.h
 * @brief Utility structures representing various states of the
 *        placement. Also contains declarations of related routines.
 */

#pragma once
#include "vpr_types.h"

/**
 * @brief Data structure that stores different cost values in the placer.
 *
 * Although we do cost calculations with float values, we use doubles
 * for the accumulated costs to avoid round-off, particularly on large
 * designs where the magnitude of a single move's delta cost is small
 * compared to the overall cost.
 *
 * The cost normalization factors are updated upon every temperature change
 * in the outer_loop_update_timing_info routine. They are the multiplicative
 * inverses of their respective cost values when the routine is called. They
 * serve to normalize the trade-off between timing and wirelength (bb).
 *
 *   @param cost The weighted average of the wiring cost and the timing cost.
 *   @param bb_cost The bounding box cost, aka the wiring cost.
 *   @param timing_cost The timing cost, which is connection delay * criticality.
 *
 *   @param bb_cost_norm The normalization factor for the wiring cost.
 *   @param timing_cost_norm The normalization factor for the timing cost, which
 *              is upper-bounded by the value of MAX_INV_TIMING_COST.
 *
 *   @param MAX_INV_TIMING_COST Stops inverse timing cost from going to infinity
 *              with very lax timing constraints, which avoids multiplying by a
 *              gigantic timing_cost_norm when auto-normalizing. The exact value
 *              of this cost has relatively little impact, but should not be large
 *              enough to be on the order of timing costs for normal constraints.
 *
 *   @param place_algorithm Determines how the member values are updated upon
 *              each temperature change during the placer annealing process.
 */
class t_placer_costs {
  public: //members
    double cost;
    double bb_cost;
    double timing_cost;
    double bb_cost_norm;
    double timing_cost_norm;

  public: //Constructor
    t_placer_costs(t_place_algorithm algo)
        : place_algorithm(algo) {}

  public: //Mutator
    void update_norm_factors();

  private:
    double MAX_INV_TIMING_COST = 1.e9;
    t_place_algorithm place_algorithm;
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
