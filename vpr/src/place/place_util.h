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

// forward declaration of t_placer_costs so that it can be used an argument
// in NocCostTerms constructor
class t_placer_costs;

/**
 * @brief Data structure that stores different cost terms for NoC placement.
 * This data structure can also be used to store normalization and weighting
 * factors for NoC-related cost terms.
 *
 *   @param aggregate_bandwidth The aggregate NoC bandwidth cost. This is
 *   computed by summing all used link bandwidths.
 *   @param latency The NoC latency cost, calculated as the sum of latencies
 *   experienced by each traffic flow.
 *   @param latency_overrun Sum of latency overrun for traffic flows that have
 *   a latency constraint.
 *   @param congestion The NoC congestion cost, i.e. how over-utilized
 *   NoC links are. This is computed by dividing over-utilized bandwidth
 *   by link bandwidth, and summing all computed ratios.
 */
struct NocCostTerms {
  public:
    NocCostTerms();
    NocCostTerms(const NocCostTerms&) = default;
    NocCostTerms(double agg_bw, double lat, double lat_overrun, double congest);
    NocCostTerms& operator=(const NocCostTerms& other) = default;
    NocCostTerms& operator+=(const NocCostTerms& noc_delta_cost);

    double aggregate_bandwidth = 0.0;
    double latency = 0.0;
    double latency_overrun = 0.0;
    double congestion = 0.0;
};

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
 *   @param noc_cost_terms NoC-related cost terms
 *   @param noc_cost_norm_factors Normalization factors for NoC-related cost terms.
 *
 *   @param MAX_INV_TIMING_COST Stops inverse timing cost from going to infinity
 *              with very lax timing constraints, which avoids multiplying by a
 *              gigantic timing_cost_norm when auto-normalizing. The exact value
 *              of this cost has relatively little impact, but should be large
 *              enough to not affect the timing costs computation for normal
 *              constraints.
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

    NocCostTerms noc_cost_terms;
    NocCostTerms noc_cost_norm_factors;

  public: //Constructor
    explicit t_placer_costs(t_place_algorithm algo)
        : place_algorithm(algo) {}
    t_placer_costs() = default;

  public: //Mutator
    /**
     * @brief Mutator: updates the norm factors in the outer loop iteration.
     *
     * At each temperature change we update these values to be used
     * for normalizing the trade-off between timing and wirelength (bb)
     */
    void update_norm_factors();

    /**
     * @brief Accumulates NoC cost difference terms
     *
     * @param noc_delta_cost Cost difference for NoC-related costs terms
     */
    t_placer_costs& operator+=(const NocCostTerms& noc_delta_cost);

  private:
    double MAX_INV_TIMING_COST = 1.e12;
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
    int NUM_LAYERS = 1;

  public: //Constructor
    t_annealing_state(const t_annealing_sched& annealing_sched,
                      float first_t,
                      float first_rlim,
                      int first_move_lim,
                      float first_crit_exponent,
                      int num_layers);

  public: //Mutator
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
    bool outer_loop_update(float success_rate,
                           const t_placer_costs& costs,
                           const t_placer_opts& placer_opts,
                           const t_annealing_sched& annealing_sched);

  private: //Mutator
    /**
     * @brief Update the range limiter to keep acceptance prob. near 0.44.
     *
     * Use a floating point rlim to allow gradual transitions at low temps.
     * The range is bounded by 1 (FINAL_RLIM) and the grid size (UPPER_RLIM).
     */
    inline void update_rlim(float success_rate);

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
    inline void update_crit_exponent(const t_placer_opts& placer_opts);

    /**
     * @brief Update the move limit based on the success rate.
     *
     * The value is bounded between 1 and move_lim_max.
     */
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

/**
 * @brief Initialize the placer's block-grid dual direction mapping.
 *
 * Forward direction - block to grid: place_ctx.block_locs.
 * Reverse direction - grid to block: place_ctx.grid_blocks.
 *
 * Initialize both of them to empty states.
 */
void init_placement_context();

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
int get_initial_move_lim(const t_placer_opts& placer_opts, const t_annealing_sched& annealing_sched);

/**
 * @brief Returns the standard deviation of data set x.
 *
 * There are n sample points, sum_x_squared is the summation over n of x^2 and av_x
 * is the average x. All operations are done in double precision, since round off
 * error can be a problem in the initial temp. std_dev calculation for big circuits.
 */
double get_std_dev(int n, double sum_x_squared, double av_x);

///@brief Initialize usage to 0 and blockID to EMPTY_BLOCK_ID for all place_ctx.grid_block locations
void zero_initialize_grid_blocks();

///@brief a utility to calculate grid_blocks given the updated block_locs (used in restore_checkpoint)
void load_grid_blocks_from_block_locs();

/**
 * @brief Builds (alloc and load) legal_pos that holds all the legal locations for placement
 *
 *   @param legal_pos
 *              a lookup of all subtiles by sub_tile type
 *              legal_pos[0..device_ctx.num_block_types-1][0..num_sub_tiles - 1] = std::vector<t_pl_loc> of all the legal locations
 *              of the proper tile type and sub_tile type
 *
 */
void alloc_and_load_legal_placement_locations(std::vector<std::vector<std::vector<t_pl_loc>>>& legal_pos);

///@brief Performs error checking to see if location is legal for block type, and sets the location and grid usage of the block if it is legal.
void set_block_location(ClusterBlockId blk_id, const t_pl_loc& location);

/// @brief check if a specified location is within the device grid
inline bool is_loc_on_chip(t_physical_tile_loc loc) {
    const auto& grid = g_vpr_ctx.device().grid;
    int x = loc.x;
    int y = loc.y;
    int layer_num = loc.layer_num;
    //return false if the location is not within the chip
    return (layer_num >= 0 && layer_num < int(grid.get_num_layers()) && x >= 0 && x < int(grid.width()) && y >= 0 && y < int(grid.height()));
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
