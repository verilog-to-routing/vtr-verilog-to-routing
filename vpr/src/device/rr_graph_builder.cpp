#include "vtr_log.h"
#include "rr_graph_builder.h"

RRGraphBuilder::RRGraphBuilder(t_rr_graph_storage* node_storage)
    : node_storage_(*node_storage) {
}

t_rr_graph_storage& RRGraphBuilder::node_storage() {
    return node_storage_;
}

RRSpatialLookup& RRGraphBuilder::node_lookup() {
    return node_lookup_;
}

void RRGraphBuilder::add_node_to_all_locs(RRNodeId node) {
    t_rr_type node_type = node_storage_.node_type(node);
    short node_ptc_num = node_storage_.node_ptc_num(node);
    for (int ix = node_storage_.node_xlow(node); ix <= node_storage_.node_xhigh(node); ix++) {
        for (int iy = node_storage_.node_ylow(node); iy <= node_storage_.node_yhigh(node); iy++) {
            switch (node_type) {
                case SOURCE:
                case SINK:
                case CHANY:
                    node_lookup_.add_node(node, ix, iy, node_type, node_ptc_num, SIDES[0]);
                    break;
                case CHANX:
                    /* Currently need to swap x and y for CHANX because of chan, seg convention 
                     * TODO: Once the builders is reworked for use consistent (x, y) convention,
                     * the following swapping can be removed
                     */
                    node_lookup_.add_node(node, iy, ix, node_type, node_ptc_num, SIDES[0]);
                    break;
                case OPIN:
                case IPIN:
                    for (const e_side& side : SIDES) {
                        if (node_storage_.is_node_on_specific_side(node, side)) {
                            node_lookup_.add_node(node, ix, iy, node_type, node_ptc_num, side);
                        }
                    }
                    break;
                default:
                    VTR_LOG_ERROR("Invalid node type for node '%lu' in the routing resource graph file", size_t(node));
                    break;
            }
        }
    }
}

void RRGraphBuilder::clear() {
    node_lookup_.clear();
}
