/***************************************
 * Methods for Object RRGraphBuilderView
 ***************************************/
#include "vtr_assert.h"
#include "rr_graph_builder_view.h"

/****************************
 * Constructors
 ****************************/
RRGraphBuilderView::RRGraphBuilderView() {
    node_storage_ = nullptr;
    rr_node_indices_ = nullptr;
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
        (*rr_node_indices_)[type].resize({size_t(x), size_t(y), size_t(side)});
    }

    if (size_t(ptc) >= (*rr_node_indices_)[type][x][y][side].size()) {
        (*rr_node_indices_)[type][x][y][side].resize(ptc);
    }

    /* Resize on demand finished; Register the node */
    (*rr_node_indices_)[type][x][y][side][ptc] = int(size_t(node));
}
