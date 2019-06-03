#ifndef VPR_ROUTING_PREDICTOR_H
#define VPR_ROUTING_PREDICTOR_H
#include <vector>
#include <cstddef>

//When the estimated number of routing iterations exceeds these factors
//(for SAFE or AGGRESSIVE mode respectively) times the max router iterations
//specified by the router aborts early
constexpr float ROUTING_PREDICTOR_ITERATION_ABORT_FACTOR_SAFE = 3;
constexpr float ROUTING_PREDICTOR_ITERATION_ABORT_FACTOR_AGGRESSIVE = 1.5;

//If the number of overused resources is below this threshold do not abort.
// This avoids giving up when solutions are nearly legal, but converging slowly
constexpr size_t ROUTING_PREDICTOR_MIN_ABSOLUTE_OVERUSE_THRESHOLD = 100;

class RoutingPredictor {
  public:
    RoutingPredictor(size_t min_history = 8, float history_factor = 0.5);

    //Returns the estimated iteration when routing will succeed
    float estimate_success_iteration();

    //Returns the current estimated slope (RR nodes per iteration)
    float estimate_overuse_slope();

    void add_iteration_overuse(size_t iteration, size_t overused_rr_node_count);

    float get_slope();

  private:
    size_t min_history_;
    float history_factor_;

    std::vector<size_t> iterations_;
    std::vector<size_t> iteration_overused_rr_node_counts_;
};

#endif
