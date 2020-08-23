#pragma once
#include "vpr_types.h"

///@brief Forward declarations.
class t_placer_costs;
class t_annealing_state;

///@brief Initialize the placement context
void init_placement_context();

///@brief Get the initial limit for inner loop block move attempt limit.
int get_initial_move_lim(const t_placer_opts& placer_opts, const t_annealing_sched& annealing_sched);

///@brief Update the annealing state according to the annealing schedule selected.
bool update_annealing_state(t_annealing_state* state,
                            float success_rat,
                            const t_placer_costs& costs,
                            const t_placer_opts& placer_opts,
                            const t_annealing_sched& annealing_sched);

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
  public:
    double cost;
    double bb_cost;
    double timing_cost;
    double bb_cost_norm;
    double timing_cost_norm;

  private:
    static constexpr double MAX_INV_TIMING_COST = 1.e9;
    enum e_place_algorithm place_algorithm;

  public: //Constructor
    t_placer_costs(enum e_place_algorithm algo);

  public: //Mutator
    void update_norm_factors();
};

/**
 * @brief Stores variables that are used by the annealing process.
 *
 * This structure is updated by update_annealing_state() on each outer
 * loop iteration. It stores various important variables that need to
 * be accessed during the placement inner loop.
 *
 *   @param t Temperature for simulated annealing.
 *   @param rlim Range limit for block swaps.
 *   @param inverse_delta_rlim Used to update crit_exponent.
 *   @param alpha Temperature decays factor (multiplied each outer loop iteration).
 *   @param restart_t Temperature used after restart due to minimum success ratio.
 *   @param crit_exponent Used by timing-driven placement to "sharpen" the timing criticality.
 *   @param move_lim_max Maximum block move limit.
 *   @param move_lim Current block move limit.
 *
 *   @param FINAL_RLIM The final rlim (range limit) is 1, which is the smallest value that
 *              can still make progress, since an rlim of 0 wouldn't allow any swaps.
 */
class t_annealing_state {
  public:
    float t;
    float rlim;
    float inverse_delta_rlim;
    float alpha;
    float restart_t;
    float crit_exponent;
    int move_lim_max;
    int move_lim;

  private:
    static constexpr float FINAL_RLIM = 1.;

  public: //Constructor
    t_annealing_state(const t_annealing_sched& annealing_sched,
                      float first_t,
                      float first_rlim,
                      int first_move_lim,
                      float first_crit_exponent);

  public: //Accessor
    float final_rlim() const { return FINAL_RLIM; }
};
