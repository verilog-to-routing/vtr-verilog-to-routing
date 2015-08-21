#pragma once

#include "DelayCalculator.hpp"
#include "TimingGraph.hpp"

/** A DelayCalculator implementation which takes a vector
 *  of pre-calculated edge delays
 */
class PreCalcDelayCalculator {
    public:

        ///Initializes the edge delays
        ///\param edge_delays A vector specifying the delay for every edge
        PreCalcDelayCalculator(std::vector<float> edge_delays) {
            edge_delays_.reserve(edge_delays.size());
            for(float delay : edge_delays) {
                edge_delays_.emplace_back(delay);
            }
        }

        Time min_edge_delay(const TimingGraph& tg, EdgeId edge_id) const { ASSERT(edge_id < (int) edge_delays_.size()); return edge_delays_[edge_id]; };
        Time max_edge_delay(const TimingGraph& tg, EdgeId edge_id) const { ASSERT(edge_id < (int) edge_delays_.size()); return edge_delays_[edge_id]; };
    private:
        std::vector<Time> edge_delays_;
};
