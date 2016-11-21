#pragma once
#include "timing_graph_fwd.hpp"

namespace tatum {

class Time;

/**
 * Defines the delay calculator concept.
 *
 * A TimingAnalyzer will call the delay calculator to determine delays accross
 * edges in the timing graph. The analyzer can ask for either the minimum,
 * or maximum edge delay (these may be different if derating is applied).
 *
 * The timing analyzers accept an arbitrary user-defined type for the delay 
 * calculator, however it must support this interface.
 *
 * For examples:
 * \see ConstantDelayCalculator
 * \see FixedDelayCalculator
 */
class DelayCalculator {
    public:
        ///\param tg The timing graph
        ///\param edge_id The ID of the edge
        ///\returns The minimum delay accross the specified edge
        Time min_edge_delay(const TimingGraph& tg, EdgeId edge_id) const;

        ///\param tg The timing graph
        ///\param edge_id The ID of the edge
        ///\returns The maximum delay accross the specified edge
        Time max_edge_delay(const TimingGraph& tg, EdgeId edge_id) const;
};

} //namepsace
