#include <cilk/cilk.h>
#include <cilk/reducer_max.h>

#include "ParallelNoDependancyCilkTimingAnalyzer.hpp"
#include "TimingGraph.hpp"

void ParallelNoDependancyCilkTimingAnalyzer::pre_traversal(TimingGraph& timing_graph) {
    /*
     * The pre-traversal sets up the timing graph for propagating arrival
     * and required times.
     * Steps performed include:
     *   - Initialize arrival times on primary inputs
     *   - Propogating clock delay to all clock pins
     *   - Initialize required times on primary outputs
     */
    //We perform a BFS from primary inputs to initialize the timing graph
    //cilk_for(NodeId i = 0; i < timing_graph.num_nodes(); i++) {
    const std::vector<NodeId>& primary_inputs = timing_graph.level(0);
    cilk_for(size_t i = 0; i < primary_inputs.size(); i++) {
        SerialTimingAnalyzer::pre_traverse_node(timing_graph, primary_inputs[i]);
    }

    const std::vector<NodeId>& primary_outputs = timing_graph.primary_outputs();
    cilk_for(size_t i = 0; i < primary_outputs.size(); i++) {
        SerialTimingAnalyzer::pre_traverse_node(timing_graph, primary_outputs[i]);
    }
}

void ParallelNoDependancyCilkTimingAnalyzer::forward_traversal(TimingGraph& timing_graph) {
    //Forward traversal (arrival times)

    cilk_for(NodeId i = 0; i < timing_graph.num_nodes(); i++) {
        SerialTimingAnalyzer::forward_traverse_node(timing_graph, i);
    }
}
    
void ParallelNoDependancyCilkTimingAnalyzer::backward_traversal(TimingGraph& timing_graph) {
    //Backward traversal (required times)
    //  Since we only store edges in the forward direction we calculate required times by
    //  setting the values for the current level based on the one below
    cilk_for(NodeId i = 0; i < timing_graph.num_nodes(); i++) {
        SerialTimingAnalyzer::backward_traverse_node(timing_graph, i);
    }
}
