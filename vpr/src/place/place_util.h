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
 * @brief The error tolerance due to round off for the total cost computation.
 * When we check it from scratch vs. incrementally. 0.01 means that there is a 1% error tolerance.
 */
constexpr double PL_INCREMENTAL_COST_TOLERANCE = .01;

// forward declaration of t_placer_costs so that it can be used an argument
// in NocCostTerms constructor
class t_placer_costs;
class BlkLocRegistry;

struct t_pl_macro;

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
    explicit t_placer_costs(t_place_algorithm algo, bool noc)
        : place_algorithm(algo)
        , noc_enabled(noc) {}
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
     * @brief Compute the total normalized cost for a given placement. This
     * computation will vary depending on the placement modes.
     *
     * @param costs The current placement cost components and their normalization
     * factors
     * @param placer_opts Determines the placement mode
     * @param noc_opts Determines if placement includes the NoC
     * @return double The computed total cost of the current placement
     */
    double get_total_cost(const t_placer_opts& placer_opts, const t_noc_opts& noc_opts);

    /**
     * @brief Accumulates NoC cost difference terms
     *
     * @param noc_delta_cost Cost difference for NoC-related costs terms
     */
    t_placer_costs& operator+=(const NocCostTerms& noc_delta_cost);

  private:
    static constexpr double MAX_INV_TIMING_COST = 1.e12;
    t_place_algorithm place_algorithm;
    bool noc_enabled;
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
 * Allocates and load placement macros.
 *
 * Initialize both of them to empty states.
 */
void init_placement_context(BlkLocRegistry& blk_loc_registry);

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

/**
 * @brief Builds (alloc and load) legal_pos that holds all the legal locations for placement
 *
 * @param legal_pos a lookup of all subtiles by sub_tile type
 * legal_pos[0..device_ctx.num_block_types-1][0..num_sub_tiles - 1] = std::vector<t_pl_loc> of all the legal locations
 * of the proper tile type and sub_tile type
 */
void alloc_and_load_legal_placement_locations(std::vector<std::vector<std::vector<t_pl_loc>>>& legal_pos);

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
 * @param blk_loc_registry Placement block location information.
 *
 */
bool macro_can_be_placed(const t_pl_macro& pl_macro,
                         const t_pl_loc& head_pos,
                         bool check_all_legality,
                         const BlkLocRegistry& blk_loc_registry);
#endif
