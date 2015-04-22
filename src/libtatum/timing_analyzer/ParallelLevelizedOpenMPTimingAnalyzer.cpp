#include "ParallelLevelizedOpenMPTimingAnalyzer.hpp"
#include "TimingGraph.hpp"

#include <omp.h>

void ParallelLevelizedOpenMPTimingAnalyzer::pre_traversal(TimingGraph& timing_graph) {
    /*
     * The pre-traversal sets up the timing graph for propagating arrival
     * and required times.
     * Steps performed include:
     *   - Initialize arrival times on primary inputs
     *   - Propogating clock delay to all clock pins
     *   - Initialize required times on primary outputs
     */
    //We perform a BFS from primary inputs to initialize the timing graph
#pragma omp for
    for(NodeId node_id = 0; node_id < timing_graph.num_nodes(); node_id++) {
        pre_traverse_node(timing_graph, node_id);
    }
}

void ParallelLevelizedOpenMPTimingAnalyzer::forward_traversal(TimingGraph& timing_graph) {
    //Forward traversal (arrival times)
    for(int level_idx = 1; level_idx < timing_graph.num_levels(); level_idx++) {
        const std::vector<int>& level_ids = timing_graph.level(level_idx);
#pragma omp for
        for(int i = 0; i < (int) level_ids.size(); i++) {

            forward_traverse_node(timing_graph, level_ids[i]);
        }
    }
}
    
void ParallelLevelizedOpenMPTimingAnalyzer::backward_traversal(TimingGraph& timing_graph) {
    //Backward traversal (required times)
    for(int level_idx = timing_graph.num_levels() - 2; level_idx >= 0; level_idx--) {
        const std::vector<int>& level_ids = timing_graph.level(level_idx);
#pragma omp for
        for(int i = 0; i < (int) level_ids.size(); i++) {

            backward_traverse_node(timing_graph, level_ids[i]);
        }
    }
}
