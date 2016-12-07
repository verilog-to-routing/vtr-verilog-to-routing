#include "validate_timing_graph_constraints.hpp"
#include "TimingGraph.hpp"
#include "TimingConstraints.hpp"

#include "tatum_error.hpp"
#include "loop_detect.hpp"

namespace tatum {

bool validate_timing_graph_constraints(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints) {

    auto edge_range = timing_constraints.disabled_timing_edges();
    std::set<EdgeId> disabled_edges(edge_range.begin(), edge_range.end());

    //Currently we only check if there are any active (i.e. not disabled) combinational loops
    auto sccs = identify_combinational_loops(timing_graph, disabled_edges);

    if(!sccs.empty()) {
        throw Error("Timing graph contains active combinational loops. "
                    "Either disable timing edges with constraints (to break the loops) or restructure the graph."); 

        //Future work: 
        //
        //  We could handle this internally by identifying the incoming and outgoing edges of the SCC,
        //  and estimating a 'max' delay through the SCC from each incoming to each outgoing edge.
        //  The SCC could then be replaced with a clique between SCC input and output edges.
        //
        //  One possible estimate is to trace the longest path through the SCC without visiting a node 
        //  more than once (although this is not gaurenteed to be conservative). 
    }
    return true;
}

} //namespace
