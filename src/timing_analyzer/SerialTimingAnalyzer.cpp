#include "SerialTimingAnalyzer.hpp"
#include "TimingGraph.hpp"

void SerialTimingAnalyzer::update_timing(TimingGraph& timing_graph) {
    //Initialize fist level for arrival time traversal
    for(int node_id : timing_graph.primary_inputs()) {
        TimingNode& node = timing_graph.node(node_id);

        node.set_arrival_time(Time(0));
    }
    
    //Forward traversal (arrival times)
    for(int level_idx = 0; level_idx < timing_graph.num_levels(); level_idx++) {
        for(int node_id : timing_graph.level(level_idx)) {
            const TimingNode& from_node = timing_graph.node(node_id);
            float from_arrival_time = from_node.arrival_time().value();

            for(int edge_idx = 0; edge_idx < from_node.num_out_edges(); edge_idx++) {
                const TimingEdge& edge = from_node.out_edge(edge_idx);

                int to_node_id = edge.to_node();

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
    
    //Initialize last level required times
    for(int node_id : timing_graph.primary_outputs()) {
        TimingNode& node = timing_graph.node(node_id);

        //TODO: use a real value, not just a default
        node.set_required_time(Time(1e-9));
    }

    //Backward traversal (required times)
    //  Since we only store edges in the forward direction we calculate required times by
    //  setting the values for the current level based on the one below
    for(int level_idx = timing_graph.num_levels() - 2; level_idx > 0; level_idx--) {
        for(int node_id : timing_graph.level(level_idx)) {
            TimingNode& node = timing_graph.node(node_id);
            float required_time = node.required_time().value();

            for(int edge_idx = 0; edge_idx < node.num_out_edges(); edge_idx++) {
                const TimingEdge& edge = node.out_edge(edge_idx);

                int to_node_id = edge.to_node();
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
