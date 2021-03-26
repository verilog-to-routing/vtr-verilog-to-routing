/***************************************
 * Methods for Object RRGraphView
 ***************************************/
#include "vtr_assert.h"
#include "rr_graph_view.h"

/****************************
 * Constructors
 ****************************/
RRGraphView::RRGraphView() {
    node_storage_ = nullptr;
    rr_node_indices_ = nullptr;
}

/****************************
 * Accessors
 ****************************/
t_rr_type RRGraphView::node_type(const RRNodeId& node) const {
    return node_storage_->node_type(node);
}

RRNodeId RRGraphView::find_node(const int& x,
                                const int& y,
                                const t_rr_type& type,
                                const int& ptc,
                                const e_side& side) const {
    /* Find actual side to be used */
    e_side node_side = side;
    if (type == IPIN || type == OPIN) {
        VTR_ASSERT_MSG(side != NUM_SIDES, "IPIN/OPIN must specify desired side (can not be default NUM_SIDES)");
    } else {
        VTR_ASSERT(type != IPIN && type != OPIN);
        node_side = SIDES[0];
    }

    /* Currently need to swap x and y for CHANX because of chan, seg convention */
    size_t node_x = x;
    size_t node_y = y;
    if (CHANX == type) {
        std::swap(node_x, node_y);
    }

    VTR_ASSERT_SAFE(3 == (*rr_node_indices_)[type].ndims());

    /* Sanity check to ensure the x, y, side and ptc are in range 
     * Data type of rr_node_indice:
     *   typedef std::array<vtr::NdMatrix<std::vector<int>, 3>, NUM_RR_TYPES> t_rr_node_indices;
     * Note that x, y and side are the 1st, 2nd and 3rd dimensions of the vtr::NdMatrix
     * ptc is in the std::vector<int>
     */
    if (size_t(type) >= (*rr_node_indices_).size()) {
        /* Node type is out of range, return an invalid index */
        return RRNodeId::INVALID();
    }

    if (node_x >= (*rr_node_indices_)[type].dim_size(0)) {
        /* Node x is out of range, return an invalid index */
        return RRNodeId::INVALID();
    }

    if (node_y >= (*rr_node_indices_)[type].dim_size(1)) {
        /* Node y is out of range, return an invalid index */
        return RRNodeId::INVALID();
    }

    if (node_side >= (*rr_node_indices_)[type].dim_size(2)) {
        /* Node side is out of range, return an invalid index */
        return RRNodeId::INVALID();
    }

    if (size_t(ptc) >= (*rr_node_indices_)[type][node_x][node_y][node_side].size()) {
        /* Ptc is out of range, return an invalid index */
        return RRNodeId::INVALID();
    }

    /* Reaching here, it means that node exists in the look-up, return the id */
    return RRNodeId((*rr_node_indices_)[type][node_x][node_y][node_side][ptc]);
}

/****************************
 * Mutators
 ****************************/
void RRGraphView::set_internal_data(t_rr_graph_storage* node_storage,
                                    t_rr_node_indices* rr_node_indices) {
    node_storage_ = node_storage;
    rr_node_indices_ = rr_node_indices;
}
