#pragma once

#include <vector>
#include <cstddef>
#include <limits>

//When the estimated number of routing iterations exceeds these factors
//(for SAFE or AGGRESSIVE mode respectively) times the max router iterations
//specified by the router aborts early
constexpr float ROUTING_PREDICTOR_ITERATION_ABORT_FACTOR_SAFE = 3;
constexpr float ROUTING_PREDICTOR_ITERATION_ABORT_FACTOR_AGGRESSIVE = 1.5;

//If the number of overused resources is below this threshold do not abort.
// This avoids giving up when solutions are nearly legal, but converging slowly
constexpr size_t ROUTING_PREDICTOR_MIN_ABSOLUTE_OVERUSE_THRESHOLD = 100;

// If overuse is flat or increasing, the predictor cannot extrapolate and returns infinity.
// In SAFE mode, allow a few such predictions before giving up.
constexpr size_t ROUTING_PREDICTOR_MAX_DEGENERATE_ITERATIONS = 10;

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

class RoutingPredictor {
  public:
    RoutingPredictor(size_t min_history = 8, bool safe_mode = false, int verbosity = 0, float history_factor = 0.5);

    //Returns the estimated iteration when routing will succeed
    float estimate_success_iteration();

    //Returns the current estimated slope (RR nodes per iteration)
    float estimate_overuse_slope();

    void add_iteration_overuse(size_t iteration, size_t overused_rr_node_count);

    float get_slope() const;

    ///@brief Returns the fit used by the most recent estimate_success_iteration() call
    const t_routing_predictor_fit& get_last_fit() const { return last_fit_; }

    /**
     * @brief Returns whether the most recent estimate_success_iteration() result should be
     *        trusted when deciding to abort routing.
     */
    bool prediction_is_valid(size_t num_overused_nodes);

  private:
    size_t min_history_;
    bool safe_mode_;
    int verbosity_;
    float history_factor_;

    std::vector<size_t> iterations_;
    std::vector<size_t> iteration_overused_rr_node_counts_;
    float slope_;
    t_routing_predictor_fit last_fit_;                              ///< Fit recorded by the most recent estimate_success_iteration() call
    float last_estimate_ = std::numeric_limits<float>::quiet_NaN(); ///< Result of the most recent estimate_success_iteration() call
    size_t initial_degenerate_predictions_ = 0;                     ///< Length of the predictor's initial run of degenerate (non-extrapolable) estimates
    bool has_extrapolated_ = false;                                 ///< True once the predictor has produced a finite estimate
};
