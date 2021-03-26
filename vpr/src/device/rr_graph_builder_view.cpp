/***************************************
 * Methods for Object RRGraphBuilderView
 ***************************************/
#include "vtr_assert.h"
#include "rr_graph_builder_view.h"

/****************************
 * Constructors
 ****************************/
RRGraphBuilderView::RRGraphBuilderView(t_rr_graph_storage* node_storage,
                                       t_rr_node_indices* rr_node_indices) {
    node_storage_ = node_storage;
    rr_node_indices_ = rr_node_indices;
}

/****************************
 * Mutators
 ****************************/
void RRGraphBuilderView::add_node_to_fast_lookup(const RRNodeId& node,
                                                 const int& x,
                                                 const int& y,
                                                 const t_rr_type& type,
                                                 const int& ptc,
                                                 const e_side& side) {
    VTR_ASSERT_SAFE(3 == (*rr_node_indices_)[type].ndims());

    /* Expand the fast look-up if the new node is out-of-range
     * This may seldom happen because the rr_graph building function
     * should ensure the fast look-up well organized  
     */
    VTR_ASSERT(type < (*rr_node_indices_).size());

    if ((size_t(x) >= (*rr_node_indices_)[type].dim_size(0))
        || (size_t(y) >= (*rr_node_indices_)[type].dim_size(1))
        || (size_t(side) >= (*rr_node_indices_)[type].dim_size(2))) {
        (*rr_node_indices_)[type].resize({std::max((*rr_node_indices_)[type].dim_size(0), size_t(x) + 1),
                                          std::max((*rr_node_indices_)[type].dim_size(1), size_t(y) + 1),
                                          std::max((*rr_node_indices_)[type].dim_size(2), size_t(side) + 1)});
    }

    if (size_t(ptc) >= (*rr_node_indices_)[type][x][y][side].size()) {
        (*rr_node_indices_)[type][x][y][side].resize(ptc + 1);
    }

    /* Resize on demand finished; Register the node */
    (*rr_node_indices_)[type][x][y][side][ptc] = int(size_t(node));
}
