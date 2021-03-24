#include "rr_graph_overlay.h"

/****************************
 * Constructors
 ****************************/
RRGraphOverlay::RRGraphOverlay() {
    node_storage_ = nullptr;
    rr_node_indices_ = nullptr;
}

/****************************
 * Accessors
 ****************************/
t_rr_type RRGraphOverlay::node_type(const RRNodeId& id) const {
    return node_storage_->node_type(id);
}

/****************************
 * Mutators
 ****************************/
void RRGraphOverlay::set_internal_data(t_rr_graph_storage* node_storage,
                                       t_rr_node_indices* rr_node_indices) {
    node_storage_ = node_storage;
    rr_node_indices_ = rr_node_indices;
}
