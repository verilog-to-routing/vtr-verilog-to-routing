#pragma once

#include "tatum/Time.hpp"
#include "tatum/TimingGraphFwd.hpp"

namespace tatum {

/** 
 * Interface for a delay calculator
 */
class DelayCalculator {
    public:
        virtual ~DelayCalculator() {}

        virtual Time min_edge_delay(const TimingGraph& tg, EdgeId edge_id) const = 0;
        virtual Time max_edge_delay(const TimingGraph& tg, EdgeId edge_id) const = 0;

        virtual Time setup_time(const TimingGraph& tg, EdgeId edge_id) const = 0;
        virtual Time hold_time(const TimingGraph& tg, EdgeId edge_id) const = 0;
};

} //namepsace
