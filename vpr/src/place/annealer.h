
#pragma once

class t_placer_costs;
struct t_placer_opts;
struct t_annealing_sched;

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
    t_annealing_state() = delete;
    t_annealing_state(const t_annealing_sched& annealing_sched,
                      float first_t,
                      float first_rlim,
                      int first_move_lim,
                      float first_crit_exponent);

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

class PlacementAnnealer {

  private:
    t_annealing_state annealing_state_;
};