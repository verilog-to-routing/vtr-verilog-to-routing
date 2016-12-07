#ifndef TATUM_VALIDATE_GRAPH_COSNTRAINTS
#define TATUM_VALIDATE_GRAPH_COSNTRAINTS

#include "timing_graph_fwd.hpp"
#include "timing_constraints_fwd.hpp"

namespace tatum {

///\returns true if the constraints are valid for the given timing graph, throws an exception if not
bool validate_timing_graph_constraints(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints);

}

#endif
