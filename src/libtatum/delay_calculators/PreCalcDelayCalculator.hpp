#pragma once

#include "DelayCalculator.hpp"
#include "TimingGraph.hpp"

class PreCalcDelayCalculator {
    public:
        PreCalcDelayCalculator(std::vector<float> edge_delays) {
            edge_delays_.reserve(edge_delays.size());
            for(float delay : edge_delays) {
                edge_delays_.emplace_back(delay);
            }
        }

        Time min_edge_delay(const TimingGraph& tg, EdgeId edge_id) const { return edge_delays_[edge_id]; };
        Time max_edge_delay(const TimingGraph& tg, EdgeId edge_id) const { return edge_delays_[edge_id]; };
    private:
        std::vector<Time> edge_delays_;
};
