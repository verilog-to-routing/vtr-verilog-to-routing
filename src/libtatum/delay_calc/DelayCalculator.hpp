#pragma once
#include "timing_graph_fwd.hpp"
namespace tatum {

class Time;

/**
 * Defines the interface to a delay calculator.
 *
 * A TimingAnalyzer will call the delay calculator to determine delays accross
 * edges in the timing graph. The analyzer can ask for either the minimum,
 * or maximum edge delay (these may be different if derating is applied).
 *
 * The timing analyzers accept an arbitrary user-defined type for the delay 
 * calculator, however it must support this interface.
 */
class DelayCalculator {
    public:
        ///Determine the minimum delay accross the specified edge
        ///\param tg The timing graph
        ///\param edge_id The ID of the edge
        Time min_edge_delay(const TimingGraph& tg, EdgeId edge_id) const;

        ///Determine the maximum delay accross the specified edge
        ///\param tg The timing graph
        ///\param edge_id The ID of the edge
        Time max_edge_delay(const TimingGraph& tg, EdgeId edge_id) const;
};

} //namepsace
