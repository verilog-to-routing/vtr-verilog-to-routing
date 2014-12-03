#include "ParallelDynamicCilkTimingAnalyzer.hpp"
#include "TimingGraph.hpp"

#define MEMORY_ORDERING std::memory_order_seq_cst

#define DYNAMIC_ALL_LEVELS

ta_runtime ParallelDynamicCilkTimingAnalyzer::calculate_timing(TimingGraph& timing_graph) {
    //Create synchronization counters and locks
    create_synchronization(timing_graph);
    
    return SerialTimingAnalyzer::calculate_timing(timing_graph);
}

void ParallelDynamicCilkTimingAnalyzer::create_synchronization(TimingGraph& tg) {
    node_arrival_inputs_ready_count_ = std::vector<std::atomic<int>>(tg.num_nodes());
    node_required_outputs_ready_count_ = std::vector<std::atomic<int>>(tg.num_nodes());
}

void ParallelDynamicCilkTimingAnalyzer::pre_traversal(TimingGraph& timing_graph) {
    /*
     * The pre-traversal sets up the timing graph for propagating arrival
     * and required times.
     * Steps performed include:
     *   - Initialize arrival times on primary inputs
     *   - Propogating clock delay to all clock pins
     *   - Initialize required times on primary outputs
     */
    //We perform a BFS from primary inputs to initialize the timing graph
    const std::vector<int>& primary_inputs = timing_graph.level(0);
    cilk_for(int i = 0; i < (int) primary_inputs.size(); i++) {
        SerialTimingAnalyzer::pre_traverse_node(timing_graph, primary_inputs[i]);
    }

    const std::vector<int>& primary_outputs = timing_graph.primary_outputs();
    cilk_for(int i = 0; i < (int) primary_outputs.size(); i++) {
        SerialTimingAnalyzer::pre_traverse_node(timing_graph, primary_outputs[i]);
    }
}

void ParallelDynamicCilkTimingAnalyzer::forward_traversal(TimingGraph& timing_graph) {
    //Forward traversal (arrival times)
    //
    //We perform a 'dynamic' breadth-first like search
    //
    //The first level was initialized by the pre-traversal, so we can directly
    //launch each node on level 1 as a task in the traversal.
    //
    //These tasks will then dynamically launch a task for any downstream node is 'ready' (i.e. has its upstream nodes fully updated)
    const std::vector<int>& level_ids = timing_graph.level(1);
    cilk_for(int i = 0; i < (int) level_ids.size(); i++) {
        forward_traverse_node(timing_graph, level_ids[i]);
    }
}
    
void ParallelDynamicCilkTimingAnalyzer::backward_traversal(TimingGraph& timing_graph) {
    //Backward traversal (required times)
    //
    //We perform a 'dynamic' reverse breadth-first like search
    //
    //The primary outputs were initialized by the pre-traversal, so we can directly
    //lauch a task from each primary output
    //
    //These tasks will then dynamically launch a task for any upstream node is 'ready' (i.e. has its downstream nodes fully updated)
    const std::vector<NodeId>& primary_outputs = timing_graph.primary_outputs();
    cilk_for(int i = 0; i < (int) primary_outputs.size(); i++) {
        backward_traverse_node(timing_graph, primary_outputs[i]);
    }
}

void ParallelDynamicCilkTimingAnalyzer::forward_traverse_node(TimingGraph& tg, NodeId node_id) {
    //From upstream sources to current node
    SerialTimingAnalyzer::forward_traverse_node(tg, node_id);

    //Update ready counts of downstream nodes, and launch any that are up to date
    for(int edge_idx = 0; edge_idx < tg.num_node_out_edges(node_id); edge_idx++) {
        EdgeId edge_id = tg.node_out_edge(node_id, edge_idx);

        NodeId sink_node_id = tg.edge_sink_node(edge_id);

        //Atomiclly update ready count
        //  We use atomic CAS to perform the update.
        //  This allows us to know if *we* were the ones 
        //  who did the final update.
        std::atomic<int>& input_ready_count = node_arrival_inputs_ready_count_[sink_node_id];
        int prev_input_ready_count = input_ready_count.load(MEMORY_ORDERING);
        while(!input_ready_count.compare_exchange_weak(prev_input_ready_count, prev_input_ready_count+1)) {
            prev_input_ready_count = input_ready_count.load(MEMORY_ORDERING);
        }

        //If we reached here we successfully updated the ready count
        //If the previous value was one less than the number of edges 
        //We know we were the ones to finally increment it to equal the number of edges
        if(prev_input_ready_count == tg.num_node_in_edges(sink_node_id) - 1) {
#ifdef DYNAMIC_ALL_LEVELS
            cilk_spawn forward_traverse_node(tg, sink_node_id);
#else
            forward_traverse_node(tg, sink_node_id);
#endif
        }
    }
}

void ParallelDynamicCilkTimingAnalyzer::backward_traverse_node(TimingGraph& tg, NodeId node_id) {
    //From downstream sinks to current node
    SerialTimingAnalyzer::backward_traverse_node(tg, node_id);

    //Update ready counts of upstream nodes, and launch any that are up to date
    for(int edge_idx = 0; edge_idx < tg.num_node_in_edges(node_id); edge_idx++) {
        EdgeId edge_id = tg.node_in_edge(node_id, edge_idx);

        int src_node_id = tg.edge_src_node(edge_id);

        //Atomiclly update ready count
        //  We use atomic CAS to perform the update.
        //  This allows us to know if *we* were the ones 
        //  who did the final update (by comparing the prev value).
        std::atomic<int>& output_ready_count = node_required_outputs_ready_count_[src_node_id];
        int prev_output_ready_count = output_ready_count.load(MEMORY_ORDERING);
        while(!output_ready_count.compare_exchange_weak(prev_output_ready_count, prev_output_ready_count+1)) {
            prev_output_ready_count = output_ready_count.load(MEMORY_ORDERING);
        }

        //If we reached here we successfully updated the ready count
        //If the previous value was one less than the number of edges 
        //we know we were the ones to finally increment it to equal the number of edges
        if(prev_output_ready_count == tg.num_node_out_edges(src_node_id) - 1) {
#ifdef DYNAMIC_ALL_LEVELS
            cilk_spawn backward_traverse_node(tg, src_node_id);
#else
            backward_traverse_node(tg, src_node_id);
#endif
        }
    }
}
