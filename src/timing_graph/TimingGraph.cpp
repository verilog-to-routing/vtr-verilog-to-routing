#include "TimingGraph.hpp"


NodeId TimingGraph::add_node(const TimingNode& new_node) {
    nodes_.push_back(new_node);

    TN_Type node_type = new_node.type();

    //Save primary outputs as we build the graph
    if(node_type == TN_Type::OUTPAD_SINK ||
       node_type == TN_Type::FF_SINK) {
        primary_outputs_.push_back(nodes_.size() - 1);
    }
       
    //Return the ID of the added node
    return nodes_.size() - 1;
}

EdgeId TimingGraph::add_edge(const TimingEdge& new_edge) {
    edges_.push_back(new_edge);

    //Return the edge id of the added edge
    return edges_.size() - 1;
}
