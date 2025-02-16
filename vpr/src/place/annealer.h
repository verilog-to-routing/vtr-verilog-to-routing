
#pragma once

#include "vpr_types.h"

#include "move_generator.h" // movestats
#include "net_cost_handler.h"
#include "manual_move_generator.h"

#include <optional>
#include <tuple>

class PlaceMacros;
class PlacerState;
class t_placer_costs;
struct t_placer_opts;
enum class e_agent_state;

class NocCostHandler;
class NetPinTimingInvalidator;
class PlacerSetupSlacks;

/**
 * These variables keep track of the number of swaps
 * rejected, accepted or aborted. The total number of swap attempts
 * is the sum of the three number.
 */
struct t_swap_stats {
    int num_swap_rejected = 0;
    int num_swap_accepted = 0;
    int num_swap_aborted = 0;
    int num_ts_called = 0;
};

/**
 * @brief Stores variables that are used by the annealing process.
 *
 * This structure is updated by update_annealing_state() on each outer
 * loop iteration. It stores various important variables that need to
 * be accessed during the placement inner loop.
 *
 * Public members:
 *   @param t
 *              Temperature for simulated annealing.
 *   @param alpha
 *              Temperature decays factor (multiplied each outer loop iteration).
 *   @param num_temps
 *              The count of how many temperature iterations have passed.
 *   @param rlim
 *              Range limit for block swaps.
 *              Currently only updated by AUTO_SCHED.
 *   @param crit_exponent
 *              Used by timing-driven placement to "sharpen" the timing criticality.
 *              Depends on rlim. Currently only updated by AUTO_SCHED.
 *   @param move_lim
 *              Current block move limit.
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
    t_annealing_state() = default;
    t_annealing_state(float first_t,
                      float first_rlim,
                      int first_move_lim,
                      float first_crit_exponent);

  public: //Mutator
    /**
     * @brief Update the annealing state according to the annealing schedule selected.
     *
     *   USER_SCHED:  A manual fixed schedule with fixed alpha and exit criteria.
     *   AUTO_SCHED:  A more sophisticated schedule where alpha varies based on success ratio.
     *
     * @return True->continues the annealing. False->exits the annealing.
     */
    bool outer_loop_update(float success_rate,
                           const t_placer_costs& costs,
                           const t_placer_opts& placer_opts);

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
};

/**
 * @class PlacementAnnealer
 * @brief Simulated annealing optimizer for minimizing placement cost via block swaps.
 *
 * @details This class implements simulated annealing to optimize placement cost by swapping clustered blocks.
 * Swaps that reduce the cost are always accepted, while those that increase the cost are accepted
 * with a diminishing probability.
 *
 * The annealing process consists of two nested loops:
 * - The **inner loop** (implemented in `placement_inner_loop()`) performs individual swaps, all evaluated at a fixed temperature.
 * - The **outer loop** adjusts the temperature and determines whether further iterations are needed.
 *
 * Usage workflow:
 * 1. Call `outer_loop_update_timing_info()` to update timing information.
 * 2. Execute `placement_inner_loop()` for swap evaluations.
 * 3. Call `outer_loop_update_state()` to check if more outer loop iterations are needed.
 * 4. Optionally, use `start_quench()` to set the temperature to zero for a greedy optimization (quenching stage),
 *    then repeat steps 1 and 2.
 *
 *    Usage example:
 *    **************************************
 *    PlacementAnnealer annealer(...);
 *
 *    do {
 *      annealer.outer_loop_update_timing_info();
 *      annealer.placement_inner_loop();
 *    } while (annealer.outer_loop_update_state());
 *
 *    annealer.start_quench();
 *    annealer.outer_loop_update_timing_info();
 *    annealer.placement_inner_loop();
 *    **************************************
 */
class PlacementAnnealer {
  public:
    PlacementAnnealer(const t_placer_opts& placer_opts,
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
                      int move_lim);

    /**
     * @brief Contains the inner loop of the simulated annealing that performs
     * a certain number of swaps with a single temperature
     */
    void placement_inner_loop();

    /**
     * @brief Updates the setup slacks and criticalities before the inner loop
     * of the annealing/quench. It also updates normalization factors for different
     * placement cost terms.
     */
    void outer_loop_update_timing_info();

    /**
     * @brief Update the annealing state according to the annealing schedule selected.
     * @return True->continues the annealing. False->exits the annealing.
     */
    bool outer_loop_update_state();

    /**
     * @brief Starts the quench stage in simulated annealing by
     * setting the temperature to zero and reverting the move range limit
     * to the initial value.
     */
    void start_quench();

    /// @brief Returns the total number iterations (attempted swaps).
    int get_total_iteration() const;

    /// @brief Return the RL agent's state
    e_agent_state get_agent_state() const;

    /// @brief Returns a constant reference to the annealing state
    const t_annealing_state& get_annealing_state() const;

    /// @brief Returns constant references to different statistics objects
    std::tuple<const t_swap_stats&, const MoveTypeStat&, const t_placer_statistics&> get_stats() const;

    /**
     * @brief Returns MoveAbortionLogger to report how many moves
     * were aborted for each reason.
     * @return A constant reference to a  MoveAbortionLogger object.
     */
    const MoveAbortionLogger& get_move_abortion_logger() const;

  private:

    /**
     * @brief Pick some block and moves it to another spot.
     *
     * If the new location is empty, directly move the block. If the new location
     * is occupied, switch the blocks. Due to the different sizes of the blocks,
     * this block switching may occur for multiple times. It might also cause the
     * current swap attempt to abort due to inability to find suitable locations
     * for moved blocks.
     *
     * The move generator will record all the switched blocks in the variable
     * `blocks_affected`. Afterwards, the move will be assessed by the chosen
     * cost formulation. Currently, there are three ways to assess move cost,
     * which are stored in the enum type `t_place_algorithm`.
     *
     * @return Whether the block swap is accepted, rejected or aborted.
     */
    e_move_result try_swap_(MoveGenerator& move_generator,
                            const t_place_algorithm& place_algorithm,
                            bool manual_move_enabled);

    /**
     * @brief Determines whether a move should be accepted or not.
     * Moves with negative delta cost are always accepted, but
     * moves that increase the total cost are accepted with a
     * probability that diminishes as the temperature decreases.
     * @param delta_c The cost difference if the move is accepted.
     * @param t The annealer's temperature.
     * @return Whether the move is accepted or not.
     */
    e_move_result assess_swap_(double delta_c, double t);

    /// @brief Find the starting temperature for the annealing loop.
    float estimate_starting_temperature_();

  private:
    const t_placer_opts& placer_opts_;
    PlacerState& placer_state_;
    const PlaceMacros& place_macros_;
    /// Stores different placement cost terms
    t_placer_costs& costs_;
    /// Computes bounding box for each cluster net
    NetCostHandler& net_cost_handler_;
    /// Computes NoC-related cost terms when NoC optimization are enabled
    std::optional<NocCostHandler>& noc_cost_handler_;
    /// Contains weighting factors for NoC-related cost terms
    const t_noc_opts& noc_opts_;
    /// Random number generator for selecting random blocks and random locations
    vtr::RngContainer& rng_;

    /// The move generator used in the first state of RL agent and initial temperature computation
    std::unique_ptr<MoveGenerator> move_generator_1_;
    /// The move generator used in the second state of RL agent
    std::unique_ptr<MoveGenerator> move_generator_2_;
    /// Handles manual swaps proposed by the user through graphical user interface
    ManualMoveGenerator manual_move_generator_;
    /// RL agent state
    e_agent_state agent_state_;

    const PlaceDelayModel* delay_model_;
    PlacerCriticalities* criticalities_;
    PlacerSetupSlacks* setup_slacks_;
    SetupTimingInfo* timing_info_;
    NetPinTimingInvalidator* pin_timing_invalidator_;
    std::unique_ptr<FILE, decltype(&vtr::fclose)> move_stats_file_;
    int outer_crit_iter_count_;

    t_annealing_state annealing_state_;
    /// Swap statistics keep record of the number accepted/rejected/aborted swaps.
    t_swap_stats swap_stats_;
    MoveTypeStat move_type_stats_;
    t_placer_statistics placer_stats_;

    /// Keep record of moved blocks and affected pins in a swap
    t_pl_blocks_to_be_moved blocks_affected_;

  private:
    /**
     * @brief The maximum number of swap attempts before invoking the
     * once-in-a-while placement legality check as well as floating point
     * variables round-offs check.
     */
    static constexpr int MAX_MOVES_BEFORE_RECOMPUTE = 500000;

    /// Specifies how often (after how many swaps) timing information is recomputed
    /// when the annealer isn't in the quench stage
    int inner_recompute_limit_;
    /// Specifies how often timing information is recomputed when the annealer is in the quench stage
    int quench_recompute_limit_;
    /// Used to trigger a BB and NoC cost re-computation from scratch
    int moves_since_cost_recompute_;
    /// Total number of iterations (attempted swaps).
    int tot_iter_;
    /// Indicates whether the annealer has entered into the quench stage
    bool quench_started_;

    void LOG_MOVE_STATS_HEADER();
    void LOG_MOVE_STATS_PROPOSED();
    void LOG_MOVE_STATS_OUTCOME(double delta_cost, double delta_bb_cost, double delta_td_cost,
                                const char* outcome, const char* reason);

    /**
     * @brief Defines the RL agent's reward function factor constant. This factor controls the weight of bb cost
     * compared to the timing cost in the agent's reward function. The reward is calculated as
     * -1*(1.5-REWARD_BB_TIMING_RELATIVE_WEIGHT)*timing_cost + (1+REWARD_BB_TIMING_RELATIVE_WEIGHT)*bb_cost)
     */
    static constexpr float REWARD_BB_TIMING_RELATIVE_WEIGHT = 0.4;
};
