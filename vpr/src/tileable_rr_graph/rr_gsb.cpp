/************************************************************************
 * Member functions for class RRGSB
 ***********************************************************************/
/* Headers from vtrutil library */
#include "vtr_log.h"
#include "vtr_assert.h"

#include "openfpga_rr_graph_utils.h"
#include "openfpga_side_manager.h"

#include "rr_gsb.h"
#include "vtr_geometry.h"

/************************************************************************
 * Constructors
 ***********************************************************************/
/* Constructor for an empty object */
RRGSB::RRGSB() {
    /* Set a clean start! */
    coordinate_.set(0, 0);

    chan_node_.clear();
    chan_node_direction_.clear();
    chan_node_in_edges_.clear();
    ipin_node_in_edges_.clear();

    ipin_node_.clear();

    opin_node_.clear();
    for (size_t icb_type = 0; icb_type < 2; icb_type++) {
        for (size_t iside = 0; iside < NUM_2D_SIDES; iside++) {
            cb_opin_node_[icb_type][iside].clear();
        }
    }
}

/************************************************************************
 * Accessors
 ***********************************************************************/
/* Get the number of sides of this SB */
size_t RRGSB::get_num_sides() const {
    VTR_ASSERT(validate_num_sides());
    return chan_node_direction_.size();
}

/* Get the number of routing tracks on a side */
size_t RRGSB::get_chan_width(const e_side& side) const {
    SideManager side_manager(side);
    VTR_ASSERT(side_manager.validate());
    return chan_node_[side_manager.to_size_t()].get_chan_width();
}

/* Get the number of routing tracks on a side */
t_rr_type RRGSB::get_chan_type(const e_side& side) const {
    SideManager side_manager(side);
    VTR_ASSERT(side_manager.validate());
    return chan_node_[side_manager.to_size_t()].get_type();
}

/* Get the maximum number of routing tracks on all sides */
size_t RRGSB::get_max_chan_width() const {
    size_t max_chan_width = 0;
    for (size_t side = 0; side < get_num_sides(); ++side) {
        SideManager side_manager(side);
        max_chan_width = std::max(max_chan_width, get_chan_width(side_manager.get_side()));
    }
    return max_chan_width;
}

const RRChan& RRGSB::chan(const e_side& chan_side) const {
    return chan_node_[size_t(chan_side)];
}

/* Get the number of routing tracks of a X/Y-direction CB */
size_t RRGSB::get_cb_chan_width(const t_rr_type& cb_type) const {
    return get_chan_width(get_cb_chan_side(cb_type));
}

/* Get the sides of ipin_nodes belong to the cb */
std::vector<enum e_side> RRGSB::get_cb_ipin_sides(const t_rr_type& cb_type) const {
    VTR_ASSERT(validate_cb_type(cb_type));

    std::vector<enum e_side> ipin_sides;

    /* Make sure a clean start */
    ipin_sides.clear();

    switch (cb_type) {
        case CHANX:
            ipin_sides.push_back(TOP);
            ipin_sides.push_back(BOTTOM);
            break;
        case CHANY:
            ipin_sides.push_back(RIGHT);
            ipin_sides.push_back(LEFT);
            break;
        default:
            VTR_LOG("Invalid type of connection block!\n");
            exit(1);
    }

    return ipin_sides;
}

/* Get the sides of ipin_nodes belong to the cb */
std::vector<enum e_side> RRGSB::get_cb_opin_sides(const t_rr_type& cb_type) const {
    VTR_ASSERT(validate_cb_type(cb_type));

    std::vector<enum e_side> opin_sides;

    /* Make sure a clean start */
    opin_sides.clear();

    switch (cb_type) {
        case CHANX:
        case CHANY:
            opin_sides.push_back(TOP);
            opin_sides.push_back(RIGHT);
            opin_sides.push_back(BOTTOM);
            opin_sides.push_back(LEFT);
            break;
        default:
            VTR_LOG("Invalid type of connection block!\n");
            exit(1);
    }

    return opin_sides;
}

/* Get the direction of a rr_node at a given side and track_id */
enum PORTS RRGSB::get_chan_node_direction(const e_side& side, const size_t& track_id) const {
    SideManager side_manager(side);
    VTR_ASSERT(side_manager.validate());

    /* Ensure the side is valid in the context of this switch block */
    VTR_ASSERT(validate_side(side));

    /* Ensure the track is valid in the context of this switch block at a specific side */
    VTR_ASSERT(validate_track_id(side, track_id));

    return chan_node_direction_[side_manager.to_size_t()][track_id];
}

/* Get a list of segments used in this routing channel */
std::vector<RRSegmentId> RRGSB::get_chan_segment_ids(const e_side& side) const {
    SideManager side_manager(side);
    VTR_ASSERT(side_manager.validate());

    /* Ensure the side is valid in the context of this switch block */
    VTR_ASSERT(validate_side(side));

    return chan_node_[side_manager.to_size_t()].get_segment_ids();
}

/* Get a list of rr_nodes whose sed_id is specified */
std::vector<size_t> RRGSB::get_chan_node_ids_by_segment_ids(const e_side& side,
                                                            const RRSegmentId& seg_id) const {
    return chan_node_[size_t(side)].get_node_ids_by_segment_ids(seg_id);
}

/* get a rr_node at a given side and track_id */
RRNodeId RRGSB::get_chan_node(const e_side& side, const size_t& track_id) const {
    SideManager side_manager(side);
    VTR_ASSERT(side_manager.validate());

    /* Ensure the side is valid in the context of this switch block */
    VTR_ASSERT(validate_side(side));

    /* Ensure the track is valid in the context of this switch block at a specific side */
    VTR_ASSERT(validate_track_id(side, track_id));

    return chan_node_[side_manager.to_size_t()].get_node(track_id);
}

std::vector<RREdgeId> RRGSB::get_chan_node_in_edges(const RRGraphView& rr_graph,
                                                    const e_side& side,
                                                    const size_t& track_id) const {
    SideManager side_manager(side);
    VTR_ASSERT(side_manager.validate());

    /* Ensure the side is valid in the context of this switch block */
    VTR_ASSERT(validate_side(side));

    /* Ensure the track is valid in the context of this switch block at a specific side */
    VTR_ASSERT(validate_track_id(side, track_id));

    /* The chan node must be an output port for the GSB, we allow users to access input edges*/
    VTR_ASSERT(OUT_PORT == get_chan_node_direction(side, track_id));

    /* if sorted, we give sorted edges
     * if not sorted, we give the empty vector
     */
    if (0 == chan_node_in_edges_.size()) {
        std::vector<RREdgeId> unsorted_edges;

        for (const RREdgeId& edge : rr_graph.node_in_edges(get_chan_node(side, track_id))) {
            unsorted_edges.push_back(edge);
        }

        return unsorted_edges;
    }

    return chan_node_in_edges_[side_manager.to_size_t()][track_id];
}

std::vector<RREdgeId> RRGSB::get_ipin_node_in_edges(const RRGraphView& rr_graph,
                                                    const e_side& side,
                                                    const size_t& ipin_id) const {
    SideManager side_manager(side);
    VTR_ASSERT(side_manager.validate());

    /* Ensure the side is valid in the context of this switch block */
    VTR_ASSERT(validate_side(side));

    /* Ensure the track is valid in the context of this switch block at a specific side */
    VTR_ASSERT(validate_ipin_node_id(side, ipin_id));

    /* if sorted, we give sorted edges
     * if not sorted, we give the empty vector
     */
    if (0 == ipin_node_in_edges_.size()) {
        std::vector<RREdgeId> unsorted_edges;

        for (const RREdgeId& edge : rr_graph.node_in_edges(get_ipin_node(side, ipin_id))) {
            unsorted_edges.push_back(edge);
        }

        return unsorted_edges;
    }

    return ipin_node_in_edges_[side_manager.to_size_t()][ipin_id];
}

/* get the segment id of a channel rr_node */
RRSegmentId RRGSB::get_chan_node_segment(const e_side& side, const size_t& track_id) const {
    SideManager side_manager(side);
    VTR_ASSERT(side_manager.validate());

    /* Ensure the side is valid in the context of this switch block */
    // VTR_ASSERT(validate_side(side));

    /* Ensure the track is valid in the context of this switch block at a specific side */
    // VTR_ASSERT(validate_track_id(side, track_id));

    return chan_node_[side_manager.to_size_t()].get_node_segment(track_id);
}

/* Get the number of IPIN rr_nodes on a side */
size_t RRGSB::get_num_ipin_nodes(const e_side& side) const {
    SideManager side_manager(side);
    VTR_ASSERT(side_manager.validate());
    return ipin_node_[side_manager.to_size_t()].size();
}

/* get a opin_node at a given side and track_id */
RRNodeId RRGSB::get_ipin_node(const e_side& side, const size_t& node_id) const {
    SideManager side_manager(side);
    VTR_ASSERT(side_manager.validate());

    /* Ensure the side is valid in the context of this switch block */
    VTR_ASSERT(validate_side(side));

    /* Ensure the track is valid in the context of this switch block at a specific side */
    VTR_ASSERT(validate_ipin_node_id(side, node_id));

    return ipin_node_[side_manager.to_size_t()][node_id];
}

/* Get the number of OPIN rr_nodes on a side */
size_t RRGSB::get_num_opin_nodes(const e_side& side) const {
    SideManager side_manager(side);
    VTR_ASSERT(side_manager.validate());
    return opin_node_[side_manager.to_size_t()].size();
}

/* get a opin_node at a given side and track_id */
RRNodeId RRGSB::get_opin_node(const e_side& side, const size_t& node_id) const {
    SideManager side_manager(side);
    VTR_ASSERT(side_manager.validate());

    /* Ensure the side is valid in the context of this switch block */
    VTR_ASSERT(validate_side(side));

    /* Ensure the track is valid in the context of this switch block at a specific side */
    VTR_ASSERT(validate_opin_node_id(side, node_id));

    return opin_node_[side_manager.to_size_t()][node_id];
}

/* Get the number of OPIN rr_nodes on a side */
size_t RRGSB::get_num_cb_opin_nodes(const t_rr_type& cb_type, const e_side& side) const {
    SideManager side_manager(side);
    VTR_ASSERT(side_manager.validate());
    size_t icb_type = get_cb_opin_type_id(cb_type);
    return cb_opin_node_[icb_type][side_manager.to_size_t()].size();
}

/* get a opin_node at a given side and track_id */
RRNodeId RRGSB::get_cb_opin_node(const t_rr_type& cb_type, const e_side& side, const size_t& node_id) const {
    SideManager side_manager(side);
    VTR_ASSERT(side_manager.validate());

    /* Ensure the side is valid in the context of this switch block */
    VTR_ASSERT(validate_side(side));

    /* Ensure the track is valid in the context of this switch block at a specific side */
    VTR_ASSERT(validate_cb_opin_node_id(cb_type, side, node_id));

    size_t icb_type = get_cb_opin_type_id(cb_type);
    return cb_opin_node_[icb_type][side_manager.to_size_t()][node_id];
}

/* Get the node index of a routing track of a connection block, return -1 if not found */
int RRGSB::get_cb_chan_node_index(const t_rr_type& cb_type, const RRNodeId& node) const {
    enum e_side chan_side = get_cb_chan_side(cb_type);
    return get_chan_node_index(chan_side, node);
}

/* Get the node index in the array, return -1 if not found */
int RRGSB::get_chan_node_index(const e_side& node_side, const RRNodeId& node) const {
    VTR_ASSERT(validate_side(node_side));
    return chan_node_[size_t(node_side)].get_node_track_id(node);
}

/* Get the node index in the array, return -1 if not found */
int RRGSB::get_node_index(const RRGraphView& rr_graph,
                          const RRNodeId& node,
                          const e_side& node_side,
                          const PORTS& node_direction) const {
    size_t cnt;
    int ret;

    cnt = 0;
    ret = -1;

    /* Depending on the type of rr_node, we search different arrays */
    switch (rr_graph.node_type(node)) {
        case CHANX:
        case CHANY:
            for (size_t inode = 0; inode < get_chan_width(node_side); ++inode) {
                if ((node == chan_node_[size_t(node_side)].get_node(inode))
                    /* Check if direction meets specification */
                    && (node_direction == chan_node_direction_[size_t(node_side)][inode])) {
                    cnt++;
                    ret = inode;
                    break;
                }
            }
            break;
        case IPIN:
            for (size_t inode = 0; inode < get_num_ipin_nodes(node_side); ++inode) {
                if (node == ipin_node_[size_t(node_side)][inode]) {
                    cnt++;
                    ret = inode;
                    break;
                }
            }
            break;
        case OPIN:
            for (size_t inode = 0; inode < get_num_opin_nodes(node_side); ++inode) {
                if (node == opin_node_[size_t(node_side)][inode]) {
                    cnt++;
                    ret = inode;
                    break;
                }
            }
            break;
        default:
            VTR_LOG("Invalid cur_rr_node type! Should be [CHANX|CHANY|IPIN|OPIN]\n");
            exit(1);
    }

    VTR_ASSERT((0 == cnt) || (1 == cnt));

    return ret; /* Return an invalid value: nonthing is found*/
}

/* Get the side of a node in this SB */
void RRGSB::get_node_side_and_index(const RRGraphView& rr_graph,
                                    const RRNodeId& node,
                                    const PORTS& node_direction,
                                    e_side& node_side,
                                    int& node_index) const {
    size_t side;
    SideManager side_manager;

    /* Count the number of existence of cur_rr_node in cur_sb_info
     * It could happen that same cur_rr_node appears on different sides of a SB
     * For example, a routing track go vertically across the SB.
     * Then its corresponding rr_node appears on both TOP and BOTTOM sides of this SB.
     * We need to ensure that the found rr_node has the same direction as user want.
     * By specifying the direction of rr_node, There should be only one rr_node can satisfy!
     */
    for (side = 0; side < get_num_sides(); ++side) {
        side_manager.set_side(side);
        node_index = get_node_index(rr_graph, node, side_manager.get_side(), node_direction);
        if (-1 != node_index) {
            break;
        }
    }

    if (side == get_num_sides()) {
        /* we find nothing, return NUM_SIDES, and a OPEN node (-1) */
        node_side = NUM_2D_SIDES;
        VTR_ASSERT(-1 == node_index);
        return;
    }

    node_side = side_manager.get_side();
    VTR_ASSERT(-1 != node_index);

    return;
}

/* Check if the node exist in the opposite side of this Switch Block */
bool RRGSB::is_sb_node_exist_opposite_side(const RRGraphView& rr_graph,
                                           const RRNodeId& node,
                                           const e_side& node_side) const {
    SideManager side_manager(node_side);
    int index;

    VTR_ASSERT((CHANX == rr_graph.node_type(node)) || (CHANY == rr_graph.node_type(node)));

    /* See if we can find the same src_rr_node in the opposite chan_side
     * if there is one, it means a shorted wire across the SB
     */
    index = get_node_index(rr_graph, node, side_manager.get_opposite(), IN_PORT);

    return (-1 != index);
}

/* check if the CB exist in this GSB */
bool RRGSB::is_cb_exist(const t_rr_type& cb_type) const {
    /* if channel width is zero, there is no CB */
    return (0 != get_cb_chan_width(cb_type));
}

/* check if the SB exist in this GSB */
bool RRGSB::is_sb_exist(const RRGraphView& rr_graph) const {
    /* Count the number of sides that there are routing wires or opin nodes */
    size_t num_sides_contain_routing_wires = 0;
    size_t num_sides_contain_opin_nodes = 0;
    for (size_t side = 0; side < get_num_sides(); ++side) {
        SideManager side_manager(side);
        if (0 != get_chan_width(side_manager.get_side())) {
            num_sides_contain_routing_wires++;
        }
        if (0 != get_num_opin_nodes(side_manager.get_side())) {
            num_sides_contain_opin_nodes++;
        }
    }
    /* When there are zero nodes, the sb does not exist */
    if (num_sides_contain_routing_wires == 0 && num_sides_contain_opin_nodes == 0) {
        return false;
    }
    /* If there is only 1 side of nodes and there are 0 opin nodes, and there are no incoming edges, this is also an empty switch block */
    if (num_sides_contain_routing_wires == 1 && num_sides_contain_opin_nodes == 0) {
        size_t num_incoming_edges = 0;
        for (size_t side = 0; side < get_num_sides(); ++side) {
            SideManager side_manager(side);
            for (size_t itrack = 0; itrack < get_chan_width(side_manager.get_side()); ++itrack) {
                if (OUT_PORT != get_chan_node_direction(side_manager.get_side(), itrack)) {
                    continue;
                }
                num_incoming_edges += get_chan_node_in_edges(rr_graph, side_manager.get_side(), itrack).size();
            }
        }
        return num_incoming_edges ? true : false;
    }

    return true;
}

/************************************************************************
 * Check if the node indicates a passing wire across the Switch Block part of the GSB
 * Therefore, we actually do the following check
 * Check if a track starts from this GSB or not
 * For INC_DIRECTION
 * (xlow, ylow) should be same as the GSB side coordinate
 * For DEC_DIRECTION
 * (xhigh, yhigh) should be same as the GSB side coordinate
 ***********************************************************************/
bool RRGSB::is_sb_node_passing_wire(const RRGraphView& rr_graph,
                                    const e_side& node_side,
                                    const size_t& track_id) const {
    /* Get the rr_node */
    RRNodeId track_node = get_chan_node(node_side, track_id);
    /* Get the coordinates */
    vtr::Point<size_t> side_coordinate = get_side_block_coordinate(node_side);

    /* Get the coordinate of where the track starts */
    vtr::Point<size_t> track_start = get_track_rr_node_start_coordinate(rr_graph, track_node);

    /* INC_DIRECTION start_track: (xlow, ylow) should be same as the GSB side coordinate */
    /* DEC_DIRECTION start_track: (xhigh, yhigh) should be same as the GSB side coordinate */
    if ((track_start.x() == side_coordinate.x())
        && (track_start.y() == side_coordinate.y())
        && (OUT_PORT == get_chan_node_direction(node_side, track_id))) {
        /* Double check: start track should be an OUTPUT PORT of the GSB */
        return false; /* This is a starting point */
    }

    /* Get the coordinate of where the track ends */
    vtr::Point<size_t> track_end = get_track_rr_node_end_coordinate(rr_graph, track_node);

    /* INC_DIRECTION end_track: (xhigh, yhigh) should be same as the GSB side coordinate */
    /* DEC_DIRECTION end_track: (xlow, ylow) should be same as the GSB side coordinate */
    if ((track_end.x() == side_coordinate.x())
        && (track_end.y() == side_coordinate.y())
        && (IN_PORT == get_chan_node_direction(node_side, track_id))) {
        /* Double check: end track should be an INPUT PORT of the GSB */
        return false; /* This is an ending point */
    }

    /* Reach here it means that this will be a passing wire,
     * we should be able to find the node on the opposite side of the GSB!
     */
    if (true != is_sb_node_exist_opposite_side(rr_graph, track_node, node_side)) {
        VTR_LOG("Cannot find a node on the opposite side to GSB[%lu][%lu] track node[%lu] at %s!\nDetailed node information:\n",
                get_x(), get_y(), track_id, TOTAL_2D_SIDE_STRINGS[node_side]);
        VTR_LOG("Node type: %s\n", rr_graph.node_type_string(track_node));
        VTR_LOG("Node coordinate: %s\n", rr_graph.node_coordinate_to_string(track_node).c_str());
        VTR_LOG("Node ptc: %d\n", rr_graph.node_ptc_num(track_node));
    }
    VTR_ASSERT(true == is_sb_node_exist_opposite_side(rr_graph, track_node, node_side));

    return true;
}

/* check if the candidate SB satisfy the basic requirements on being a mirror of the current one */
/* Idenify mirror Switch blocks
 * Check each two switch blocks:
 * Number of channel/opin/ipin rr_nodes are same
 * If all above are satisfied, the two switch blocks may be mirrors !
 */
bool RRGSB::is_sb_mirrorable(const RRGraphView& rr_graph, const RRGSB& cand) const {
    /* check the numbers of sides */
    if (get_num_sides() != cand.get_num_sides()) {
        return false;
    }

    /* check the numbers/directionality of channel rr_nodes */
    for (size_t side = 0; side < get_num_sides(); ++side) {
        SideManager side_manager(side);

        /* Ensure we have the same channel width on this side */
        if (get_chan_width(side_manager.get_side()) != cand.get_chan_width(side_manager.get_side())) {
            return false;
        }

        if (((size_t(-1) == get_track_id_first_short_connection(rr_graph, side_manager.get_side()))
             && (size_t(-1) != cand.get_track_id_first_short_connection(rr_graph, side_manager.get_side())))
            || ((size_t(-1) != get_track_id_first_short_connection(rr_graph, side_manager.get_side()))
                && (size_t(-1) == cand.get_track_id_first_short_connection(rr_graph, side_manager.get_side())))) {
            return false;
        }
    }

    /* check the numbers of opin_rr_nodes */
    for (size_t side = 0; side < get_num_sides(); ++side) {
        SideManager side_manager(side);

        if (get_num_opin_nodes(side_manager.get_side()) != cand.get_num_opin_nodes(side_manager.get_side())) {
            return false;
        }
    }

    return true;
}

/* Public Accessors: Cooridinator conversion */

/* get the x coordinate of this GSB */
size_t RRGSB::get_x() const {
    return coordinate_.x();
}

size_t RRGSB::chan_node_size(const e_side& side) const {
    SideManager side_manager(side);
    return chan_node_[side_manager.get_side()].get_chan_width();
}

size_t RRGSB::ipin_node_size(const e_side& side) const {
    SideManager side_manager(side);
    return ipin_node_[side_manager.get_side()].size();
}

size_t RRGSB::opin_node_size(const e_side& side) const {
    SideManager side_manager(side);
    return opin_node_[side_manager.get_side()].size();
}

/* get the y coordinate of this GSB */
size_t RRGSB::get_y() const {
    return coordinate_.y();
}

/* get the x coordinate of this switch block */
size_t RRGSB::get_sb_x() const {
    return coordinate_.x();
}

/* get the y coordinate of this switch block */
size_t RRGSB::get_sb_y() const {
    return coordinate_.y();
}

/* Get the number of sides of this SB */
vtr::Point<size_t> RRGSB::get_sb_coordinate() const {
    return coordinate_;
}

/* get the x coordinate of this X/Y-direction block */
size_t RRGSB::get_cb_x(const t_rr_type& cb_type) const {
    return get_cb_coordinate(cb_type).x();
}

/* get the y coordinate of this X/Y-direction block */
size_t RRGSB::get_cb_y(const t_rr_type& cb_type) const {
    return get_cb_coordinate(cb_type).y();
}

/* Get the coordinate of the X/Y-direction CB */
vtr::Point<size_t> RRGSB::get_cb_coordinate(const t_rr_type& cb_type) const {
    VTR_ASSERT(validate_cb_type(cb_type));
    switch (cb_type) {
        case CHANX:
            return coordinate_;
        case CHANY:
            return coordinate_;
        default:
            VTR_LOG("Invalid type of connection block!\n");
            exit(1);
    }
}

e_side RRGSB::get_cb_chan_side(const t_rr_type& cb_type) const {
    VTR_ASSERT(validate_cb_type(cb_type));
    switch (cb_type) {
        case CHANX:
            return LEFT;
        case CHANY:
            return BOTTOM;
        default:
            VTR_LOG("Invalid type of connection block!\n");
            exit(1);
    }
}

/* Get the side of routing channel in the GSB according to the side of IPIN */
e_side RRGSB::get_cb_chan_side(const e_side& ipin_side) const {
    switch (ipin_side) {
        case TOP:
            return LEFT;
        case RIGHT:
            return BOTTOM;
        case BOTTOM:
            return LEFT;
        case LEFT:
            return BOTTOM;
        default:
            VTR_LOG("Invalid type of ipin_side!\n");
            exit(1);
    }
}

vtr::Point<size_t> RRGSB::get_side_block_coordinate(const e_side& side) const {
    SideManager side_manager(side);
    VTR_ASSERT(side_manager.validate());
    vtr::Point<size_t> ret(get_sb_x(), get_sb_y());

    switch (side_manager.get_side()) {
        case TOP:
            /* (0 == side) */
            /* 1. Channel Y [x][y+1] inputs */
            ret.set_y(ret.y() + 1);
            break;
        case RIGHT:
            /* 1 == side */
            /* 2. Channel X [x+1][y] inputs */
            ret.set_x(ret.x() + 1);
            break;
        case BOTTOM:
            /* 2 == side */
            /* 3. Channel Y [x][y] inputs */
            break;
        case LEFT:
            /* 3 == side */
            /* 4. Channel X [x][y] inputs */
            break;
        default:
            VTR_LOG(" Invalid side!\n");
            exit(1);
    }

    return ret;
}

vtr::Point<size_t> RRGSB::get_grid_coordinate() const {
    return coordinate_;
}

/************************************************************************
 * Public Mutators
 ***********************************************************************/
/* get a copy from a source */
void RRGSB::set(const RRGSB& src) {
    /* Copy coordinate */
    this->set_coordinate(src.get_sb_coordinate().x(), src.get_sb_coordinate().y());

    /* Initialize sides */
    this->init_num_sides(src.get_num_sides());

    /* Copy vectors */
    for (size_t side = 0; side < src.get_num_sides(); ++side) {
        SideManager side_manager(side);
        /* Copy chan_nodes */
        /* skip if there is no channel width */
        if (0 < src.get_chan_width(side_manager.get_side())) {
            this->chan_node_[side_manager.get_side()].set(src.chan_node_[side_manager.get_side()]);
            /* Copy chan_node_direction_*/
            this->chan_node_direction_[side_manager.get_side()].clear();
            for (size_t inode = 0; inode < src.get_chan_width(side_manager.get_side()); ++inode) {
                this->chan_node_direction_[side_manager.get_side()].push_back(src.get_chan_node_direction(side_manager.get_side(), inode));
            }
        }

        /* Copy opin_node and opin_node_grid_side_ */
        this->opin_node_[side_manager.get_side()].clear();
        for (size_t inode = 0; inode < src.get_num_opin_nodes(side_manager.get_side()); ++inode) {
            this->opin_node_[side_manager.get_side()].push_back(src.get_opin_node(side_manager.get_side(), inode));
        }

        /* Copy ipin_node and ipin_node_grid_side_ */
        this->ipin_node_[side_manager.get_side()].clear();
        for (size_t inode = 0; inode < src.get_num_ipin_nodes(side_manager.get_side()); ++inode) {
            this->ipin_node_[side_manager.get_side()].push_back(src.get_ipin_node(side_manager.get_side(), inode));
        }
    }
}

/* Set the coordinate (x,y) for the switch block */
void RRGSB::set_coordinate(const size_t& x, const size_t& y) {
    coordinate_.set(x, y);
}

/* Allocate the vectors with the given number of sides */
void RRGSB::init_num_sides(const size_t& num_sides) {
    /* Initialize the vectors */
    chan_node_.resize(num_sides);
    chan_node_direction_.resize(num_sides);
    ipin_node_.resize(num_sides);
    opin_node_.resize(num_sides);
}

/* Add a node to the chan_node_ list and also assign its direction in chan_node_direction_ */
void RRGSB::add_chan_node(const e_side& node_side,
                          const RRChan& rr_chan,
                          const std::vector<enum PORTS>& rr_chan_dir) {
    /* Validate: 1. side is valid, the type of node is valid */
    VTR_ASSERT(validate_side(node_side));

    /* fill the dedicated element in the vector */
    chan_node_[size_t(node_side)].set(rr_chan);
    chan_node_direction_[size_t(node_side)].resize(rr_chan_dir.size());
    for (size_t inode = 0; inode < rr_chan_dir.size(); ++inode) {
        chan_node_direction_[size_t(node_side)][inode] = rr_chan_dir[inode];
    }
}

/* Add a node to the chan_node_ list and also assign its direction in chan_node_direction_ */
void RRGSB::add_ipin_node(const RRNodeId& node, const e_side& node_side) {
    VTR_ASSERT(validate_side(node_side));
    /* push pack the dedicated element in the vector */
    ipin_node_[size_t(node_side)].push_back(node);
}

/* Add a node to the chan_node_ list and also assign its direction in chan_node_direction_ */
void RRGSB::add_opin_node(const RRNodeId& node, const e_side& node_side) {
    VTR_ASSERT(validate_side(node_side));
    /* push pack the dedicated element in the vector */
    opin_node_[size_t(node_side)].push_back(node);
}

void RRGSB::sort_chan_node_in_edges(const RRGraphView& rr_graph,
                                    const e_side& chan_side,
                                    const size_t& track_id) {
    std::map<size_t, std::map<size_t, RREdgeId>> from_grid_edge_map;
    std::map<size_t, std::map<size_t, RREdgeId>> from_track_edge_map;

    const RRNodeId& chan_node = chan_node_[size_t(chan_side)].get_node(track_id);

    /* Count the edges and ensure every of them has been sorted */
    size_t edge_counter = 0;

    /* For each incoming edge, find the node side and index in this GSB.
     * and cache these. Then we will use the data to sort the edge in the
     * following sequence:
     *  0----------------------------------------------------------------> num_in_edges()
     *  |<--TOP side-->|<--RIGHT side-->|<--BOTTOM side-->|<--LEFT side-->|
     *  For each side, the edge will be sorted by the node index starting from 0
     *  For each side, the edge from grid pins will be the 1st part
     *  while the edge from routing tracks will be the 2nd part
     */
    for (const RREdgeId& edge : rr_graph.node_in_edges(chan_node)) {
        /* We care the source node of this edge, and it should be an input of the GSB!!! */
        const RRNodeId& src_node = rr_graph.edge_src_node(edge);
        e_side side = NUM_2D_SIDES;
        int index = 0;
        get_node_side_and_index(rr_graph, src_node, IN_PORT, side, index);

        /* Must have valid side and index */
        if (NUM_2D_SIDES == side) {
            VTR_LOG("GSB[%lu][%lu]:\n", get_x(), get_y());
            VTR_LOG("----------------------------------\n");
            VTR_LOG("SRC node:\n");
            VTR_LOG("Node info: %s\n", rr_graph.node_coordinate_to_string(src_node).c_str());
            VTR_LOG("Node ptc: %d\n", rr_graph.node_ptc_num(src_node));
            VTR_LOG("Fan-out nodes:\n");
            for (const auto& temp_edge : rr_graph.edge_range(src_node)) {
                VTR_LOG("\t%s\n", rr_graph.node_coordinate_to_string(rr_graph.edge_sink_node(temp_edge)).c_str());
            }
            VTR_LOG("\n----------------------------------\n");
            VTR_LOG("Channel node:\n");
            VTR_LOG("Node info: %s\n", rr_graph.node_coordinate_to_string(chan_node).c_str());
            VTR_LOG("Node ptc: %d\n", rr_graph.node_ptc_num(chan_node));
            VTR_LOG("Fan-in nodes:\n");
            for (const auto& temp_edge : rr_graph.node_in_edges(chan_node)) {
                VTR_LOG("\t%s\n", rr_graph.node_coordinate_to_string(rr_graph.edge_src_node(temp_edge)).c_str());
            }
        }

        VTR_ASSERT(NUM_2D_SIDES != side);
        VTR_ASSERT(OPEN != index);

        if (OPIN == rr_graph.node_type(src_node)) {
            from_grid_edge_map[side][index] = edge;
        } else {
            VTR_ASSERT((CHANX == rr_graph.node_type(src_node))
                       || (CHANY == rr_graph.node_type(src_node)));
            from_track_edge_map[side][index] = edge;
        }

        edge_counter++;
    }

    /* Store the sorted edge */
    for (size_t side = 0; side < get_num_sides(); ++side) {
        /* Edges from grid outputs are the 1st part */
        for (size_t opin_id = 0; opin_id < opin_node_[side].size(); ++opin_id) {
            if ((0 < from_grid_edge_map.count(side))
                && (0 < from_grid_edge_map.at(side).count(opin_id))) {
                chan_node_in_edges_[size_t(chan_side)][track_id].push_back(from_grid_edge_map[side][opin_id]);
            }
        }

        /* Edges from routing tracks are the 2nd part */
        for (size_t itrack = 0; itrack < chan_node_[side].get_chan_width(); ++itrack) {
            if ((0 < from_track_edge_map.count(side))
                && (0 < from_track_edge_map.at(side).count(itrack))) {
                chan_node_in_edges_[size_t(chan_side)][track_id].push_back(from_track_edge_map[side][itrack]);
            }
        }
    }

    VTR_ASSERT(edge_counter == chan_node_in_edges_[size_t(chan_side)][track_id].size());
}

void RRGSB::sort_chan_node_in_edges(const RRGraphView& rr_graph) {
    /* Allocate here, as sort edge is optional, we do not allocate when adding nodes */
    chan_node_in_edges_.resize(get_num_sides());

    for (size_t side = 0; side < get_num_sides(); ++side) {
        SideManager side_manager(side);
        /* Bypass boundary GSBs here. When perimeter_cb option is on, Some GSBs may have only 1 side of CHANX or CHANY. There are no edges in the GSB, so we should skip them */
        chan_node_in_edges_[side].resize(chan_node_[side].get_chan_width());
        for (size_t track_id = 0; track_id < chan_node_[side].get_chan_width(); ++track_id) {
            /* Only sort the output nodes and bypass passing wires */
            if ((OUT_PORT == chan_node_direction_[side][track_id])
                && (false == is_sb_node_passing_wire(rr_graph, side_manager.get_side(), track_id))) {
                sort_chan_node_in_edges(rr_graph, side_manager.get_side(), track_id);
            }
        }
    }
}

void RRGSB::sort_ipin_node_in_edges(const RRGraphView& rr_graph,
                                    const e_side& ipin_side,
                                    const size_t& ipin_id) {

    std::array<std::map<size_t, RREdgeId>, NUM_2D_SIDES> from_track_edge_map;
    std::array<std::map<size_t, RREdgeId>, NUM_2D_SIDES> from_opin_edge_map;
    size_t edge_counter = 0;
    size_t side;

    const RRNodeId& ipin_node = ipin_node_[size_t(ipin_side)][ipin_id];

    /* Count the edges and ensure every of them has been sorted */

    /* For each incoming edge, find the node side and index in this GSB.
     * and cache these. Then we will use the data to sort the edge in the
     * following sequence:
     *  0----------------------------------------------------------------> num_in_edges()
     *  |<---------------------------1st part routing tracks ------------->
     *  |<--TOP side-->|<--RIGHT side-->|<--BOTTOM side-->|<--LEFT side-->|
     *  |<---------------------------2nd part IPINs ------------->
     *  |<--TOP side-->|<--RIGHT side-->|<--BOTTOM side-->|<--LEFT side-->|
     *  For each side, the edge will be sorted by the node index starting from 0
     *  For each side, the edge from grid pins will be the 2nd part (sorted by ptc number)
     *  while the edge from routing tracks will be the 1st part
     */
    for (const RREdgeId& edge : rr_graph.node_in_edges(ipin_node)) {
        /* We care the source node of this edge, and it should be an input of the GSB!!! */
        const RRNodeId& src_node = rr_graph.edge_src_node(edge);
        /* In this part, we only sort routing track nodes. IPIN nodes will be handled later */
        if (CHANX != rr_graph.node_type(src_node) && CHANY != rr_graph.node_type(src_node)) {
          continue;
        }
        /* The driver routing channel node can be either an input or an output to the GSB.
         * Just try to find a qualified one. */
        int index = OPEN;

        for (side = 0; side < get_num_sides(); ++side) {
            SideManager side_manager(side);
            e_side chan_side = side_manager.get_side();

            for (PORTS port_direc : {IN_PORT, OUT_PORT}) {
                index = get_node_index(rr_graph, src_node, chan_side, port_direc);
                if (OPEN != index) { break; }
            }
            if (OPEN != index) { break;}
        }

        /* Must have valid side and index */
        if (OPEN == index) {
            VTR_LOG("GSB[%lu][%lu]:\n", get_x(), get_y());
            VTR_LOG("----------------------------------\n");
            VTR_LOG("SRC node:\n");
            VTR_LOG("Node info: %s\n", rr_graph.node_coordinate_to_string(src_node).c_str());
            VTR_LOG("Node ptc: %d\n", rr_graph.node_ptc_num(src_node));
            VTR_LOG("Fan-out nodes:\n");
            for (const auto& temp_edge : rr_graph.edge_range(src_node)) {
                VTR_LOG("\t%s\n", rr_graph.node_coordinate_to_string(rr_graph.edge_sink_node(temp_edge)).c_str());
            }
            VTR_LOG("\n----------------------------------\n");
            VTR_LOG("IPIN node:\n");
            VTR_LOG("Node info: %s\n", rr_graph.node_coordinate_to_string(ipin_node).c_str());
            VTR_LOG("Node ptc: %d\n", rr_graph.node_ptc_num(ipin_node));
            VTR_LOG("Fan-in nodes:\n");
            for (const auto& temp_edge : rr_graph.node_in_edges(ipin_node)) {
                VTR_LOG("\t%s\n", rr_graph.node_coordinate_to_string(rr_graph.edge_src_node(temp_edge)).c_str());
            }
        }
        VTR_ASSERT(OPEN != index);

        from_track_edge_map[side][index] = edge;
        edge_counter++;
    }

    for (const RREdgeId& edge : rr_graph.node_in_edges(ipin_node)) {
        /* We care the source node of this edge, and it should be an input of the GSB!!! */
        const RRNodeId& src_node = rr_graph.edge_src_node(edge);
        /* In this part, we only sort routing track nodes. IPIN nodes will be handled later */
        if (OPIN != rr_graph.node_type(src_node)) {
          continue;
        }
        enum e_side cb_opin_side = NUM_2D_SIDES;
        int cb_opin_index = -1;
        get_node_side_and_index(rr_graph, src_node, IN_PORT, cb_opin_side,
                                cb_opin_index);
        VTR_ASSERT((-1 != cb_opin_index) && (NUM_2D_SIDES != cb_opin_side));
        /* Must have valid side and index */
        if (OPEN == cb_opin_index || NUM_2D_SIDES == cb_opin_side) {
            VTR_LOG("GSB[%lu][%lu]:\n", get_x(), get_y());
            VTR_LOG("----------------------------------\n");
            VTR_LOG("SRC node:\n");
            VTR_LOG("Node info: %s\n", rr_graph.node_coordinate_to_string(src_node).c_str());
            VTR_LOG("Node ptc: %d\n", rr_graph.node_ptc_num(src_node));
            VTR_LOG("Fan-out nodes:\n");
            for (const auto& temp_edge : rr_graph.edge_range(src_node)) {
                VTR_LOG("\t%s\n", rr_graph.node_coordinate_to_string(rr_graph.edge_sink_node(temp_edge)).c_str());
            }
            VTR_LOG("\n----------------------------------\n");
            VTR_LOG("IPIN node:\n");
            VTR_LOG("Node info: %s\n", rr_graph.node_coordinate_to_string(ipin_node).c_str());
            VTR_LOG("Node ptc: %d\n", rr_graph.node_ptc_num(ipin_node));
            VTR_LOG("Fan-in nodes:\n");
            for (const auto& temp_edge : rr_graph.node_in_edges(ipin_node)) {
                VTR_LOG("\t%s\n", rr_graph.node_coordinate_to_string(rr_graph.edge_src_node(temp_edge)).c_str());
            }
        }
        from_opin_edge_map[size_t(cb_opin_side)][cb_opin_index] = edge;
        edge_counter++;
    }

    /* Store the sorted edge */
    for (size_t iside = 0; iside < get_num_sides(); ++iside) {
        for (size_t itrack = 0; itrack < chan_node_[iside].get_chan_width(); ++itrack) {
            if (0 < from_track_edge_map[iside].count(itrack)) {
                ipin_node_in_edges_[size_t(ipin_side)][ipin_id].push_back(from_track_edge_map[iside][itrack]);
            }
        }
    }

    for (e_side iside : {TOP, RIGHT, BOTTOM, LEFT}) {
        for (size_t ipin = 0; ipin < get_num_opin_nodes(iside); ++ipin) {
            if (0 < from_opin_edge_map[size_t(iside)].count(ipin)) {
                ipin_node_in_edges_[size_t(ipin_side)][ipin_id].push_back(from_opin_edge_map[size_t(iside)][ipin]);
            }
        }
    }

    VTR_LOG("Edge counter: %lu, ipin_node_in_edges_ size: %lu\n", edge_counter, ipin_node_in_edges_[size_t(ipin_side)][ipin_id].size());
    VTR_ASSERT(edge_counter == ipin_node_in_edges_[size_t(ipin_side)][ipin_id].size());
}

void RRGSB::sort_ipin_node_in_edges(const RRGraphView& rr_graph) {
    /* Allocate here, as sort edge is optional, we do not allocate when adding nodes */
    ipin_node_in_edges_.resize(get_num_sides());

    for (t_rr_type cb_type : {CHANX, CHANY}) {
        for (e_side ipin_side : get_cb_ipin_sides(cb_type)) {
            SideManager side_manager(ipin_side);
            ipin_node_in_edges_[size_t(ipin_side)].resize(ipin_node_[size_t(ipin_side)].size());
            for (size_t ipin_id = 0; ipin_id < ipin_node_[size_t(ipin_side)].size(); ++ipin_id) {
                sort_ipin_node_in_edges(rr_graph, side_manager.get_side(), ipin_id);
            }
        }
    }
}

void RRGSB::build_cb_opin_nodes(const RRGraphView& rr_graph) {
  for (t_rr_type cb_type : {CHANX, CHANY}) {
    size_t icb_type = cb_type == CHANX ? 0 : 1;
    std::vector<enum e_side> cb_ipin_sides = get_cb_ipin_sides(cb_type);
    for (size_t iside = 0; iside < cb_ipin_sides.size(); ++iside) {
      enum e_side cb_ipin_side = cb_ipin_sides[iside];
      for (size_t inode = 0; inode < get_num_ipin_nodes(cb_ipin_side);
           ++inode) {
        std::vector<RREdgeId> driver_rr_edges =
          get_ipin_node_in_edges(rr_graph, cb_ipin_side, inode);
        for (const RREdgeId curr_edge : driver_rr_edges) {
          RRNodeId cand_node = rr_graph.edge_src_node(curr_edge);
          if (OPIN != rr_graph.node_type(cand_node)) {
            continue;
          }
          enum e_side cb_opin_side = NUM_2D_SIDES;
          int cb_opin_index = -1;
          get_node_side_and_index(rr_graph, cand_node, IN_PORT, cb_opin_side,
                                  cb_opin_index);
          if ((-1 == cb_opin_index) || (NUM_2D_SIDES == cb_opin_side)) {
              VTR_LOG("GSB[%lu][%lu]:\n", get_x(), get_y());
              VTR_LOG("----------------------------------\n");
              VTR_LOG("SRC node:\n");
              VTR_LOG("Node info: %s\n", rr_graph.node_coordinate_to_string(cand_node).c_str());
              VTR_LOG("Node ptc: %d\n", rr_graph.node_ptc_num(cand_node));
              VTR_LOG("Fan-out nodes:\n");
              for (const auto& temp_edge : rr_graph.edge_range(cand_node)) {
                  VTR_LOG("\t%s\n", rr_graph.node_coordinate_to_string(rr_graph.edge_sink_node(temp_edge)).c_str());
              }
          }
          VTR_ASSERT((-1 != cb_opin_index) && (NUM_2D_SIDES != cb_opin_side));

          if (cb_opin_node_[icb_type][size_t(cb_opin_side)].end() ==
              std::find(cb_opin_node_[icb_type][size_t(cb_opin_side)].begin(), cb_opin_node_[icb_type][size_t(cb_opin_side)].end(), cand_node)) {
            cb_opin_node_[icb_type][size_t(cb_opin_side)].push_back(cand_node);
          }
        }
      }
    }
  }
}

/************************************************************************
 * Public Mutators: clean-up functions
 ***********************************************************************/
/* Reset the RRGSB to pristine state */
void RRGSB::clear() {
    /* Clean all the vectors */
    VTR_ASSERT(validate_num_sides());
    /* Clear the inner vector of each matrix */
    for (size_t side = 0; side < get_num_sides(); ++side) {
        chan_node_direction_[side].clear();
        chan_node_[side].clear();
        ipin_node_[side].clear();
        opin_node_[side].clear();
    }
    chan_node_direction_.clear();
    chan_node_.clear();
    ipin_node_.clear();
    opin_node_.clear();
}

/* Clean the chan_width of a side */
void RRGSB::clear_chan_nodes(const e_side& node_side) {
    VTR_ASSERT(validate_side(node_side));

    chan_node_[size_t(node_side)].clear();
    chan_node_direction_[size_t(node_side)].clear();
}

/* Clean the number of IPINs of a side */
void RRGSB::clear_ipin_nodes(const e_side& node_side) {
    VTR_ASSERT(validate_side(node_side));

    ipin_node_[size_t(node_side)].clear();
}

/* Clean the number of OPINs of a side */
void RRGSB::clear_opin_nodes(const e_side& node_side) {
    VTR_ASSERT(validate_side(node_side));

    opin_node_[size_t(node_side)].clear();
}

/* Clean chan/opin/ipin nodes at one side */
void RRGSB::clear_one_side(const e_side& node_side) {
    clear_chan_nodes(node_side);
    clear_ipin_nodes(node_side);
    clear_opin_nodes(node_side);
}

/************************************************************************
 * Internal Accessors: identify mirrors
 ***********************************************************************/
size_t RRGSB::get_track_id_first_short_connection(const RRGraphView& rr_graph, const e_side& node_side) const {
    VTR_ASSERT(validate_side(node_side));

    /* Walk through chan_nodes and find the first short connection */
    for (size_t inode = 0; inode < get_chan_width(node_side); ++inode) {
        if (true == is_sb_node_passing_wire(rr_graph, node_side, inode)) {
            return inode;
        }
    }

    return size_t(-1);
}

/************************************************************************
 * Internal validators
 ***********************************************************************/
/* Validate if the number of sides are consistent among internal data arrays ! */
bool RRGSB::validate_num_sides() const {
    size_t num_sides = chan_node_direction_.size();

    if (num_sides != chan_node_.size()) {
        return false;
    }

    if (num_sides != ipin_node_.size()) {
        return false;
    }

    if (num_sides != opin_node_.size()) {
        return false;
    }

    return true;
}

/* Check if the side valid in the context: does the switch block have the side? */
bool RRGSB::validate_side(const e_side& side) const {
    return (size_t(side) < get_num_sides());
}

/* Check the track_id is valid for chan_node_ and chan_node_direction_ */
bool RRGSB::validate_track_id(const e_side& side, const size_t& track_id) const {
    if (false == validate_side(side)) {
        return false;
    }

    return ((track_id < chan_node_[size_t(side)].get_chan_width())
            && (track_id < chan_node_direction_[size_t(side)].size()));
}

/* Check the opin_node_id is valid for opin_node_ and opin_node_grid_side_ */
bool RRGSB::validate_opin_node_id(const e_side& side, const size_t& node_id) const {
    if (false == validate_side(side)) {
        return false;
    }
    return (node_id < opin_node_[size_t(side)].size());
}

/* Check the opin_node_id is valid for opin_node_ and opin_node_grid_side_ */
bool RRGSB::validate_cb_opin_node_id(const t_rr_type& cb_type, const e_side& side, const size_t& node_id) const {
    if (false == validate_side(side)) {
        return false;
    }
    size_t icb_type = get_cb_opin_type_id(cb_type);
    return (node_id < cb_opin_node_[icb_type][size_t(side)].size());
}

/* Check the ipin_node_id is valid for opin_node_ and opin_node_grid_side_ */
bool RRGSB::validate_ipin_node_id(const e_side& side, const size_t& node_id) const {
    if (false == validate_side(side)) {
        return false;
    }
    return (node_id < ipin_node_[size_t(side)].size());
}

bool RRGSB::validate_cb_type(const t_rr_type& cb_type) const {
    return ((CHANX == cb_type) || (CHANY == cb_type));
}

size_t RRGSB::get_cb_opin_type_id(const t_rr_type& cb_type) const {
    VTR_ASSERT(validate_cb_type(cb_type));
    return cb_type == CHANX ? 0 : 1;
}
