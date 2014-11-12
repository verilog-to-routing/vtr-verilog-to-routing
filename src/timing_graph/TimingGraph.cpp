#include "TimingGraph.hpp"


void TimingGraph::add_node(const TimingNode& new_node) {
    nodes_.push_back(new_node);

    TN_Type node_type = new_node.type();

    //Save primary outputs as we build the graph
    if(node_type == TN_Type::OUTPAD_SINK ||
       node_type == TN_Type::FF_SINK) {
        primary_outputs_.push_back(nodes_.size() - 1);
    }
       
}
