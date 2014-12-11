#include <cassert>
#include <map>

#include "TimingGraph.hpp"



NodeId TimingGraph::add_node(const TimingNode& new_node) {
    //Type
    TN_Type new_node_type = new_node.type();
    node_types_.push_back(new_node.type());

    //Save primary outputs as we build the graph
    if(new_node_type == TN_Type::OUTPAD_SINK ||
       new_node_type == TN_Type::FF_SINK) {
        primary_outputs_.push_back(node_types_.size() - 1);
    }

    //Out edges
    std::vector<EdgeId> out_edges = std::vector<EdgeId>();
    out_edges.reserve(new_node.num_out_edges());
    for(int i = 0; i < new_node.num_out_edges(); i++) {
        EdgeId edge_id = new_node.out_edge_id(i);
        out_edges.emplace_back(edge_id);
    }
    node_out_edges_.push_back(std::move(out_edges));
    
    //In edges
    //  these get filled in later, after all nodes have been added
    std::vector<EdgeId> in_edges = std::vector<EdgeId>();
    node_in_edges_.push_back(std::move(in_edges));

    //Arrival/Required Times
    node_arr_times_.push_back(new_node.arrival_time());
    node_req_times_.push_back(new_node.required_time());
    
    //Verify sizes
    assert(node_types_.size() == node_out_edges_.size());
    assert(node_types_.size() == node_in_edges_.size());
    assert(node_types_.size() == node_arr_times_.size());
    assert(node_types_.size() == node_req_times_.size());

    //Return the ID of the added node
    return node_types_.size() - 1;
}

EdgeId TimingGraph::add_edge(const TimingEdge& new_edge) {
    edge_sink_nodes_.push_back(new_edge.to_node_id());
    edge_src_nodes_.push_back(new_edge.from_node_id());
    edge_delays_.push_back(new_edge.delay());

    assert(edge_sink_nodes_.size() == edge_src_nodes_.size());
    assert(edge_sink_nodes_.size() == edge_delays_.size());

    //Return the edge id of the added edge
    return edge_delays_.size() - 1;
}

void TimingGraph::fill_back_edges() {
    for(int i = 0; i < num_nodes(); i++) {
        for(EdgeId edge_id : node_out_edges_[i]) {
            NodeId sink_node = edge_sink_node(edge_id);

            //edge_id drives the sink so it is an in edge
            node_in_edges_[sink_node].push_back(edge_id);
        }
    }
}

void TimingGraph::contiguize_level_edges() {
    //Make all edges in a level be contiguous in memory
    std::cout << "Re-allocating edges so levels are in contiguous memory" << std::endl;

    //Collect levels for edges
    std::vector<std::vector<EdgeId>> edge_levels;
    for(int level_idx = 0; level_idx < num_levels(); level_idx++) {
        edge_levels.push_back(std::vector<EdgeId>());
        for(auto node_id : node_levels_[level_idx]) {
            for(int i = 0; i < num_node_out_edges(node_id); i++) {
                EdgeId edge_id = node_out_edge(node_id, i);

                edge_levels[level_idx].push_back(edge_id);
            }
        }
    }

    //Save a map from old edge_id to new edge_id, will be used to update node refs
    std::map<EdgeId,EdgeId> old_edge_to_new_edge;
    int cnt = 0;
    for(std::vector<EdgeId>& edge_level : edge_levels) {
        for(EdgeId orig_edge_id : edge_level) {
            old_edge_to_new_edge[orig_edge_id] = cnt; 
            cnt++;
        }
    }

    /*
     * Re-allocate edges to be contiguous in memory
     */

    //Save the old values while we write the new ones
    std::vector<NodeId> old_edge_sink_nodes_;
    std::vector<NodeId> old_edge_src_nodes_;
#ifdef TIME_MEM_ALIGN
    std::vector<Time, aligned_allocator<Time, TIME_MEM_ALIGN>> old_edge_delays_;
#else
    std::vector<Time> old_edge_delays_;
#endif

    //Swap them
    std::swap(old_edge_sink_nodes_, edge_sink_nodes_);
    std::swap(old_edge_src_nodes_, edge_src_nodes_);
    std::swap(old_edge_delays_, edge_delays_);

    //Update values
    for(auto& edge_level : edge_levels) {
        for(auto& orig_edge_id : edge_level) {
            //Write edges in the new contiguous order
            edge_sink_nodes_.push_back(old_edge_sink_nodes_[orig_edge_id]);
            edge_src_nodes_.push_back(old_edge_src_nodes_[orig_edge_id]);
            edge_delays_.push_back(old_edge_delays_[orig_edge_id]);
        }
    }

    /*
     * Update edge refs in nodes
     */
    for(int i = 0; i < num_nodes(); i++) {
        for(size_t j = 0; j < node_out_edges_[i].size(); j++) {
            EdgeId old_edge_id = node_out_edges_[i][j];
            node_out_edges_[i][j] = old_edge_to_new_edge[old_edge_id];
        }
        for(size_t j = 0; j < node_in_edges_[i].size(); j++) {
            EdgeId old_edge_id = node_in_edges_[i][j];
            node_in_edges_[i][j] = old_edge_to_new_edge[old_edge_id];
        }
    }
}

std::map<NodeId,NodeId> TimingGraph::contiguize_level_nodes() {
    //Make all nodes in a level be contiguous in memory
    std::cout << "Re-allocating nodes so levels are in contiguous memory" << std::endl;

    /*
     * Build a map of the old and new node ids to update edges
     * and node levels later
     */
    std::map<NodeId,NodeId> old_node_to_new_node;
    int cnt = 0;
    for(int level_idx = 0; level_idx < num_levels(); level_idx++) {
        for(NodeId node_id : node_levels_[level_idx]) {
            old_node_to_new_node[node_id] = cnt;
            cnt++;
        }
    }


    /*
     * Re-allocate nodes so levels are in contiguous memory
     */
    std::vector<TN_Type> old_node_types;
    std::vector<std::vector<EdgeId>> old_node_out_edges;
    std::vector<std::vector<EdgeId>> old_node_in_edges;
#ifdef TIME_MEM_ALIGN
    std::vector<Time, aligned_allocator<Time, TIME_MEM_ALIGN>> old_node_arr_times;
    std::vector<Time, aligned_allocator<Time, TIME_MEM_ALIGN>> old_node_req_times;
#else
    std::vector<Time> old_node_arr_times;
    std::vector<Time> old_node_req_times;
#endif

    //Swap the values
    std::swap(old_node_types, node_types_);
    std::swap(old_node_out_edges, node_out_edges_);
    std::swap(old_node_in_edges, node_in_edges_);
    std::swap(old_node_arr_times, node_arr_times_);
    std::swap(old_node_req_times, node_req_times_);

    //Update the values
    for(int level_idx = 0; level_idx < num_levels(); level_idx++) {
        for(NodeId old_node_id : node_levels_[level_idx]) {
            node_types_.push_back(old_node_types[old_node_id]);
            node_out_edges_.push_back(old_node_out_edges[old_node_id]);
            node_in_edges_.push_back(old_node_in_edges[old_node_id]);
            node_arr_times_.push_back(old_node_arr_times[old_node_id]);
            node_req_times_.push_back(old_node_req_times[old_node_id]);
        }
    }

    /*
     * Update old references to node_ids with thier new values
     */
    //The node levels
    for(int level_idx = 0; level_idx < num_levels(); level_idx++) {
        for(size_t i = 0; i < node_levels_[level_idx].size(); i++) {
            NodeId old_node_id = node_levels_[level_idx][i];
            node_levels_[level_idx][i] = old_node_to_new_node[old_node_id];
        }
    }

    //The primary outputs
    for(size_t i = 0; i < primary_outputs_.size(); i++) {
        NodeId old_node_id = primary_outputs_[i];
        primary_outputs_[i] = old_node_to_new_node[old_node_id];
    }

    //The Edges
    for(int i = 0; i < num_edges(); i++) {
        NodeId old_sink_node = edge_sink_nodes_[i];
        NodeId old_src_node = edge_src_nodes_[i];

        edge_sink_nodes_[i] = old_node_to_new_node[old_sink_node];
        edge_src_nodes_[i] = old_node_to_new_node[old_src_node];
    }

    return old_node_to_new_node;
}
