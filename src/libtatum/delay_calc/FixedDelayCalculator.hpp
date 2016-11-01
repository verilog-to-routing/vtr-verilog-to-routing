#pragma once

#include "DelayCalculator.hpp"
#include "TimingGraph.hpp"

namespace tatum {

/** A DelayCalculator implementation which takes a vector
 *  of fixed pre-calculated edge delays
 */
class FixedDelayCalculator {
    public:

        ///Initializes the edge delays
        ///\param edge_delays A vector specifying the delay for every edge
        FixedDelayCalculator(std::vector<float> edge_delays) {
            edge_delays_.reserve(edge_delays.size());
            for(float delay : edge_delays) {
                edge_delays_.emplace_back(delay);
            }
        }

        Time min_edge_delay(const TimingGraph& /*tg*/, EdgeId edge_id) const { TATUM_ASSERT(size_t(edge_id) < edge_delays_.size()); return edge_delays_[size_t(edge_id)]; };
        Time max_edge_delay(const TimingGraph& /*tg*/, EdgeId edge_id) const { TATUM_ASSERT(size_t(edge_id) < edge_delays_.size()); return edge_delays_[size_t(edge_id)]; };
    private:
        std::vector<Time> edge_delays_;
};

} //namepsace
