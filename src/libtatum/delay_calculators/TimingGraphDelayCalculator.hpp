#pragma once

#include "DelayCalculator.hpp"
#include "TimingGraph.hpp"

class TimingGraphDelayCalculator : public DelayCalculator {
    public:
        Time min_edge_delay(const TimingGraph& tg, EdgeId edge_id) const { return tg.edge_delay(edge_id); };
        Time max_edge_delay(const TimingGraph& tg, EdgeId edge_id) const { return tg.edge_delay(edge_id); };
};
