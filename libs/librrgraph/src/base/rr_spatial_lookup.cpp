#include "rr_graph_fwd.h"
#include "vtr_assert.h"
#include "rr_spatial_lookup.h"
#include <set>

RRNodeId RRSpatialLookup::find_node(int layer,
                                    int x,
                                    int y,
                                    e_rr_type type,
                                    int ptc,
                                    e_side side) const {
    /* Find actual side to be used
     * - For the node which are input/outputs of a grid, there must be a specific side for them.
     *   Because they should have a specific pin location on the perimeter of a grid.
     * - For other types of nodes, there is no need to define a side. However, a default value
     *   is needed when store the node in the fast look-up data structure.
     *   Here we just arbitrary use the first side of the SIDE vector as the default value.
     *   We may consider to use NUM_2D_SIDES as the default value but it will cause an increase
     *   in the dimension of the fast look-up data structure.
     *   Please note that in the add_node function, we should keep the SAME convention!
     */
    e_side node_side = side;
    if (type == e_rr_type::IPIN || type == e_rr_type::OPIN) {
        VTR_ASSERT_MSG(side != NUM_2D_SIDES, "IPIN/OPIN must specify desired side (can not be default NUM_2D_SIDES)");
    } else {
        VTR_ASSERT_SAFE(type != e_rr_type::IPIN && type != e_rr_type::OPIN);
        node_side = TOTAL_2D_SIDES[0];
    }

    /* Pre-check: the layer, x, y, side and ptc should be non-negative numbers! Otherwise, return an invalid id */
    if ((layer < 0) || (x < 0) || (y < 0) || (node_side == NUM_2D_SIDES) || (ptc < 0)) {
        return RRNodeId::INVALID();
    }

    VTR_ASSERT_SAFE(4 == rr_node_indices_[type].ndims());

    /* Sanity check to ensure the layer, x, y, side and ptc are in range
     * - Return an valid id by searching in look-up when all the parameters are in range
     * - Return an invalid id if any out-of-range is detected
     */
    if (size_t(type) >= rr_node_indices_.size()) {
        return RRNodeId::INVALID();
    }

    if (size_t(layer) >= rr_node_indices_[type].dim_size(0)) {
        return RRNodeId::INVALID();
    }

    if (size_t(x) >= rr_node_indices_[type].dim_size(1)) {
        return RRNodeId::INVALID();
    }

    if (size_t(y) >= rr_node_indices_[type].dim_size(2)){
        return RRNodeId::INVALID();
    }

    if (node_side >= rr_node_indices_[type].dim_size(3)) {
        return RRNodeId::INVALID();
    }

    if (size_t(ptc) >= rr_node_indices_[type][layer][x][y][node_side].size()) {
        return RRNodeId::INVALID();
    }

    return rr_node_indices_[type][layer][x][y][node_side][ptc];
}

std::vector<RRNodeId> RRSpatialLookup::find_nodes_in_range(int layer,
                                                           int xlow,
                                                           int ylow,
                                                           int xhigh,
                                                           int yhigh,
                                                           e_rr_type type,
                                                           int ptc,
                                                           e_side side) const {
    std::set<RRNodeId> nodes;
    for (int x = xlow; x <= xhigh; ++x) {
        for (int y = ylow; y <= yhigh; ++y) {
            RRNodeId node = find_node(layer, x, y, type, ptc, side);

            if (node != RRNodeId::INVALID())
                nodes.insert(node);
        }
    }

    return std::vector<RRNodeId>(nodes.begin(), nodes.end());
}

std::vector<RRNodeId> RRSpatialLookup::find_nodes(int layer,
                                                  int x,
                                                  int y,
                                                  e_rr_type type,
                                                  e_side side) const {
    /* TODO: The implementation of this API should be worked 
     * when rr_node_indices adapts RRNodeId natively!
     */
    std::vector<RRNodeId> nodes;

    /* Pre-check: the layer, x, y are valid! Otherwise, return an empty vector */
    if (layer < 0 || x < 0 || y < 0) {
        return nodes;
    }

    VTR_ASSERT_SAFE(4 == rr_node_indices_[type].ndims());

    /* Sanity check to ensure the x, y, side are in range 
     * - Return a list of valid ids by searching in look-up when all the parameters are in range
     * - Return an empty list if any out-of-range is detected
     */
    if (size_t(type) >= rr_node_indices_.size()) {
        return nodes;
    }

    if (size_t(layer) >= rr_node_indices_[type].dim_size(0)) {
        return nodes;
    }

    if (size_t(x) >= rr_node_indices_[type].dim_size(1)) {
        return nodes;
    }

    if (size_t(y) >= rr_node_indices_[type].dim_size(2)){
        return nodes;
    }

    if (side >= rr_node_indices_[type].dim_size(3)) {
        return nodes;
    }

    /* Reserve space to avoid memory fragmentation */
    size_t num_nodes = 0;
    for (RRNodeId node : rr_node_indices_[type][layer][x][y][side]) {
        if (node.is_valid()) {
            num_nodes++;
        }
    }

    nodes.reserve(num_nodes);
    for (RRNodeId node : rr_node_indices_[type][layer][x][y][side]) {
        if (node.is_valid()) {
            nodes.emplace_back(node);
        }
    }

    return nodes;
}

std::vector<RRNodeId> RRSpatialLookup::find_channel_nodes(int layer,
                                                          int x,
                                                          int y,
                                                          e_rr_type type) const {
    // Pre-check: node type should be routing tracks.
    if (type != e_rr_type::CHANX && type != e_rr_type::CHANY && type != e_rr_type::CHANZ) {
        return {};
    }

    return find_nodes(layer, x, y, type);
}

std::vector<RRNodeId> RRSpatialLookup::find_nodes_at_all_sides(int layer,
                                                               int x,
                                                               int y,
                                                               e_rr_type rr_type,
                                                               int ptc) const {
    std::vector<RRNodeId> indices;

    /* TODO: Consider to access the raw data like find_node() rather than calling find_node() many times, which hurts runtime */
    if (rr_type == e_rr_type::IPIN || rr_type == e_rr_type::OPIN) {
        indices.reserve(NUM_2D_SIDES);
        // For pins, we need to look at all the sides of the current grid tile
        for (e_side side : TOTAL_2D_SIDES) {
            RRNodeId rr_node_index = find_node(layer, x, y, rr_type, ptc, side);
            if (rr_node_index) {
                indices.push_back(rr_node_index);
            }
        }
        indices.shrink_to_fit();
    } else {
        // Sides do not affect non-pins so there should only be one per ptc
        RRNodeId rr_node_index = find_node(layer, x, y, rr_type, ptc);
        if (rr_node_index) {
            indices.push_back(rr_node_index);
        }
    }

    return indices;
}

std::vector<RRNodeId> RRSpatialLookup::find_grid_nodes_at_all_sides(int layer,
                                                                    int x,
                                                                    int y,
                                                                    e_rr_type rr_type) const {
    VTR_ASSERT(rr_type == e_rr_type::SOURCE || rr_type == e_rr_type::OPIN || rr_type == e_rr_type::IPIN || rr_type == e_rr_type::SINK || rr_type == e_rr_type::MUX);
    if (rr_type == e_rr_type::SOURCE || rr_type == e_rr_type::SINK || rr_type == e_rr_type::MUX) {
        return find_nodes(layer,x, y, rr_type);
    }

    std::vector<RRNodeId> nodes;
    // Reserve space to avoid memory fragmentation
    size_t num_nodes = 0;
    for (e_side node_side : TOTAL_2D_SIDES) {
        num_nodes += find_nodes(layer, x, y, rr_type, node_side).size();
    }

    nodes.reserve(num_nodes);
    for (e_side node_side : TOTAL_2D_SIDES) {
        std::vector<RRNodeId> temp_nodes = find_nodes(layer,x, y, rr_type, node_side);
        nodes.insert(nodes.end(), temp_nodes.begin(), temp_nodes.end());
    }
    return nodes;
}

void RRSpatialLookup::reserve_nodes(int layer,
                                    int x,
                                    int y,
                                    e_rr_type type,
                                    int num_nodes,
                                    e_side side) {
    VTR_ASSERT_SAFE(4 == rr_node_indices_[type].ndims());

    /* For non-IPIN/OPIN nodes, the side should always be the TOP side which follows the convention in find_node() API! */
    if (type != e_rr_type::IPIN && type != e_rr_type::OPIN) {
        VTR_ASSERT(side == TOTAL_2D_SIDES[0]);
    }

    resize_nodes(layer, x, y, type, side);

    rr_node_indices_[type][layer][x][y][side].reserve(num_nodes);
}

void RRSpatialLookup::add_node(RRNodeId node,
                               int layer,
                               int x,
                               int y,
                               e_rr_type type,
                               int ptc,
                               e_side side) {
    VTR_ASSERT(node.is_valid()); /* Must have a valid node id to be added */
    VTR_ASSERT_SAFE(4 == rr_node_indices_[type].ndims());

    /* For non-IPIN/OPIN nodes, the side should always be the TOP side which follows the convention in find_node() API! */
    if (type != e_rr_type::IPIN && type != e_rr_type::OPIN) {
        VTR_ASSERT(side == TOTAL_2D_SIDES[0]);
    }

    resize_nodes(layer, x, y, type, side);

    if (size_t(ptc) >= rr_node_indices_[type][layer][x][y][side].size()) {
        /* Deposit invalid ids to newly allocated elements while original elements are untouched */
        rr_node_indices_[type][layer][x][y][side].resize(ptc + 1, RRNodeId::INVALID());
    }

    /* Resize on demand finished; Register the node */
    rr_node_indices_[type][layer][x][y][side][ptc] = node;
}

bool RRSpatialLookup::remove_node(RRNodeId node,
                                  int layer,
                                  int x,
                                  int y,
                                  e_rr_type type,
                                  int ptc,
                                  e_side side) {
    VTR_ASSERT(node.is_valid());
    VTR_ASSERT_SAFE(4 == rr_node_indices_[type].ndims());
    VTR_ASSERT_SAFE(layer >= 0);
    VTR_ASSERT_SAFE(x >= 0);
    VTR_ASSERT_SAFE(y >= 0);
    VTR_ASSERT_SAFE(type != e_rr_type::NUM_RR_TYPES);
    VTR_ASSERT_SAFE(ptc >= 0);
    VTR_ASSERT_SAFE(side != NUM_2D_SIDES);

    // Check if the node given is in the spatial lookup at the given indices
    if ((size_t)type >= rr_node_indices_.size()) return false;
    if ((size_t)layer >= rr_node_indices_[type].dim_size(0)) return false;
    if ((size_t)x >= rr_node_indices_[type].dim_size(1)) return false;
    if ((size_t)y >= rr_node_indices_[type].dim_size(2)) return false;
    if (side >= rr_node_indices_[type].dim_size(3)) return false;
    if ((size_t)ptc >= rr_node_indices_[type][layer][x][y][side].size()) return false;
    if (rr_node_indices_[type][layer][x][y][side][ptc] != node) return false;

    // The node was in the spatial lookup; remove it. -1 corresponds to an invalid node id,
    // and so is treated as absent in the spatial lookup
    rr_node_indices_[type][layer][x][y][side][ptc] = RRNodeId::INVALID();
    return true;
}

void RRSpatialLookup::mirror_nodes(const int layer,
                                   const vtr::Point<int>& src_coord,
                                   const vtr::Point<int>& des_coord,
                                   e_rr_type type,
                                   e_side side) {
    VTR_ASSERT(e_rr_type::SOURCE == type || e_rr_type::SINK == type);
    resize_nodes(layer, des_coord.x(), des_coord.y(), type, side);
    rr_node_indices_[type][layer][des_coord.x()][des_coord.y()][side] = rr_node_indices_[type][layer][src_coord.x()][src_coord.y()][side];
}

void RRSpatialLookup::resize_nodes(int layer,
                                   int x,
                                   int y,
                                   e_rr_type type,
                                   e_side side) {
    /* Expand the fast look-up if the new node is out-of-range
     * This may seldom happen because the rr_graph building function
     * should ensure the fast look-up well organized  
     */
    VTR_ASSERT((size_t)type < rr_node_indices_.size());
    VTR_ASSERT(x >= 0);
    VTR_ASSERT(y >= 0);
    VTR_ASSERT(layer >= 0);

    if ((layer >= int(rr_node_indices_[type].dim_size(0)))
        || (x >= int(rr_node_indices_[type].dim_size(1)))
        || (y >= int(rr_node_indices_[type].dim_size(2)))
        || (size_t(side) >= rr_node_indices_[type].dim_size(3))) {
        rr_node_indices_[type].resize({std::max(rr_node_indices_[type].dim_size(0), size_t(layer)+1),
                                       std::max(rr_node_indices_[type].dim_size(1), size_t(x) + 1),
                                       std::max(rr_node_indices_[type].dim_size(2), size_t(y) + 1),
                                       std::max(rr_node_indices_[type].dim_size(3), size_t(side) + 1)});
    }
}

void RRSpatialLookup::reorder(const vtr::vector<RRNodeId, RRNodeId>& dest_order) {
    // update rr_node_indices, a map to optimize rr_index lookups
    for (auto& grid : rr_node_indices_) {
        for(size_t l = 0; l < grid.dim_size(0); l++) {
            for (size_t x = 0; x < grid.dim_size(1); x++) {
                for (size_t y = 0; y < grid.dim_size(2); y++) {
                    for (size_t s = 0; s < grid.dim_size(3); s++) {
                        for (RRNodeId &node: grid[l][x][y][s]) {
                            if (node.is_valid()) {
                                node = dest_order[node];
                            }
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
