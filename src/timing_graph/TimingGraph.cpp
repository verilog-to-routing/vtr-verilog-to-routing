#include <cassert>

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
