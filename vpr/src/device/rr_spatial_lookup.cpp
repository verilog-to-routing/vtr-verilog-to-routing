#include "vtr_assert.h"
#include "rr_spatial_lookup.h"

RRSpatialLookup::RRSpatialLookup(t_rr_node_indices& rr_node_indices)
    : rr_node_indices_(rr_node_indices) {
}

RRNodeId RRSpatialLookup::find_node(int x,
                                    int y,
                                    t_rr_type type,
                                    int ptc,
                                    e_side side) const {
    /* Find actual side to be used */
    e_side node_side = side;
    if (type == IPIN || type == OPIN) {
        VTR_ASSERT_MSG(side != NUM_SIDES, "IPIN/OPIN must specify desired side (can not be default NUM_SIDES)");
    } else {
        VTR_ASSERT(type != IPIN && type != OPIN);
        node_side = SIDES[0];
    }

    /* Currently need to swap x and y for CHANX because of chan, seg convention 
     * This is due to that the fast look-up builders uses (y, x) coordinate when
     * registering a CHANX node in the look-up
     * TODO: Once the builders is reworked for use consistent (x, y) convention,
     * the following swapping can be removed
     */
    size_t node_x = x;
    size_t node_y = y;
    if (CHANX == type) {
        std::swap(node_x, node_y);
    }

    VTR_ASSERT_SAFE(3 == rr_node_indices_[type].ndims());

    /* Sanity check to ensure the x, y, side and ptc are in range 
     * - Return an valid id by searching in look-up when all the parameters are in range
     * - Return an invalid id if any out-of-range is detected
     */
    if (size_t(type) >= rr_node_indices_.size()) {
        return RRNodeId::INVALID();
    }

    if (node_x >= rr_node_indices_[type].dim_size(0)) {
        return RRNodeId::INVALID();
    }

    if (node_y >= rr_node_indices_[type].dim_size(1)) {
        return RRNodeId::INVALID();
    }

    if (node_side >= rr_node_indices_[type].dim_size(2)) {
        return RRNodeId::INVALID();
    }

    if (size_t(ptc) >= rr_node_indices_[type][node_x][node_y][node_side].size()) {
        return RRNodeId::INVALID();
    }

    return RRNodeId(rr_node_indices_[type][node_x][node_y][node_side][ptc]);
}

void RRSpatialLookup::add_node(RRNodeId node,
                               int x,
                               int y,
                               t_rr_type type,
                               int ptc,
                               e_side side) {
    VTR_ASSERT_SAFE(3 == rr_node_indices_[type].ndims());

    /* Expand the fast look-up if the new node is out-of-range
     * This may seldom happen because the rr_graph building function
     * should ensure the fast look-up well organized  
     */
    VTR_ASSERT(type < rr_node_indices_.size());
    VTR_ASSERT(0 <= x);
    VTR_ASSERT(0 <= y);

    if ((x >= int(rr_node_indices_[type].dim_size(0)))
        || (y >= int(rr_node_indices_[type].dim_size(1)))
        || (size_t(side) >= rr_node_indices_[type].dim_size(2))) {
        rr_node_indices_[type].resize({std::max(rr_node_indices_[type].dim_size(0), size_t(x) + 1),
                                       std::max(rr_node_indices_[type].dim_size(1), size_t(y) + 1),
                                       std::max(rr_node_indices_[type].dim_size(2), size_t(side) + 1)});
    }

    if (size_t(ptc) >= rr_node_indices_[type][x][y][side].size()) {
        rr_node_indices_[type][x][y][side].resize(ptc + 1);
    }

    /* Resize on demand finished; Register the node */
    rr_node_indices_[type][x][y][side][ptc] = int(size_t(node));
}
