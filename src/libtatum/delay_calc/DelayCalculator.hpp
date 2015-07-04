#pragma once
#include "timing_graph_fwd.hpp"
class Time;

class DelayCalculator {
    public:
        Time min_edge_delay(const TimingGraph& tg, EdgeId edge_id) const;
        Time max_edge_delay(const TimingGraph& tg, EdgeId edge_id) const;
};
