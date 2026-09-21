#pragma once

#include <vector>
#include <cstddef>
#include <limits>

struct t_router_opts;

/**
 * @brief Summary of the linear fit behind the most recent success-iteration estimate.
 */
struct t_routing_predictor_fit {
    float slope = std::numeric_limits<float>::quiet_NaN();       ///< Fitted slope, in log(overuse) per iteration
    float y_intercept = std::numeric_limits<float>::quiet_NaN(); ///< Fitted log(overuse) at iteration zero
    size_t first_iteration = 0;                                  ///< First routing iteration included in the fit
    size_t last_iteration = 0;                                   ///< Last routing iteration included in the fit
    size_t num_samples = 0;                                      ///< Number of iterations included in the fit
};

/**
 * @brief Tracks per-iteration routing overuse and predicts when routing will become legal.
 *
 * The predictor fits a line to the log of the recent overuse history and extrapolates
 * the iteration at which overuse reaches zero. It also owns the routing failure
 * prediction policy: should_abort_routing() decides whether the router should give up
 * based on the configured --routing_failure_predictor mode.
 */
class RoutingPredictor {
  public:
    ///@brief Configures the predictor and its abort policy from the router options
    explicit RoutingPredictor(const t_router_opts& router_opts);

    //Returns the estimated iteration when routing will succeed.
    float estimate_success_iteration() const;

    //Returns the current estimated slope (RR nodes per iteration)
    float estimate_overuse_slope();

    /**
     * @brief Records the overuse measured for a routing iteration, and updates all
     *        derived state. Call once per routing iteration.
     */
    void add_iteration_overuse(size_t iteration, size_t overused_rr_node_count);

    float get_slope() const;

    /**
     * @brief Returns whether the router should give up because a legal routing is
     *        predicted to take too many iterations. Logs the reason when it returns true.
     */
    bool should_abort_routing() const;

  private:
    /**
     * @brief Returns whether the current estimate_success_iteration() result should be
     *        trusted when deciding to abort routing.
     */
    bool prediction_is_valid_() const;

    ///@brief True while safe mode is tolerating the predictor's initial run of degenerate fits
    bool awaiting_usable_prediction_() const;

    ///@brief Fits a linear model to the log of the last history_factor of the overuse history
    t_routing_predictor_fit fit_model_(float history_factor) const;

    size_t min_history_;              ///< Number of iterations recorded before any estimate is made
    bool safe_mode_;                  ///< True for the SAFE routing failure predictor mode
    int verbosity_;                   ///< Router verbosity level controlling diagnostic output
    float history_factor_;            ///< Fraction of the recorded history used for the success-iteration fit
    float abort_iteration_threshold_; ///< Estimated success iteration above which routing is abandoned (infinity disables aborting)

    std::vector<size_t> iterations_;                                ///< Routing iterations recorded so far
    std::vector<size_t> iteration_overused_rr_node_counts_;         ///< Overused RR node count for each recorded iteration
    float slope_;                                                   ///< Cached slope of the most recent fit
    t_routing_predictor_fit last_fit_;                              ///< Fit reflecting the most recent add_iteration_overuse() call
    float last_estimate_ = std::numeric_limits<float>::quiet_NaN(); ///< Success-iteration estimate reflecting the most recent add_iteration_overuse() call
    size_t initial_degenerate_predictions_ = 0;                     ///< Length of the predictor's initial run of degenerate (non-extrapolable) estimates; frozen once has_extrapolated_ is set
    bool has_extrapolated_ = false;                                 ///< True once the predictor has produced a finite estimate; permanently ends the safe-mode grace period
};
