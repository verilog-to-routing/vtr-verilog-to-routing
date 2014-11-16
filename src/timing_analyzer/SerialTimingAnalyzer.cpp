#include "SerialTimingAnalyzer.hpp"
#include "TimingGraph.hpp"

#define DEFAULT_CLOCK_PERIOD 1.0e-9

void SerialTimingAnalyzer::calculate_timing(TimingGraph& timing_graph) {
    pre_traversal(timing_graph);
    forward_traversal(timing_graph);
    backward_traversal(timing_graph);
}

void SerialTimingAnalyzer::reset_timing(TimingGraph& timing_graph) {
    for(int node_id = 0; node_id < timing_graph.num_nodes(); node_id++) {
        TimingNode& node = timing_graph.node(node_id);

        node.set_arrival_time(Time(NAN));
        node.set_required_time(Time(NAN));
    }
}

void SerialTimingAnalyzer::pre_traversal(TimingGraph& timing_graph) {
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
        for(int node_id : timing_graph.level(level_idx)) {
            TimingNode& node = timing_graph.node(node_id);

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
    }
}

void SerialTimingAnalyzer::forward_traversal(TimingGraph& timing_graph) {
    //Forward traversal (arrival times)
    for(int level_idx = 0; level_idx < timing_graph.num_levels(); level_idx++) {
        for(NodeId node_id : timing_graph.level(level_idx)) {
            const TimingNode& from_node = timing_graph.node(node_id);
            float from_arrival_time = from_node.arrival_time().value();

            for(int edge_idx = 0; edge_idx < from_node.num_out_edges(); edge_idx++) {
                EdgeId edge_id = from_node.out_edge_id(edge_idx);
                const TimingEdge& edge = timing_graph.edge(edge_id);

                NodeId to_node_id = edge.to_node_id();

                TimingNode& to_node = timing_graph.node(to_node_id);

                //TODO: Generalize this to support a more general Time class (e.g. vector of timing corners)
                float orig_arrival_time = to_node.arrival_time().value();
                float edge_delay = edge.delay().value();

                float new_arrival_time;
                if(isnan(orig_arrival_time)) {
                    new_arrival_time = from_arrival_time + edge_delay;
                } else {
                    new_arrival_time = std::max(orig_arrival_time, from_arrival_time + edge_delay);
                }

                to_node.set_arrival_time(Time(new_arrival_time));
            }
        }
    }
}
    
void SerialTimingAnalyzer::backward_traversal(TimingGraph& timing_graph) {
    //Backward traversal (required times)
    //  Since we only store edges in the forward direction we calculate required times by
    //  setting the values for the current level based on the one below
    for(int level_idx = timing_graph.num_levels() - 2; level_idx >= 0; level_idx--) {
        for(NodeId node_id : timing_graph.level(level_idx)) {
            TimingNode& node = timing_graph.node(node_id);
            float required_time = node.required_time().value();

            for(int edge_idx = 0; edge_idx < node.num_out_edges(); edge_idx++) {
                EdgeId edge_id = node.out_edge_id(edge_idx);
                const TimingEdge& edge = timing_graph.edge(edge_id);

                NodeId to_node_id = edge.to_node_id();
                const TimingNode& to_node = timing_graph.node(to_node_id);

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
    }
}
