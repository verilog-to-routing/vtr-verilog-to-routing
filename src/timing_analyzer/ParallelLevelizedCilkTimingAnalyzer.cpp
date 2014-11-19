#include <cilk/cilk.h>

#include "ParallelLevelizedCilkTimingAnalyzer.hpp"
#include "TimingGraph.hpp"


#define PRE_TRAVERSAL_GRAIN_SIZE 1024
#define MAIN_TRAVERSAL_GRAIN_SIZE 1024 //2*16384

#define DEFAULT_CLOCK_PERIOD 1.0e-9

void ParallelLevelizedCilkTimingAnalyzer::calculate_timing(TimingGraph& timing_graph) {
    pre_traversal(timing_graph);

    //TODO: While forward and backward tranversals work on the same edges
    //and nodes, they work on independant data sets (i.e. arrival time for forward and required time for backward)
    //so they can occur in parallel
    forward_traversal(timing_graph);
    backward_traversal(timing_graph);
}

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
//#pragma cilk grainsize=PRE_TRAVERSAL_GRAIN_SIZE
        cilk_for(int i = 0; i < (int) level_ids.size(); i++) {
            pre_traverse_node(timing_graph, level_ids[i], level_idx);
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
//#pragma cilk grainsize=MAIN_TRAVERSAL_GRAIN_SIZE
        cilk_for(int i = 0; i < (int) level_ids.size(); i++) {

            forward_traverse_node(timing_graph, level_ids[i]);
        }
    }
}
    
void ParallelLevelizedCilkTimingAnalyzer::backward_traversal(TimingGraph& timing_graph) {
    //Backward traversal (required times)
    //  Since we only store edges in the forward direction we calculate required times by
    //  setting the values for the current level based on the one below
    for(int level_idx = timing_graph.num_levels() - 2; level_idx >= 0; level_idx--) {
        const std::vector<int>& level_ids = timing_graph.level(level_idx);
//#pragma cilk grainsize=MAIN_TRAVERSAL_GRAIN_SIZE
        cilk_for(int i = 0; i < (int) level_ids.size(); i++) {

            backward_traverse_node(timing_graph, level_ids[i]);
        }
    }
}

void ParallelLevelizedCilkTimingAnalyzer::pre_traverse_node(TimingGraph& tg, NodeId node_id, int level_idx) {
    TimingNode& node = tg.node(node_id);
    if(level_idx == 0) { //Primary Input
        //Initialize with zero arrival time
        node.set_arrival_time(Time(0));
    }

    if(node.num_out_edges() == 0) { //Primary Output

        //Initialize required time
        //FIXME Currently assuming:
        //   * A single clock
        //   * At fixed frequency
        //   * Non-propogated (i.e. no clock delay/skew)
        node.set_required_time(Time(DEFAULT_CLOCK_PERIOD));
    }
}

void ParallelLevelizedCilkTimingAnalyzer::forward_traverse_node(TimingGraph& tg, NodeId node_id) {
    //The from nodes were updated by the previous level update, so they are only accessed
    //read only.
    //Since only a single thread updates to_node, we don't need any locks
    TimingNode& to_node = tg.node(node_id);

    float new_arrival_time = NAN;
    for(int edge_idx = 0; edge_idx < to_node.num_in_edges(); edge_idx++) {
        EdgeId edge_id = to_node.in_edge_id(edge_idx);
        const TimingEdge& edge = tg.edge(edge_id);
        float edge_delay = edge.delay().value();

        int from_node_id = edge.from_node_id();

        const TimingNode& from_node = tg.node(from_node_id);

        //TODO: Generalize this to support a more general Time class (e.g. vector of timing corners)
        float from_arrival_time = from_node.arrival_time().value();

        if(edge_idx == 0) {
            new_arrival_time = from_arrival_time + edge_delay;
        } else {
            new_arrival_time = std::max(new_arrival_time, from_arrival_time + edge_delay);
        }

    }
    to_node.set_arrival_time(Time(new_arrival_time));
}

void ParallelLevelizedCilkTimingAnalyzer::backward_traverse_node(TimingGraph& tg, NodeId node_id) {
    //Only one thread ever updates node so we don't need to synchronize access to it
    //
    //The downstream (to_node) was updated on the previous level (i.e. is read-only), 
    //so we don't need to worry about synchronizing access to it.
    TimingNode& node = tg.node(node_id);
    float required_time = node.required_time().value();

    for(int edge_idx = 0; edge_idx < node.num_out_edges(); edge_idx++) {
        EdgeId edge_id = node.out_edge_id(edge_idx);
        const TimingEdge& edge = tg.edge(edge_id);

        int to_node_id = edge.to_node_id();
        const TimingNode& to_node = tg.node(to_node_id);

        //TODO: Generalize this to support a general Time class (e.g. vector of corners or statistical etc.)
        float to_required_time = to_node.required_time().value();
        float edge_delay = edge.delay().value();

        if(isnan(required_time)) {
            required_time = to_required_time - edge_delay;
        } else {
            required_time = std::min(required_time, to_required_time - edge_delay);
        }

    }
    node.set_required_time(Time(required_time));
}
