#pragma once

#include "DelayCalculator.hpp"
#include "TimingGraph.hpp"

namespace tatum {

/** 
 * An exmaple DelayCalculator implementation which takes 
 * a vector of fixed pre-calculated edge delays
 *
 * \see DelayCalculator
 */
class FixedDelayCalculator {
    public:

        ///Initializes the edge delays
        ///\param edge_delays A vector specifying the delay for every edge
        FixedDelayCalculator(const std::vector<float>& setup_edge_delays, const std::vector<float>& hold_edge_delays=std::vector<float>()) {
            setup_edge_delays_.reserve(setup_edge_delays.size());
            for(float delay : setup_edge_delays) {
                setup_edge_delays_.emplace_back(delay);
            }

            if(hold_edge_delays.empty()) {
                hold_edge_delays_ = setup_edge_delays_;
            } else {
                hold_edge_delays_.reserve(hold_edge_delays.size());
                for(float delay : hold_edge_delays) {
                    hold_edge_delays_.emplace_back(delay);
                }

            }
        }

        Time min_edge_delay(const TimingGraph& /*tg*/, EdgeId edge_id) const { 
            TATUM_ASSERT(size_t(edge_id) < hold_edge_delays_.size()); 
            return hold_edge_delays_[size_t(edge_id)]; 
        };

        Time max_edge_delay(const TimingGraph& /*tg*/, EdgeId edge_id) const { 
            TATUM_ASSERT(size_t(edge_id) < setup_edge_delays_.size()); 
            return setup_edge_delays_[size_t(edge_id)]; 
        };
    private:
        std::vector<Time> setup_edge_delays_;
        std::vector<Time> hold_edge_delays_;
};

} //namepsace
