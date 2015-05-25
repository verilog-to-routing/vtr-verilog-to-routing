#pragma once

#include "TimingAnalyzer.hpp"

class SetupTimingAnalyzer : public TimingAnalyzer {
    public:
        SetupTimingAnalyzer(const TimingGraph& tg, const TimingConstraints& tc)
            : TimingAnalyzer(tg, tc) {}
        virtual const TimingTags& setup_data_tags(const NodeId node_id) const = 0;
        virtual const TimingTags& setup_clock_tags(const NodeId node_id) const = 0;
};
