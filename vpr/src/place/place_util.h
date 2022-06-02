/**
 * @file place_util.h
 * @brief Utility structures representing various states of the
 *        placement and utility functions used by the placer.
 */

#ifndef PLACE_UTIL_H
#define PLACE_UTIL_H
#include <string>
#include "vpr_types.h"
#include "vtr_util.h"
#include "vtr_vector_map.h"
#include "globals.h"

/**
 * @brief Data structure that stores different cost values in the placer.
 *
 * Although we do cost calculations with float values, we use doubles
 * for the accumulated costs to avoid round-off, particularly on large
 * designs where the magnitude of a single move's delta cost is small
 * compared to the overall cost.
 *
 * To balance the trade-off between timing and wirelength (bb) cost, the
 * change in costs produced by block swaps are divided by the final cost
 * values of the previous iteration. However, the divisions are expensive,
 * so we store their multiplicative inverses when they are updated in
 * the outer loop routines to speed up the normalization process.
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
    double cost = 0.;
    double bb_cost = 0.;
    double timing_cost = 0.;
    double bb_cost_norm = 0.;
    double timing_cost_norm = 0.;

  public: //Constructor
    t_placer_costs(t_place_algorithm algo)
        : place_algorithm(algo) {}
    t_placer_costs() {}

  public: //Mutator
    void update_norm_factors();

  private:
    double MAX_INV_TIMING_COST = 1.e9;
    t_place_algorithm place_algorithm;
};

/**
 * @brief Stores variables that are used by the annealing process.
 *
 * This structure is updated by update_annealing_state() on each outer
 * loop iteration. It stores various important variables that need to
 * be accessed during the placement inner loop.
 *
 * Private variables are not given accessor functions. They serve as
 * macros originally defined in place.cpp as global scope variables.
 *
 * Public members:
 *   @param t
 *              Temperature for simulated annealing.
 *   @param restart_t
 *              Temperature used after restart due to minimum success ratio.
 *              Currently only used and updated by DUSTY_SCHED.
 *   @param alpha
 *              Temperature decays factor (multiplied each outer loop iteration).
 *   @param num_temps
 *              The count of how many temperature iterations have passed.
 *
 *   @param rlim
 *              Range limit for block swaps.
 *              Currently only updated by DUSTY_SCHED and AUTO_SCHED.
 *   @param crit_exponent
 *              Used by timing-driven placement to "sharpen" the timing criticality.
 *              Depends on rlim. Currently only updated by DUSTY_SCHED and AUTO_SCHED.
 *   @param move_lim
 *              Current block move limit.
 *              Currently only updated by DUSTY_SCHED.
 *   @param move_lim_max
 *              Maximum block move limit.
 *
 * Private members:
 *   @param UPPER_RLIM
 *              The upper limit for the range limiter value.
 *   @param FINAL_RLIM
 *              The final rlim (range limit) is 1, which is the smallest value that
 *              can still make progress, since an rlim of 0 wouldn't allow any swaps.
 *   @param INVERSE_DELTA_RLIM
 *              Used to update crit_exponent. See update_rlim() for more.
 *
 * Mutators:
 *   @param outer_loop_update()
 *              Update the annealing state variables in the placement outer loop.
 *   @param update_rlim(), update_crit_exponent(), update_move_lim()
 *              Inline subroutines used by the main routine outer_loop_update().
 */
class t_annealing_state {
  public:
    float t;
    float restart_t;
    float alpha;
    int num_temps;

    float rlim;
    float crit_exponent;
    int move_lim;
    int move_lim_max;

  private:
    float UPPER_RLIM;
    float FINAL_RLIM = 1.;
    float INVERSE_DELTA_RLIM;

  public: //Constructor
    t_annealing_state(const t_annealing_sched& annealing_sched,
                      float first_t,
                      float first_rlim,
                      int first_move_lim,
                      float first_crit_exponent);

  public: //Mutator
    bool outer_loop_update(float success_rate,
                           const t_placer_costs& costs,
                           const t_placer_opts& placer_opts,
                           const t_annealing_sched& annealing_sched);

  private: //Mutator
    inline void update_rlim(float success_rate);
    inline void update_crit_exponent(const t_placer_opts& placer_opts);
    inline void update_move_lim(float success_target, float success_rate);
};

/**
 * @brief Stores statistics produced by a single annealing iteration.
 *
 * This structure is refreshed at the beginning of every annealing loop
 * by calling reset(). Whenever a block swap move is accepted, this
 * structure calls single_swap_update() to update its variables. At the
 * end of the current iteration, it calls calc_iteration_stats() to
 * summarize the results (success_rate & std_dev of the total costs).
 *
 * In terms of calculating statistics for total cost, we mean that we
 * operate upon the set of placer cost values gathered after every
 * accepted block move.
 *
 *   @param av_cost
 *              Average total cost. Cost formulation depends on
 *              the place algorithm currently being used.
 *   @param av_bb_cost
 *              Average bounding box (wiring) cost.
 *   @param av_timing_cost
 *              Average timing cost (delay * criticality).
 *   @param sum_of_squares
 *              Sum of squares of the total cost.
 *   @param success_num
 *              Number of accepted block swaps for the current iteration.
 *   @param success_rate
 *              num_accepted / total_trials for the current iteration.
 *   @param std_dev
 *              Standard deviation of the total cost.
 *
 */
class t_placer_statistics {
  public:
    double av_cost;
    double av_bb_cost;
    double av_timing_cost;
    double sum_of_squares;
    int success_sum;
    float success_rate;
    double std_dev;

  public: //Constructor
    t_placer_statistics() { reset(); }

  public: //Mutator
    ///@brief Clear all data fields.
    void reset();

    ///@brief Update stats when a single swap move has been accepted.
    void calc_iteration_stats(const t_placer_costs& costs, int move_lim);

    ///@brief Calculate placer success rate and cost std_dev for this iteration.
    void single_swap_update(const t_placer_costs& costs);
};

///@brief Initialize the placer's block-grid dual direction mapping.
void init_placement_context();

///@brief Get the initial limit for inner loop block move attempt limit.
int get_initial_move_lim(const t_placer_opts& placer_opts, const t_annealing_sched& annealing_sched);

///@brief Returns the standard deviation of data set x.
double get_std_dev(int n, double sum_x_squared, double av_x);

///@brief Initialize usage to 0 and blockID to EMPTY_BLOCK_ID for all place_ctx.grid_block locations
void zero_initialize_grid_blocks();

///@brief a utility to calculate grid_blocks given the updated block_locs (used in restore_checkpoint)
void load_grid_blocks_from_block_locs();

///@brief Builds legal_pos structure. legal_pos[type->index] is an array that gives every legal value of (x,y,z) that can accommodate a block.
void alloc_and_load_legal_placement_locations(std::vector<std::vector<std::vector<t_pl_loc>>>& legal_pos);

///@brief Performs error checking to see if location is legal for block type, and sets the location and grid usage of the block if it is legal.
void set_block_location(ClusterBlockId blk_id, const t_pl_loc& location);

/// @brief check if a specified location is within the device grid
inline bool is_loc_on_chip(int x, int y) {
    auto& device_ctx = g_vpr_ctx.device();
    //return false if the location is not within the chip
    return (x >= 0 && x < int(device_ctx.grid.width()) && y >= 0 && y < int(device_ctx.grid.height()));
}

/**
 * @brief  Checks that each macro member location is legal based on the head position and its offset
 *   
 * If the function is called from initial placement or simulated annealing placer,
 * it should ensure that the macro placement is entirely legal. Each macro member 
 * should be placed in a location with the right type that can accommodate more
 * blocks, and floorplanning constraint should also be checked.
 * If the function is called from analytical placement, it should only ensure 
 * that all macro members are placed within the chip. The overused blocks will
 * be spread by the strict_legalizer function. Floorplanning constraint is also not supported
 * by analytical placer.
 *  
 * @param pl_macro
 *        macro's member can be accessible from pl_macro parameter. 
 * @param head_pos
 *        head_pos is the macro's head location.
 * @param check_all_legality
 *        determines whether the routine should check all legality constraint 
 *        Analytic placer does not require to check block's capacity or
 *        floorplanning constraints. However, initial placement or SA-based approach
 *        require to check for all legality constraints.
 */
bool macro_can_be_placed(t_pl_macro pl_macro, t_pl_loc head_pos, bool check_all_legality);

#endif
