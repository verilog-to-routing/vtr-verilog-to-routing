#include "rr_node.h"
#include "rr_graph_storage.h"

//Returns the max 'length' over the x or y or z direction
short t_rr_node::length() const {
    return std::max({storage_->node_yhigh(id_) - storage_->node_ylow(id_),
                     storage_->node_xhigh(id_) - storage_->node_xlow(id_),
                     storage_->node_layer_high(id_) - storage_->node_layer_low(id_)});
}


