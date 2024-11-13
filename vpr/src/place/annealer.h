
#pragma once

#include "vpr_types.h"

#include "move_generator.h" // movestats
#include "net_cost_handler.h"

#include <optional>
#include <tuple>

class PlacerState;
class t_placer_costs;
struct t_placer_opts;

class NocCostHandler;
class ManualMoveGenerator;
class NetPinTimingInvalidator;

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
 * Private variables are not given accessor functions. They serve as
 * macros originally defined in place.cpp as global scope variables.
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

    /**
     * @brief Update the move limit based on the success rate.
     *
     * The value is bounded between 1 and move_lim_max.
     */
    inline void update_move_lim(float success_target, float success_rate);
};


class PlacementAnnealer {
  public:
    PlacementAnnealer(const t_placer_opts& placer_opts,
                      PlacerState& placer_state,
                      t_placer_costs& costs,
                      NetCostHandler& net_cost_handler,
                      std::optional<NocCostHandler>& noc_cost_handler,
                      const t_noc_opts& noc_opts,
                      vtr::RngContainer& rng,
                      MoveGenerator& move_generator_1,
                      MoveGenerator& move_generator_2,
                      ManualMoveGenerator& manual_move_generator,
                      const PlaceDelayModel* delay_model,
                      PlacerCriticalities* criticalities,
                      PlacerSetupSlacks* setup_slacks,
                      SetupTimingInfo* timing_info,
                      NetPinTimingInvalidator* pin_timing_invalidator,
                      int move_lim);

    ///@brief Contains the inner loop of the simulated annealing
    void placement_inner_loop(MoveGenerator& move_generator,
                              float timing_bb_factor);

    void outer_loop_update_timing_info();

    bool outer_loop_update_state();

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
    e_move_result try_swap(MoveGenerator& move_generator,
                           const t_place_algorithm& place_algorithm,
                           float timing_bb_factor,
                           bool manual_move_enabled);

    ///@brief Returns the total number iterations or attempted swaps
    int get_total_iteration() const;

    ///@brief Returns a constant reference to the annealing state
    const t_annealing_state& get_annealing_state() const;

    std::tuple<const t_swap_stats&, const MoveTypeStat&, const t_placer_statistics&> get_stats() const;

    /**
     * @brief Starts the quench stage in simulated annealing by
     * setting the temperature to zero and reverting the move range limit
     * to the initial value.
     */
    void start_quench();

  private:
    e_move_result assess_swap_(double delta_c, double t);

  public:
    const t_placer_opts& placer_opts_;
    PlacerState& placer_state_;
    t_placer_costs& costs_;
    NetCostHandler& net_cost_handler_;
    std::optional<NocCostHandler>& noc_cost_handler_;
    const t_noc_opts& noc_opts_;
    vtr::RngContainer& rng_;

    MoveGenerator& move_generator_1_;
    MoveGenerator& move_generator_2_;
    ManualMoveGenerator& manual_move_generator_;

    const PlaceDelayModel* delay_model_;
    PlacerCriticalities* criticalities_;
    PlacerSetupSlacks* setup_slacks_;
    SetupTimingInfo* timing_info_;
    NetPinTimingInvalidator* pin_timing_invalidator_;
    std::unique_ptr<FILE, decltype(&vtr::fclose)> move_stats_file_;
    int outer_crit_iter_count_;

    t_annealing_state annealing_state_;
    ///Swap statistics keep record of the number accepted/rejected/aborted swaps.
    t_swap_stats swap_stats_;
    MoveTypeStat move_type_stats_;
    t_placer_statistics placer_stats_;

    t_pl_blocks_to_be_moved blocks_affected_;

  private:

    /**
     * @brief The maximum number of swap attempts before invoking the
     * once-in-a-while placement legality check as well as floating point
     * variables round-offs check.
     */
    static constexpr int MAX_MOVES_BEFORE_RECOMPUTE = 500000;

    ///Specifies how often timing information is recomputed when the annealer isn't in the quench stage
    int inner_recompute_limit_;
    ///Specifies how often timing information is recomputed when the annealer is in the quench stage
    int quench_recompute_limit_;
    ///Used to trigger a BB and NoC cost re-computation from scratch
    int moves_since_cost_recompute_;
    ///Total number of iterations or attempted swaps
    int tot_iter_;
    ///Indicates whether the annealer has entered into the quench stage
    bool quench_started_;

    void LOG_MOVE_STATS_HEADER();
    void LOG_MOVE_STATS_PROPOSED();
    void LOG_MOVE_STATS_OUTCOME(double delta_cost, double delta_bb_cost, double delta_td_cost,
                                const char* outcome, const char* reason);

  private:
    ///@brief Find the starting temperature for the annealing loop.
    float estimate_starting_temperature();
};