#include <cilk/cilk.h>

#include "ParallelLevelizedCilkTimingAnalyzer.hpp"
#include "TimingGraph.hpp"

void ParallelLevelizedCilkTimingAnalyzer::pre_traversal(TimingGraph& timing_graph) {
    /*
     * The pre-traversal sets up the timing graph for propagating arrival
     * and required times.
     * Steps performed include:
     *   - Initialize arrival times on primary inputs
     *   - Propogating clock delay to all clock pins
     *   - Initialize required times on primary outputs
     */
    //We perform a BFS from primary inputs to initialize the timing graph
    for(int level_idx = 0; level_idx < timing_graph.num_levels(); level_idx++) {
        const std::vector<int>& level_ids = timing_graph.level(level_idx);
        cilk_for(int i = 0; i < (int) level_ids.size(); i++) {
            SerialTimingAnalyzer::pre_traverse_node(timing_graph, level_ids[i], level_idx);
        }
    }
}

void ParallelLevelizedCilkTimingAnalyzer::forward_traversal(TimingGraph& timing_graph) {
    //Forward traversal (arrival times)

    //Need synchronization on nodes, since each source node could propogate
    //arrival times to multiple nodes.
    //We could get a race-condition if multiple threads try to update a single
    //node in parallel
    
    //Levelized Breadth-first
    //
    //The first level was initialized by the pre-traversal
    //We update each node based on its predicessors
    for(int level_idx = 1; level_idx < timing_graph.num_levels(); level_idx++) {
        const std::vector<int>& level_ids = timing_graph.level(level_idx);
        cilk_for(int i = 0; i < (int) level_ids.size(); i++) {

            SerialTimingAnalyzer::forward_traverse_node(timing_graph, level_ids[i]);
        }
    }
}
    
void ParallelLevelizedCilkTimingAnalyzer::backward_traversal(TimingGraph& timing_graph) {
    //Backward traversal (required times)
    //  Since we only store edges in the forward direction we calculate required times by
    //  setting the values for the current level based on the one below
    for(int level_idx = timing_graph.num_levels() - 2; level_idx >= 0; level_idx--) {
        const std::vector<int>& level_ids = timing_graph.level(level_idx);
        cilk_for(int i = 0; i < (int) level_ids.size(); i++) {

            SerialTimingAnalyzer::backward_traverse_node(timing_graph, level_ids[i]);
        }
    }
}
