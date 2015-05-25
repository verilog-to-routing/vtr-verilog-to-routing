#pragma once

#include "TimingAnalyzer.hpp"

class HoldTimingAnalyzer : public TimingAnalyzer {
    public:
        HoldTimingAnalyzer(const TimingGraph& tg, const TimingConstraints& tc)
            : TimingAnalyzer(tg, tc) {}
        virtual const TimingTags& hold_data_tags(const NodeId node_id) const = 0;
        virtual const TimingTags& hold_clock_tags(const NodeId node_id) const = 0;
};
