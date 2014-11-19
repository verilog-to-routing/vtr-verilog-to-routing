#include "ParallelDynamicCilkTimingAnalyzer.hpp"
#include "TimingGraph.hpp"

#define DEFAULT_CLOCK_PERIOD 1.0e-9

#define MEMORY_ORDERING std::memory_order_seq_cst

void ParallelDynamicCilkTimingAnalyzer::calculate_timing(TimingGraph& timing_graph) {
    //Create synchronization counters and locks
    create_synchronization(timing_graph);
    
    {
        //Initialize the timing graph
        pre_traversal(timing_graph);
    }

    {
        //Traverse the timing graph
        cilk_spawn forward_traversal(timing_graph);
        backward_traversal(timing_graph);
    }
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
    const std::vector<int>& level_ids = timing_graph.level(0);
    cilk_for(int i = 0; i < (int) level_ids.size(); i++) {
        TimingNode& node = timing_graph.node(level_ids[i]);
        node.set_arrival_time(Time(0));
    }

    cilk_for(NodeId node_id = 0; node_id < (int) timing_graph.num_nodes(); node_id++) {
        pre_traverse_node(timing_graph, node_id);
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
        NodeId node_id = level_ids[i];
        forward_traverse_node(timing_graph, node_id);
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

void ParallelDynamicCilkTimingAnalyzer::pre_traverse_node(TimingGraph& tg, NodeId node_id) {
    TimingNode& node = tg.node(node_id);
    if(node.arrival_time().value() == 0) {
        return;
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

void ParallelDynamicCilkTimingAnalyzer::forward_traverse_node(TimingGraph& tg, NodeId node_id) {
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

    //Update ready counts of downstream nodes, and launch any that are up to date
    for(int edge_idx = 0; edge_idx < to_node.num_out_edges(); edge_idx++) {
        EdgeId edge_id = to_node.out_edge_id(edge_idx);
        const TimingEdge& edge = tg.edge(edge_id);

        int downstream_node_id = edge.to_node_id();

        const TimingNode& downstream_node = tg.node(downstream_node_id);

        //Atomiclly update ready count
        //  We use atomic CAS to perform the update.
        //  This allows us to know if *we* were the ones 
        //  who did the final update.
        std::atomic<int>& input_ready_count = node_arrival_inputs_ready_count_[downstream_node_id];
        int prev_input_ready_count = input_ready_count.load(MEMORY_ORDERING);
        while(!input_ready_count.compare_exchange_weak(prev_input_ready_count, prev_input_ready_count+1)) {
            prev_input_ready_count = input_ready_count.load(MEMORY_ORDERING);
        }

        //If we reached here we successfully updated the ready count
        //If the previous value was one less than the number of edges 
        //We know we were the ones to finally increment it to equal the number of edges
        if(prev_input_ready_count == downstream_node.num_in_edges() - 1) {
            forward_traverse_node(tg, downstream_node_id);
        }
    }
}

void ParallelDynamicCilkTimingAnalyzer::backward_traverse_node(TimingGraph& tg, NodeId node_id) {
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

    //Update ready counts of upstream nodes, and launch any that are up to date
    for(int edge_idx = 0; edge_idx < node.num_in_edges(); edge_idx++) {
        EdgeId edge_id = node.in_edge_id(edge_idx);
        const TimingEdge& edge = tg.edge(edge_id);

        int upstream_node_id = edge.from_node_id();

        const TimingNode& upstream_node = tg.node(upstream_node_id);

        //Atomiclly update ready count
        //  We use atomic CAS to perform the update.
        //  This allows us to know if *we* were the ones 
        //  who did the final update (by comparing the prev value).
        std::atomic<int>& output_ready_count = node_required_outputs_ready_count_[upstream_node_id];
        int prev_output_ready_count = output_ready_count.load(MEMORY_ORDERING);
        while(!output_ready_count.compare_exchange_weak(prev_output_ready_count, prev_output_ready_count+1)) {
            prev_output_ready_count = output_ready_count.load(MEMORY_ORDERING);
        }

        //If we reached here we successfully updated the ready count
        //If the previous value was one less than the number of edges 
        //we know we were the ones to finally increment it to equal the number of edges
        if(prev_output_ready_count == upstream_node.num_out_edges() - 1) {
            backward_traverse_node(tg, upstream_node_id);
        }
    }
}
