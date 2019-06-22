#include <limits>
#include <numeric>
#include <cmath>
#include <algorithm>

#include "vtr_assert.h"

#include "routing_predictor.h"

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

    float get_slope() {
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
float variance(std::vector<float> values, float avg);

float covariance(std::vector<size_t> x_values, std::vector<float> y_values, float x_avg, float y_avg);
LinearModel simple_linear_regression(std::vector<size_t> x_values, std::vector<float> y_values);
LinearModel fit_model(std::vector<size_t> iterations, std::vector<size_t> overuse, float history_factor);

template<typename T>
float variance(std::vector<T> values, float avg) {
    float var = 0;
    for (float val : values) {
        var += (val - avg) * (val - avg);
    }

    return var;
}

float covariance(std::vector<size_t> x_values, std::vector<float> y_values, float x_avg, float y_avg) {
    VTR_ASSERT(x_values.size() == y_values.size());

    float cov = 0;
    for (size_t i = 0; i < x_values.size(); ++i) {
        cov += (x_values[i] - x_avg) * (y_values[i] - y_avg);
    }

    return cov;
}

float RoutingPredictor::get_slope() {
    if (iterations_.size() > min_history_) {
        auto model = fit_model(iterations_, iteration_overused_rr_node_counts_, history_factor_);
        return model.get_slope();
    }
    return -1;
}

LinearModel simple_linear_regression(std::vector<size_t> x_values, std::vector<float> y_values) {
    float y_avg = std::accumulate(y_values.begin(), y_values.end(), 0.) / y_values.size();
    float x_avg = std::accumulate(x_values.begin(), x_values.end(), 0.) / x_values.size();

    float covariance_x_y = covariance(x_values, y_values, x_avg, y_avg);
    float variance_x = variance(x_values, x_avg);

    float beta = covariance_x_y / variance_x;
    float alpha = y_avg - beta * x_avg;

    return LinearModel(beta, alpha);
}

LinearModel fit_model(std::vector<size_t> iterations, std::vector<size_t> overuse, float history_factor) {
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
    size_t start = overuse.size() - std::round(history_factor * overuse.size());
    size_t end = overuse.size();

    //Calculate the log overuse for the history we are interested in
    std::vector<float> hist_log_overuse;
    std::vector<size_t> hist_iters;
    for (size_t i = start; i < end; ++i) {
        hist_log_overuse.push_back(std::log(overuse[i]));
        hist_iters.push_back(iterations[i]);
    }

    //We fit a linear model to the log of the overuse, this keeps the model simple but
    //captures the (typically) negative-exponential behaviour of overuse
    return simple_linear_regression(hist_iters, hist_log_overuse);
}

RoutingPredictor::RoutingPredictor(size_t min_history, float history_factor)
    : min_history_(min_history)
    , history_factor_(history_factor) {
    //nop
}

float RoutingPredictor::estimate_success_iteration() {
    float success_iteration = std::numeric_limits<float>::quiet_NaN();

    if (iterations_.size() > min_history_) {
        auto model = fit_model(iterations_, iteration_overused_rr_node_counts_, history_factor_);
        success_iteration = model.find_x_for_y_value(0.);

        if (success_iteration < 0.) {
            //Iterations less than zero occurs when the slope is positive,
            //and the intercept is before the y-axis
            success_iteration = std::numeric_limits<float>::infinity();
        }
    }

    return success_iteration;
}

float RoutingPredictor::estimate_overuse_slope() {
    //We use a fixed size sliding window of history to estimate the slope
    //This makes the slope estimate more 'recent' than the values used to estimate
    //the success iteration (although at the risk of being noisier).
    constexpr float FIXED_HISTORY_SIZE = 5; //# of previous iterations to consider

    float slope = std::numeric_limits<float>::quiet_NaN();

    float history_factor = FIXED_HISTORY_SIZE / iterations_.size(); //Fixed history size

    if (iterations_.size() >= FIXED_HISTORY_SIZE) {
        auto model = fit_model(iterations_, iteration_overused_rr_node_counts_, history_factor);

        float log_curr_usage = model.find_y_for_x_value(*(--iterations_.end()));
        float log_next_usage = model.find_y_for_x_value(*(--iterations_.end()) + 1);

        float curr_usage = std::exp(log_curr_usage);
        float next_usage = std::exp(log_next_usage);

        slope = next_usage - curr_usage;
    }

    return slope;
}

void RoutingPredictor::add_iteration_overuse(size_t iteration, size_t overused_rr_node_count) {
    iterations_.push_back(iteration);
    iteration_overused_rr_node_counts_.push_back(overused_rr_node_count);
}
