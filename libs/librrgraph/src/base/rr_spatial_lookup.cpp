#include "vtr_assert.h"
#include "rr_spatial_lookup.h"

RRSpatialLookup::RRSpatialLookup() {
}

RRNodeId RRSpatialLookup::find_node(int x,
                                    int y,
                                    t_rr_type type,
                                    int ptc,
                                    e_side side) const {
    /* Find actual side to be used
     * - For the node which are input/outputs of a grid, there must be a specific side for them.
     *   Because they should have a specific pin location on the perimeter of a grid.
     * - For other types of nodes, there is no need to define a side. However, a default value
     *   is needed when store the node in the fast look-up data structure.
     *   Here we just arbitrary use the first side of the SIDE vector as the default value.
     *   We may consider to use NUM_SIDES as the default value but it will cause an increase
     *   in the dimension of the fast look-up data structure.
     *   Please note that in the add_node function, we should keep the SAME convention!
     */
    e_side node_side = side;
    if (type == IPIN || type == OPIN) {
        VTR_ASSERT_MSG(side != NUM_SIDES, "IPIN/OPIN must specify desired side (can not be default NUM_SIDES)");
    } else {
        VTR_ASSERT_SAFE(type != IPIN && type != OPIN);
        node_side = SIDES[0];
    }

    /* Pre-check: the x, y, side and ptc should be non negative numbers! Otherwise, return an invalid id */
    if ((x < 0) || (y < 0) || (node_side == NUM_SIDES) || (ptc < 0)) {
        return RRNodeId::INVALID();
    }

    /* Currently need to swap x and y for CHANX because of chan, seg convention 
     * This is due to that the fast look-up builders uses (y, x) coordinate when
     * registering a CHANX node in the look-up
     * TODO: Once the builders is reworked for use consistent (x, y) convention,
     * the following swapping can be removed
     */
    size_t node_x = x;
    size_t node_y = y;
    if (type == CHANX) {
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

std::vector<RRNodeId> RRSpatialLookup::find_nodes(int x,
                                                  int y,
                                                  t_rr_type type,
                                                  e_side side) const {
    /* TODO: The implementation of this API should be worked 
     * when rr_node_indices adapts RRNodeId natively!
     */
    std::vector<RRNodeId> nodes;

    /* Pre-check: the x, y, type are valid! Otherwise, return an empty vector */
    if (x < 0 || y < 0) {
        return nodes;
    }

    /* Currently need to swap x and y for CHANX because of chan, seg convention 
     * This is due to that the fast look-up builders uses (y, x) coordinate when
     * registering a CHANX node in the look-up
     * TODO: Once the builders is reworked for use consistent (x, y) convention,
     * the following swapping can be removed
     */
    size_t node_x = x;
    size_t node_y = y;
    if (type == CHANX) {
        std::swap(node_x, node_y);
    }

    VTR_ASSERT_SAFE(3 == rr_node_indices_[type].ndims());

    /* Sanity check to ensure the x, y, side are in range 
     * - Return a list of valid ids by searching in look-up when all the parameters are in range
     * - Return an empty list if any out-of-range is detected
     */
    if (size_t(type) >= rr_node_indices_.size()) {
        return nodes;
    }

    if (node_x >= rr_node_indices_[type].dim_size(0)) {
        return nodes;
    }

    if (node_y >= rr_node_indices_[type].dim_size(1)) {
        return nodes;
    }

    if (side >= rr_node_indices_[type].dim_size(2)) {
        return nodes;
    }

    /* Reserve space to avoid memory fragmentation */
    size_t num_nodes = 0;
    for (const auto& node : rr_node_indices_[type][node_x][node_y][side]) {
        if (RRNodeId(node)) {
            num_nodes++;
        }
    }

    nodes.reserve(num_nodes);
    for (const auto& node : rr_node_indices_[type][node_x][node_y][side]) {
        if (RRNodeId(node)) {
            nodes.push_back(RRNodeId(node));
        }
    }

    return nodes;
}

std::vector<RRNodeId> RRSpatialLookup::find_channel_nodes(int x,
                                                          int y,
                                                          t_rr_type type) const {
    /* Pre-check: node type should be routing tracks! */
    if (type != CHANX && type != CHANY) {
        return std::vector<RRNodeId>();
    }

    return find_nodes(x, y, type);
}

std::vector<RRNodeId> RRSpatialLookup::find_nodes_at_all_sides(int x,
                                                               int y,
                                                               t_rr_type rr_type,
                                                               int ptc) const {
    std::vector<RRNodeId> indices;

    /* TODO: Consider to access the raw data like find_node() rather than calling find_node() many times, which hurts runtime */
    if (rr_type == IPIN || rr_type == OPIN) {
        indices.reserve(NUM_SIDES);
        //For pins we need to look at all the sides of the current grid tile
        for (e_side side : SIDES) {
            RRNodeId rr_node_index = find_node(x, y, rr_type, ptc, side);
            if (rr_node_index) {
                indices.push_back(rr_node_index);
            }
        }
        indices.shrink_to_fit();
    } else {
        //Sides do not effect non-pins so there should only be one per ptc
        RRNodeId rr_node_index = find_node(x, y, rr_type, ptc);
        if (rr_node_index) {
            indices.push_back(rr_node_index);
        }
    }

    return indices;
}

std::vector<RRNodeId> RRSpatialLookup::find_grid_nodes_at_all_sides(int x,
                                                                    int y,
                                                                    t_rr_type rr_type) const {
    VTR_ASSERT(rr_type == SOURCE || rr_type == OPIN || rr_type == IPIN || rr_type == SINK);
    if (rr_type == SOURCE || rr_type == SINK) {
        return find_nodes(x, y, rr_type);
    }

    std::vector<RRNodeId> nodes;
    /* Reserve space to avoid memory fragmentation */
    size_t num_nodes = 0;
    for (e_side node_side : SIDES) {
        num_nodes += find_nodes(x, y, rr_type, node_side).size();
    }

    nodes.reserve(num_nodes);
    for (e_side node_side : SIDES) {
        std::vector<RRNodeId> temp_nodes = find_nodes(x, y, rr_type, node_side);
        nodes.insert(nodes.end(), temp_nodes.begin(), temp_nodes.end());
    }
    return nodes;
}

void RRSpatialLookup::reserve_nodes(int x,
                                    int y,
                                    t_rr_type type,
                                    int num_nodes,
                                    e_side side) {
    VTR_ASSERT_SAFE(3 == rr_node_indices_[type].ndims());

    /* For non-IPIN/OPIN nodes, the side should always be the TOP side which follows the convention in find_node() API! */
    if (type != IPIN && type != OPIN) {
        VTR_ASSERT(side == SIDES[0]);
    }

    resize_nodes(x, y, type, side);

    rr_node_indices_[type][x][y][side].reserve(num_nodes);
}

void RRSpatialLookup::add_node(RRNodeId node,
                               int x,
                               int y,
                               t_rr_type type,
                               int ptc,
                               e_side side) {
    VTR_ASSERT(node); /* Must have a valid node id to be added */
    VTR_ASSERT_SAFE(3 == rr_node_indices_[type].ndims());

    /* For non-IPIN/OPIN nodes, the side should always be the TOP side which follows the convention in find_node() API! */
    if (type != IPIN && type != OPIN) {
        VTR_ASSERT(side == SIDES[0]);
    }

    resize_nodes(x, y, type, side);

    if (size_t(ptc) >= rr_node_indices_[type][x][y][side].size()) {
        /* Deposit invalid ids to newly allocated elements while original elements are untouched */
        rr_node_indices_[type][x][y][side].resize(ptc + 1, int(size_t(RRNodeId::INVALID())));
    }

    /* Resize on demand finished; Register the node */
    rr_node_indices_[type][x][y][side][ptc] = int(size_t(node));
}

void RRSpatialLookup::mirror_nodes(const vtr::Point<int>& src_coord,
                                   const vtr::Point<int>& des_coord,
                                   t_rr_type type,
                                   e_side side) {
    VTR_ASSERT(SOURCE == type || SINK == type);
    resize_nodes(des_coord.x(), des_coord.y(), type, side);
    rr_node_indices_[type][des_coord.x()][des_coord.y()][side] = rr_node_indices_[type][src_coord.x()][src_coord.y()][side];
}

void RRSpatialLookup::resize_nodes(int x,
                                   int y,
                                   t_rr_type type,
                                   e_side side) {
    /* Expand the fast look-up if the new node is out-of-range
     * This may seldom happen because the rr_graph building function
     * should ensure the fast look-up well organized  
     */
    VTR_ASSERT(type < rr_node_indices_.size());
    VTR_ASSERT(x >= 0);
    VTR_ASSERT(y >= 0);

    if ((x >= int(rr_node_indices_[type].dim_size(0)))
        || (y >= int(rr_node_indices_[type].dim_size(1)))
        || (size_t(side) >= rr_node_indices_[type].dim_size(2))) {
        rr_node_indices_[type].resize({std::max(rr_node_indices_[type].dim_size(0), size_t(x) + 1),
                                       std::max(rr_node_indices_[type].dim_size(1), size_t(y) + 1),
                                       std::max(rr_node_indices_[type].dim_size(2), size_t(side) + 1)});
    }
}

void RRSpatialLookup::reorder(const vtr::vector<RRNodeId, RRNodeId> dest_order) {
    // update rr_node_indices, a map to optimize rr_index lookups
    for (auto& grid : rr_node_indices_) {
        for (size_t x = 0; x < grid.dim_size(0); x++) {
            for (size_t y = 0; y < grid.dim_size(1); y++) {
                for (size_t s = 0; s < grid.dim_size(2); s++) {
                    for (auto& node : grid[x][y][s]) {
                        if (node != OPEN) {
                            node = size_t(dest_order[RRNodeId(node)]);
                        }
                    }
                }
            }
        }
    }
}

void RRSpatialLookup::clear() {
    for (auto& data : rr_node_indices_) {
        data.clear();
    }
}
