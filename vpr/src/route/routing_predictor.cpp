#include <limits>
#include <numeric>
#include <cmath>

#include "vtr_assert.h"
#include "vtr_log.h"

#include "routing_predictor.h"

namespace {

class LinearModel {
  public:
    LinearModel(float slope = std::numeric_limits<float>::quiet_NaN(), float y_intercept = std::numeric_limits<float>::quiet_NaN())
        : slope_(slope)
        , y_intercept_(y_intercept) {
    }

    float find_x_for_y_value(float y_value) const {
        //y = m*x + b
        //x = (y - b) / m

        return (y_value - y_intercept_) / slope_;
    }

    float get_slope() const {
        return slope_;
    }

    float find_y_for_x_value(float x_value) const {
        //y = m*x + b
        return slope_ * x_value + y_intercept_;
    }

  private:
    float slope_;
    float y_intercept_;
};

template<typename T>
float variance(const std::vector<T>& values, float avg) {
    float var = 0;
    for (float val : values) {
        var += (val - avg) * (val - avg);
    }

    return var;
}

float covariance(const std::vector<size_t>& x_values, const std::vector<float>& y_values, float x_avg, float y_avg) {
    VTR_ASSERT(x_values.size() == y_values.size());

    float cov = 0;
    for (size_t i = 0; i < x_values.size(); ++i) {
        cov += (x_values[i] - x_avg) * (y_values[i] - y_avg);
    }

    return cov;
}

LinearModel simple_linear_regression(const std::vector<size_t>& x_values, const std::vector<float>& y_values) {
    float y_avg = std::accumulate(y_values.begin(), y_values.end(), 0.) / y_values.size();
    float x_avg = std::accumulate(x_values.begin(), x_values.end(), 0.) / x_values.size();

    float covariance_x_y = covariance(x_values, y_values, x_avg, y_avg);
    float variance_x = variance(x_values, x_avg);

    float beta = covariance_x_y / variance_x;
    float alpha = y_avg - beta * x_avg;

    return LinearModel(beta, alpha);
}

} // namespace

float RoutingPredictor::get_slope() const {
    //Return cached slope, computed in add_iteration_overuse()
    return slope_;
}

t_routing_predictor_fit RoutingPredictor::fit_model_(float history_factor) const {
    //For pathfinder-based routing overuse tends to follow a negative-exponential:
    //
    //    ^
    //    | *
    //    |
    //    |  *
    //  o |
    //  v |   *
    //  e |
    //  r |    *
    //  u |
    //  s |
    //  e |      *
    //    |           *
    //    |                     *
    //    |                                       *
    //    ------------------------------------------------>
    //                  iterations
    //
    //initially falling off rapidly but slowing down as the iterations increase
    //(intuitively the easy congestion is resolved quickly as the non-critical signals
    //are routed around, leaving only critical signals which compete for fast resources).
    //
    //A simple linear model will typically do a poor job of fitting an exponential.
    //However an exponential appears linear when plotted on a log-linear plot:
    //
    //    ^
    //    |
    //  l |
    //  o |  *
    //  g |
    //    |         *
    //  o |
    //  v |                 *
    //  e |
    //  r |                         *
    //  u |                              *
    //  s |                                   *
    //  e |                                        *
    //    |
    //    ------------------------------------------------>
    //                  iterations
    //
    //As a result we fit to the logarithm of the overuse, allowing us to capture the
    //exponential congestion behaviour with a simple linear model

    //We use the last history_factor of all iterations to perform our fit
    //This helps avoid the problem of under estimating convergence at the
    //end of the route when overuse is low, as compared to a fixed history length
    //(since the history inspected grows as the number of iterations increases,
    //later iterations use a longer history which helps reduce the noise caused by
    //small numbers of overused nodes)
    size_t start = iterations_.size() - std::round(history_factor * iterations_.size());
    size_t end = iterations_.size();

    //Calculate the log overuse for the history we are interested in
    std::vector<float> hist_log_overuse;
    std::vector<size_t> hist_iters;
    for (size_t i = start; i < end; ++i) {
        hist_log_overuse.push_back(std::log(iteration_overused_rr_node_counts_[i]));
        hist_iters.push_back(iterations_[i]);
    }

    //We fit a linear model to the log of the overuse, this keeps the model simple but
    //captures the (typically) negative-exponential behaviour of overuse
    VTR_ASSERT(!hist_iters.empty());
    LinearModel model = simple_linear_regression(hist_iters, hist_log_overuse);

    //The slope and y-intercept fully describe the fitted model; also record what
    //the fit was built from, so callers can report and interpret it
    t_routing_predictor_fit fit;
    fit.slope = model.get_slope();
    fit.y_intercept = model.find_y_for_x_value(0.);
    fit.first_iteration = hist_iters.front();
    fit.last_iteration = hist_iters.back();
    fit.num_samples = hist_iters.size();

    return fit;
}

RoutingPredictor::RoutingPredictor(size_t min_history, bool safe_mode, int verbosity, float history_factor)
    : min_history_(min_history)
    , safe_mode_(safe_mode)
    , verbosity_(verbosity)
    , history_factor_(history_factor)
    , slope_(-1) {
    //nop
}

float RoutingPredictor::estimate_success_iteration() const {
    return last_estimate_;
}

bool RoutingPredictor::prediction_is_valid() const {
    if (iteration_overused_rr_node_counts_.empty()
        || iteration_overused_rr_node_counts_.back() <= ROUTING_PREDICTOR_MIN_ABSOLUTE_OVERUSE_THRESHOLD) {
        //Only consider the prediction actionable if there is a significant number of
        //overused resources; near-legal routings may converge slowly
        return false;
    }

    return !std::isnan(last_estimate_) && !awaiting_usable_prediction_();
}

bool RoutingPredictor::awaiting_usable_prediction_() const {
    // In safe mode, tolerate an initial run of degenerate fits rather than treating
    // their infinite estimates as predictions that routing will never converge
    return safe_mode_
           && std::isinf(last_estimate_)
           && !has_extrapolated_
           && initial_degenerate_predictions_ <= ROUTING_PREDICTOR_MAX_DEGENERATE_ITERATIONS;
}

float RoutingPredictor::estimate_overuse_slope() {
    //We use a fixed size sliding window of history to estimate the slope
    //This makes the slope estimate more 'recent' than the values used to estimate
    //the success iteration (although at the risk of being noisier).
    constexpr float FIXED_HISTORY_SIZE = 5; //# of previous iterations to consider

    float slope = std::numeric_limits<float>::quiet_NaN();

    float history_factor = FIXED_HISTORY_SIZE / iterations_.size(); //Fixed history size

    if (iterations_.size() >= FIXED_HISTORY_SIZE) {
        t_routing_predictor_fit fit = fit_model_(history_factor);
        LinearModel model(fit.slope, fit.y_intercept);

        float log_curr_usage = model.find_y_for_x_value(*(--iterations_.end()));
        float log_next_usage = model.find_y_for_x_value(*(--iterations_.end()) + 1);

        float curr_usage = std::exp(log_curr_usage);
        float next_usage = std::exp(log_next_usage);

        slope = next_usage - curr_usage;
    }

    return slope;
}

void RoutingPredictor::add_iteration_overuse(size_t iteration, size_t overused_rr_node_count) {
    VTR_ASSERT_MSG(iterations_.empty() || iteration > iterations_.back(),
                   "Routing iterations must be recorded once each, in increasing order");

    iterations_.push_back(iteration);
    iteration_overused_rr_node_counts_.push_back(overused_rr_node_count);

    //Update the cached fit, slope and success-iteration estimate
    last_fit_ = t_routing_predictor_fit();
    last_estimate_ = std::numeric_limits<float>::quiet_NaN();
    if (iterations_.size() > min_history_) {
        last_fit_ = fit_model_(history_factor_);
        slope_ = last_fit_.slope;

        LinearModel model(last_fit_.slope, last_fit_.y_intercept);
        last_estimate_ = model.find_x_for_y_value(0.);
        if (last_estimate_ < 0.) {
            //Iterations less than zero occurs when the slope is positive,
            //and the intercept is before the y-axis
            //
            // Note that this infinity records that the model could not extrapolate, rather
            // than a prediction that routing will never converge.
            last_estimate_ = std::numeric_limits<float>::infinity();
        }
    }

    if (overused_rr_node_count > ROUTING_PREDICTOR_MIN_ABSOLUTE_OVERUSE_THRESHOLD) {
        // An infinite estimate means the fit over the recent history has a non-negative
        // slope.
        if (!has_extrapolated_ && !std::isnan(last_estimate_)) {
            if (std::isinf(last_estimate_)) {
                ++initial_degenerate_predictions_;
            } else {
                has_extrapolated_ = true;
            }
        }

        VTR_LOGV(verbosity_ > 1 && last_fit_.num_samples > 0,
                 "Routing predictor: fit over iterations %zu-%zu (%zu samples), log-overuse slope %+.4g,"
                 " estimated success iteration %.1f%s\n",
                 last_fit_.first_iteration, last_fit_.last_iteration, last_fit_.num_samples,
                 last_fit_.slope, last_estimate_,
                 awaiting_usable_prediction_() ? " (waiting for an extrapolable fit)" : "");
    }
}
