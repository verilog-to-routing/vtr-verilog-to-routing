#include "ParallelDynamicOpenMPTasksTimingAnalyzer.hpp"
#include "TimingGraph.hpp"

#include <omp.h>

#define DEFAULT_CLOCK_PERIOD 1.0e-9

void ParallelDynamicOpenMPTasksTimingAnalyzer::calculate_timing(TimingGraph& timing_graph) {
    //Create synchronization counters and locks
    create_locks(timing_graph);
    node_arrival_inputs_ready_count_ = std::vector<int>(timing_graph.num_nodes());
    node_required_outputs_ready_count_ = std::vector<int>(timing_graph.num_nodes());
#pragma omp parallel
    {
        //Initialize locks
        init_locks();

        //Initialize the timing graph
        pre_traversal(timing_graph);
#pragma omp single
        {
            forward_traversal(timing_graph);
            backward_traversal(timing_graph);
        }
#pragma omp taskwait //Wait for traversals to finish

        cleanup_locks();
    }
}

void ParallelDynamicOpenMPTasksTimingAnalyzer::create_locks(TimingGraph& tg) {
    node_arrival_locks_ = std::vector<omp_lock_t>(tg.num_nodes());
    node_required_locks_ = std::vector<omp_lock_t>(tg.num_nodes());
}

void ParallelDynamicOpenMPTasksTimingAnalyzer::init_locks() {
#pragma omp for
    for(int i = 0; i < (int) node_arrival_locks_.size(); i++) {
        omp_init_lock(&node_arrival_locks_[i]);
        omp_init_lock(&node_required_locks_[i]);
    }
}

void ParallelDynamicOpenMPTasksTimingAnalyzer::cleanup_locks() {
#pragma omp for
    for(int i = 0; i < (int) node_arrival_locks_.size(); i++) {
        omp_destroy_lock(&node_arrival_locks_[i]);
        omp_destroy_lock(&node_required_locks_[i]);
    }
}

void ParallelDynamicOpenMPTasksTimingAnalyzer::pre_traversal(TimingGraph& timing_graph) {
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
#pragma omp for
        for(int i = 0; i < (int) level_ids.size(); i++) {
            pre_traverse_node(timing_graph, level_ids[i], level_idx);
        }
    }
}

void ParallelDynamicOpenMPTasksTimingAnalyzer::forward_traversal(TimingGraph& timing_graph) {
    //Forward traversal (arrival times)
    //
    //We perform a 'dynamic' breadth-first like search
    //
    //The first level was initialized by the pre-traversal, so we can directly
    //launch each node on level 1 as a task in the traversal.
    //
    //These tasks will then dynamically launch a task for any downstream node is 'ready' (i.e. has its upstream nodes fully updated)
    const std::vector<int>& level_ids = timing_graph.level(1);
    for(int i = 0; i < (int) level_ids.size(); i++) {
        NodeId node_id = level_ids[i];
#pragma omp task shared(timing_graph)
        forward_traverse_node(timing_graph, node_id);
    }
}
    
void ParallelDynamicOpenMPTasksTimingAnalyzer::backward_traversal(TimingGraph& timing_graph) {
    //Backward traversal (required times)
    //
    //We perform a 'dynamic' reverse breadth-first like search
    //
    //The primary outputs were initialized by the pre-traversal, so we can directly
    //lauch a task from each primary output
    //
    //These tasks will then dynamically launch a task for any upstream node is 'ready' (i.e. has its downstream nodes fully updated)
    for(NodeId node_id : timing_graph.primary_outputs()) {
#pragma omp task shared(timing_graph)
        backward_traverse_node(timing_graph, node_id);
    }
}

void ParallelDynamicOpenMPTasksTimingAnalyzer::pre_traverse_node(TimingGraph& tg, NodeId node_id, int level_idx) {
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

void ParallelDynamicOpenMPTasksTimingAnalyzer::forward_traverse_node(TimingGraph& tg, NodeId node_id) {
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

#pragma omp atomic //Needs to be atomic since multiple nodes may have edges to the same downstream node
        node_arrival_inputs_ready_count_[downstream_node_id] += 1;

        if(node_arrival_inputs_ready_count_[downstream_node_id] == downstream_node.num_in_edges()) {
            //We could have several threads pass this condition:
            //      e.g. T1 incremented count then suspends
            //           T2 incremented count to equal num_in_edges
            //           Both T1 and T2 can now trigger this condiditon
            //We only allow one to proceed and actually lauch the task using a lock.
            //Hopefully since relatively few threads will be contending for this lock.
            if(omp_test_lock(&node_arrival_locks_[downstream_node_id])) {
#pragma omp task shared(tg)
                forward_traverse_node(tg, downstream_node_id);
                //Note that we do not release the lock, so any subsequent omp_test_lock()
                //calls will fail, allowing only a single thread to launch this task.
            }
        }
    }
}

void ParallelDynamicOpenMPTasksTimingAnalyzer::backward_traverse_node(TimingGraph& tg, NodeId node_id) {
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

#pragma omp atomic //Needs to be atomic since multiple nodes may have edges to the same downstream node
        node_required_outputs_ready_count_[upstream_node_id] += 1;

        if(node_required_outputs_ready_count_[upstream_node_id] == upstream_node.num_out_edges()) {
            //We could have several threads pass this condition:
            //      e.g. T1 incremented count then suspends
            //           T2 incremented count to equal num_in_edges
            //           Both T1 and T2 can now trigger this condiditon
            //We only allow one to proceed and actually lauch the task using a lock.
            //Hopefully since relatively few threads will be contending for this lock.
            if(omp_test_lock(&node_required_locks_[upstream_node_id])) {
#pragma omp task shared(tg)
                backward_traverse_node(tg, upstream_node_id);
                //Note that we do not release the lock, so any subsequent omp_test_lock()
                //calls will fail, allowing only a single thread to launch this task.
            }
        }
    }
}
