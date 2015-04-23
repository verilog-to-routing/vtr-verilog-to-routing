#include "cilk_safe.hpp"

#include "ParallelLevelizedCilkTimingAnalyzer.hpp"
#include "TimingGraph.hpp"
#include "sta_util.hpp"

#define NODE_STEP 1
//#define PARALLEL_MIN_FWD_LEVEL 150
//#define PARALLEL_MIN_BCK_LEVEL 2500

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
    cilk_for(NodeId node_id = 0; node_id < timing_graph.num_nodes(); node_id += NODE_STEP) {
        for(int i = 0; i < NODE_STEP && node_id + i < timing_graph.num_nodes(); i++) {
            pre_traverse_node(timing_graph, node_id + i);
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
#ifdef SAVE_LEVEL_TIMES
        clock_gettime(CLOCK_MONOTONIC, &fwd_start_[level_idx]);
#endif
#ifdef PARALLEL_MIN_FWD_LEVEL
        if(level_ids.size() >= PARALLEL_MIN_FWD_LEVEL) {
            cilk_for(int i = 0; i < (int) level_ids.size(); i += NODE_STEP) {

                for(int j = 0; j < NODE_STEP && i + j < (int) level_ids.size(); j++) {
                    SerialTimingAnalyzer::forward_traverse_node(timing_graph, level_ids[i + j]);
                }
            }
        } else {
            for(int i = 0; i < (int) level_ids.size(); i += NODE_STEP) {
                SerialTimingAnalyzer::forward_traverse_node(timing_graph, level_ids[i]);
            }

        }
#else
        cilk_for(int i = 0; i < (int) level_ids.size(); i += NODE_STEP) {

            for(int j = 0; j < NODE_STEP && i + j < (int) level_ids.size(); j++) {
                SerialTimingAnalyzer::forward_traverse_node(timing_graph, level_ids[i + j]);
            }
        }
#endif
#ifdef SAVE_LEVEL_TIMES
        clock_gettime(CLOCK_MONOTONIC, &fwd_end_[level_idx]);
#endif
    }
}
    
void ParallelLevelizedCilkTimingAnalyzer::backward_traversal(TimingGraph& timing_graph) {
    //Backward traversal (required times)
    //  Since we only store edges in the forward direction we calculate required times by
    //  setting the values for the current level based on the one below
    for(int level_idx = timing_graph.num_levels() - 2; level_idx >= 0; level_idx--) {
        const std::vector<int>& level_ids = timing_graph.level(level_idx);
#ifdef SAVE_LEVEL_TIMES
        clock_gettime(CLOCK_MONOTONIC, &bck_start_[level_idx]);
#endif
#ifdef PARALLEL_MIN_FWD_LEVEL
        if(level_ids.size() >= PARALLEL_MIN_BCK_LEVEL) {
            cilk_for(int i = 0; i < (int) level_ids.size(); i += NODE_STEP) {

                for(int j = 0; j < NODE_STEP && i + j < (int) level_ids.size(); j++) {
                    SerialTimingAnalyzer::backward_traverse_node(timing_graph, level_ids[i + j]);
                }
            }
        } else {
            for(int i = 0; i < (int) level_ids.size(); i += NODE_STEP) {
                SerialTimingAnalyzer::backward_traverse_node(timing_graph, level_ids[i]);
            }
        }
#else
        cilk_for(int i = 0; i < (int) level_ids.size(); i += NODE_STEP) {

            for(int j = 0; j < NODE_STEP && i + j < (int) level_ids.size(); j++) {
                SerialTimingAnalyzer::backward_traverse_node(timing_graph, level_ids[i + j]);
            }
        }

#endif
#ifdef SAVE_LEVEL_TIMES
        clock_gettime(CLOCK_MONOTONIC, &bck_end_[level_idx]);
#endif
    }
}

void ParallelLevelizedCilkTimingAnalyzer::pre_traverse_node(TimingGraph& tg, NodeId node_id) {
    if(tg.num_node_in_edges(node_id) == 0) { //Primary Input
        //Initialize with zero arrival time
        tg.set_node_arr_time(node_id, Time(0));
    }

    if(tg.num_node_out_edges(node_id) == 0) { //Primary Output

        //Initialize required time
        //FIXME Currently assuming:
        //   * A single clock
        //   * At fixed frequency
        //   * Non-propogated (i.e. no clock delay/skew)
        tg.set_node_req_time(node_id, Time(DEFAULT_CLOCK_PERIOD));
    }

}
