#pragma once

#include "SetupTimingAnalyzer.hpp"
#include "HoldTimingAnalyzer.hpp"

class SetupHoldTimingAnalyzer : public SetupTimingAnalyzer,
                                public HoldTimingAnalyzer {
    public:
        SetupHoldTimingAnalyzer(const TimingGraph& tg, const TimingConstraints& tc)
            : SetupTimingAnalyzer(tg, tc)
            , HoldTimingAnalyzer(tg, tc) {}
};
