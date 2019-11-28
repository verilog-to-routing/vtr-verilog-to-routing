/************************************************************************
 * Member Functions of RRGraph
 * include mutators, accessors and utility functions 
 ***********************************************************************/
#include <cmath>
#include <algorithm>
#include <map>
#include <limits>

#include "vtr_vector_map.h"
#include "vtr_log.h"
#include "vtr_util.h"
#include "vtr_assert.h"
#include "rr_graph_obj.h"
#include "rr_graph_obj_utils.h"

//Accessors
RRGraph::node_range RRGraph::nodes() const {
    return vtr::make_range(node_ids_.begin(), node_ids_.end());
}

RRGraph::edge_range RRGraph::edges() const {
    return vtr::make_range(edge_ids_.begin(), edge_ids_.end());
}

RRGraph::switch_range RRGraph::switches() const {
    return vtr::make_range(switch_ids_.begin(), switch_ids_.end());
}

RRGraph::segment_range RRGraph::segments() const {
    return vtr::make_range(segment_ids_.begin(), segment_ids_.end());
}

//Node attributes
t_rr_type RRGraph::node_type(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    return node_types_[node];
}

size_t RRGraph::node_index(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    return size_t(node);
}

short RRGraph::node_xlow(const RRNodeId& node) const {
    return node_bounding_box(node).xmin();
}

short RRGraph::node_ylow(const RRNodeId& node) const {
    return node_bounding_box(node).ymin();
}

short RRGraph::node_xhigh(const RRNodeId& node) const {
    return node_bounding_box(node).xmax();
}

short RRGraph::node_yhigh(const RRNodeId& node) const {
    return node_bounding_box(node).ymax();
}

short RRGraph::node_length(const RRNodeId& node) const {
    return std::max(node_xhigh(node) - node_xlow(node), node_yhigh(node) - node_ylow(node));
}

vtr::Rect<short> RRGraph::node_bounding_box(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    return node_bounding_boxes_[node];
}

/* Node starting and ending points */
/************************************************************************
 * Get the coordinator of a starting point of a routing track 
 * For routing tracks in INC_DIRECTION
 * (xlow, ylow) should be the starting point 
 *
 * For routing tracks in DEC_DIRECTION
 * (xhigh, yhigh) should be the starting point 
 *
 * For routing tracks in BI_DIRECTION
 * we always use (xhigh, yhigh)
 ***********************************************************************/
vtr::Point<short> RRGraph::node_start_coordinate(const RRNodeId& node) const {
    /* Make sure we have CHANX or CHANY */
    VTR_ASSERT((CHANX == node_type(node)) || (CHANY == node_type(node)));

    vtr::Point<short> start_coordinate(node_xlow(node), node_ylow(node));

    if (DEC_DIRECTION == node_direction(node)) {
        start_coordinate.set(node_xhigh(node), node_yhigh(node));
    }

    return start_coordinate;
}

/************************************************************************
 * Get the coordinator of a end point of a routing track 
 * For routing tracks in INC_DIRECTION
 * (xhigh, yhigh) should be the ending point 
 *
 * For routing tracks in DEC_DIRECTION
 * (xlow, ylow) should be the ending point 
 *
 * For routing tracks in BI_DIRECTION
 * we always use (xhigh, yhigh)
 ***********************************************************************/
vtr::Point<short> RRGraph::node_end_coordinate(const RRNodeId& node) const {
    /* Make sure we have CHANX or CHANY */
    VTR_ASSERT((CHANX == node_type(node)) || (CHANY == node_type(node)));

    vtr::Point<short> end_coordinate(node_xhigh(node), node_yhigh(node));

    if (DEC_DIRECTION == node_direction(node)) {
        end_coordinate.set(node_xlow(node), node_ylow(node));
    }

    return end_coordinate;
}

short RRGraph::node_fan_in(const RRNodeId& node) const {
    return node_in_edges(node).size();
}

short RRGraph::node_fan_out(const RRNodeId& node) const {
    return node_out_edges(node).size();
}

short RRGraph::node_capacity(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    return node_capacities_[node];
}

short RRGraph::node_ptc_num(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    return node_ptc_nums_[node];
}

short RRGraph::node_pin_num(const RRNodeId& node) const {
    VTR_ASSERT_MSG(node_type(node) == IPIN || node_type(node) == OPIN,
                   "Pin number valid only for IPIN/OPIN RR nodes");
    return node_ptc_num(node);
}

short RRGraph::node_track_num(const RRNodeId& node) const {
    VTR_ASSERT_MSG(node_type(node) == CHANX || node_type(node) == CHANY,
                   "Track number valid only for CHANX/CHANY RR nodes");
    return node_ptc_num(node);
}

short RRGraph::node_class_num(const RRNodeId& node) const {
    VTR_ASSERT_MSG(node_type(node) == SOURCE || node_type(node) == SINK, "Class number valid only for SOURCE/SINK RR nodes");
    return node_ptc_num(node);
}

short RRGraph::node_cost_index(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    return node_cost_indices_[node];
}

e_direction RRGraph::node_direction(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    VTR_ASSERT_MSG(node_type(node) == CHANX || node_type(node) == CHANY, "Direction valid only for CHANX/CHANY RR nodes");
    return node_directions_[node];
}

e_side RRGraph::node_side(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    VTR_ASSERT_MSG(node_type(node) == IPIN || node_type(node) == OPIN, "Side valid only for IPIN/OPIN RR nodes");
    return node_sides_[node];
}

/* Get the resistance of a node */
float RRGraph::node_R(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    return node_Rs_[node];
}

/* Get the capacitance of a node */
float RRGraph::node_C(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    return node_Cs_[node];
}

/*
 * Get a segment id of a node in rr_graph 
 */
RRSegmentId RRGraph::node_segment(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));

    return node_segments_[node];
}

/* 
 * Get the number of configurable input edges of a node
 * TODO: we would use the node_num_configurable_in_edges() 
 * when the rr_graph edges have been partitioned 
 * This can avoid unneccessary walkthrough
 */
short RRGraph::node_num_configurable_in_edges(const RRNodeId& node) const {
    return node_configurable_in_edges(node).size();
}

/* 
 * Get the number of configurable output edges of a node
 * TODO: we would use the node_num_configurable_out_edges() 
 * when the rr_graph edges have been partitioned 
 * This can avoid unneccessary walkthrough
 */
short RRGraph::node_num_configurable_out_edges(const RRNodeId& node) const {
    return node_configurable_out_edges(node).size();
}

/* 
 * Get the number of non-configurable input edges of a node
 * TODO: we would use the node_num_configurable_in_edges() 
 * when the rr_graph edges have been partitioned 
 * This can avoid unneccessary walkthrough
 */
short RRGraph::node_num_non_configurable_in_edges(const RRNodeId& node) const {
    return node_non_configurable_in_edges(node).size();
}

/* 
 * Get the number of non-configurable output edges of a node
 * TODO: we would use the node_num_configurable_out_edges() 
 * when the rr_graph edges have been partitioned 
 * This can avoid unneccessary walkthrough
 */
short RRGraph::node_num_non_configurable_out_edges(const RRNodeId& node) const {
    return node_non_configurable_out_edges(node).size();
}

/* Get the list of configurable edges from the input edges of a given node 
 * And return the range(iterators) of the list 
 */
RRGraph::edge_range RRGraph::node_configurable_in_edges(const RRNodeId& node) const {
    /* Make sure we will access a valid node */
    VTR_ASSERT_SAFE(valid_node_id(node));

    /* By default the configurable edges will be stored at the first part of the edge list (0 to XX) */
    auto begin = node_in_edges(node).begin();

    /* By default the non-configurable edges will be stored at second part of the edge list (XX to end) */
    auto end = node_in_edges(node).end() - node_num_non_configurable_in_edges_[node];

    return vtr::make_range(begin, end);
}

/* Get the list of configurable edges from the input edges of a given node 
 * And return the range(iterators) of the list 
 */
RRGraph::edge_range RRGraph::node_non_configurable_in_edges(const RRNodeId& node) const {
    /* Make sure we will access a valid node */
    VTR_ASSERT_SAFE(valid_node_id(node));

    /* By default the configurable edges will be stored at the first part of the edge list (0 to XX) */
    auto begin = node_in_edges(node).end() - node_num_non_configurable_in_edges_[node];

    /* By default the non-configurable edges will be stored at second part of the edge list (XX to end) */
    auto end = node_in_edges(node).end();

    return vtr::make_range(begin, end);
}

/* Get the list of configurable edges from the input edges of a given node 
 * And return the range(iterators) of the list 
 */
RRGraph::edge_range RRGraph::node_configurable_out_edges(const RRNodeId& node) const {
    /* Make sure we will access a valid node */
    VTR_ASSERT_SAFE(valid_node_id(node));

    /* By default the configurable edges will be stored at the first part of the edge list (0 to XX) */
    auto begin = node_out_edges(node).begin();

    /* By default the non-configurable edges will be stored at second part of the edge list (XX to end) */
    auto end = node_out_edges(node).end() - node_num_non_configurable_out_edges_[node];

    return vtr::make_range(begin, end);
}

/* Get the list of configurable edges from the input edges of a given node 
 * And return the range(iterators) of the list 
 */
RRGraph::edge_range RRGraph::node_non_configurable_out_edges(const RRNodeId& node) const {
    /* Make sure we will access a valid node */
    VTR_ASSERT_SAFE(valid_node_id(node));

    /* By default the configurable edges will be stored at the first part of the edge list (0 to XX) */
    auto begin = node_out_edges(node).end() - node_num_non_configurable_out_edges_[node];

    /* By default the non-configurable edges will be stored at second part of the edge list (XX to end) */
    auto end = node_out_edges(node).end();

    return vtr::make_range(begin, end);
}

RRGraph::edge_range RRGraph::node_out_edges(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    return vtr::make_range(node_out_edges_[node].begin(), node_out_edges_[node].end());
}

RRGraph::edge_range RRGraph::node_in_edges(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    return vtr::make_range(node_in_edges_[node].begin(), node_in_edges_[node].end());
}

//Edge attributes
size_t RRGraph::edge_index(const RREdgeId& edge) const {
    VTR_ASSERT_SAFE(valid_edge_id(edge));
    return size_t(edge);
}

RRNodeId RRGraph::edge_src_node(const RREdgeId& edge) const {
    VTR_ASSERT_SAFE(valid_edge_id(edge));
    return edge_src_nodes_[edge];
}
RRNodeId RRGraph::edge_sink_node(const RREdgeId& edge) const {
    VTR_ASSERT_SAFE(valid_edge_id(edge));
    return edge_sink_nodes_[edge];
}

RRSwitchId RRGraph::edge_switch(const RREdgeId& edge) const {
    VTR_ASSERT_SAFE(valid_edge_id(edge));
    return edge_switches_[edge];
}

/* Check if the edge is a configurable edge (programmble) */
bool RRGraph::edge_is_configurable(const RREdgeId& edge) const {
    /* Make sure we have a valid edge id */
    VTR_ASSERT_SAFE(valid_edge_id(edge));

    auto iswitch = edge_switch(edge);

    return switches_[iswitch].configurable();
}

/* Check if the edge is a non-configurable edge (hardwired) */
bool RRGraph::edge_is_non_configurable(const RREdgeId& edge) const {
    /* Make sure we have a valid edge id */
    VTR_ASSERT_SAFE(valid_edge_id(edge));
    return !edge_is_configurable(edge);
}

size_t RRGraph::switch_index(const RRSwitchId& switch_id) const {
    VTR_ASSERT_SAFE(valid_switch_id(switch_id));
    return size_t(switch_id);
}

/*
 * Get a switch from the rr_switch list with a given id  
 */
const t_rr_switch_inf& RRGraph::get_switch(const RRSwitchId& switch_id) const {
    VTR_ASSERT_SAFE(valid_switch_id(switch_id));
    return switches_[switch_id];
}

size_t RRGraph::segment_index(const RRSegmentId& segment_id) const {
    VTR_ASSERT_SAFE(valid_segment_id(segment_id));
    return size_t(segment_id);
}

/*
 * Get a segment from the segment list with a given id  
 */
const t_segment_inf& RRGraph::get_segment(const RRSegmentId& segment_id) const {
    VTR_ASSERT_SAFE(valid_segment_id(segment_id));
    return segments_[segment_id];
}

/************************************************************************
 * Find all the edges interconnecting two nodes
 * Return a vector of the edge ids
 ***********************************************************************/
std::vector<RREdgeId> RRGraph::find_edges(const RRNodeId& src_node, const RRNodeId& sink_node) const {
    std::vector<RREdgeId> matching_edges;

    /* Iterate over the outgoing edges of the source node */
    for (auto edge : node_out_edges(src_node)) {
        if (edge_sink_node(edge) == sink_node) {
            /* Update edge list to return */
            matching_edges.push_back(edge);
        }
    }

    return matching_edges;
}

RRNodeId RRGraph::find_node(const short& x, const short& y, const t_rr_type& type, const int& ptc, const e_side& side) const {
    initialize_fast_node_lookup();

    size_t itype = type;
    size_t iside = side;

    /* Check if x, y, type and ptc, side is valid */
    if ((x < 0)                                     /* See if x is smaller than the index of first element */
        || (size_t(x) > node_lookup_.size() - 1)) { /* See if x is large than the index of last element */
        /* Return a zero range! */
        return RRNodeId::INVALID();
    }

    /* Check if x, y, type and ptc, side is valid */
    if ((y < 0)                                        /* See if y is smaller than the index of first element */
        || (size_t(y) > node_lookup_[x].size() - 1)) { /* See if y is large than the index of last element */
        /* Return a zero range! */
        return RRNodeId::INVALID();
    }

    /* Check if x, y, type and ptc, side is valid */
    /* itype is always larger than -1, we can skip checking */
    if (itype > node_lookup_[x][y].size() - 1) { /* See if type is large than the index of last element */
        /* Return a zero range! */
        return RRNodeId::INVALID();
    }

    /* Check if x, y, type and ptc, side is valid */
    if ((ptc < 0)                                                 /* See if ptc is smaller than the index of first element */
        || (size_t(ptc) > node_lookup_[x][y][type].size() - 1)) { /* See if ptc is large than the index of last element */
        /* Return a zero range! */
        return RRNodeId::INVALID();
    }

    /* Check if x, y, type and ptc, side is valid */
    /* iside is always larger than -1, we can skip checking */
    if (iside > node_lookup_[x][y][type][ptc].size() - 1) { /* See if side is large than the index of last element */
        /* Return a zero range! */
        return RRNodeId::INVALID();
    }

    return node_lookup_[x][y][itype][ptc][iside];
}

/* Find the channel width (number of tracks) of a channel [x][y] */
short RRGraph::chan_num_tracks(const short& x, const short& y, const t_rr_type& type) const {
    /* Must be CHANX or CHANY */
    VTR_ASSERT_MSG(CHANX == type || CHANY == type,
                   "Required node_type to be CHANX or CHANY!");
    initialize_fast_node_lookup();

    /* Check if x, y, type and ptc is valid */
    if ((x < 0)                                     /* See if x is smaller than the index of first element */
        || (size_t(x) > node_lookup_.size() - 1)) { /* See if x is large than the index of last element */
        /* Return a zero range! */
        return 0;
    }

    /* Check if x, y, type and ptc is valid */
    if ((y < 0)                                        /* See if y is smaller than the index of first element */
        || (size_t(y) > node_lookup_[x].size() - 1)) { /* See if y is large than the index of last element */
        /* Return a zero range! */
        return 0;
    }

    /* Check if x, y, type and ptc is valid */
    if ((size_t(type) > node_lookup_[x][y].size() - 1)) { /* See if type is large than the index of last element */
        /* Return a zero range! */
        return 0;
    }

    const auto& matching_nodes = node_lookup_[x][y][type];

    return vtr::make_range(matching_nodes.begin(), matching_nodes.end()).size();
}

/* This function aims to print basic information about a node */
void RRGraph::print_node(const RRNodeId& node) const {
    VTR_LOG("Node id: %d\n", node_index(node));
    VTR_LOG("Node type: %s\n", rr_node_typename[node_type(node)]);
    VTR_LOG("Node xlow: %d\n", node_xlow(node));
    VTR_LOG("Node ylow: %d\n", node_ylow(node));
    VTR_LOG("Node xhigh: %d\n", node_xhigh(node));
    VTR_LOG("Node yhigh: %d\n", node_yhigh(node));
    VTR_LOG("Node ptc: %d\n", node_ptc_num(node));
    VTR_LOG("Node num in_edges: %d\n", node_in_edges(node).size());
    VTR_LOG("Node num out_edges: %d\n", node_out_edges(node).size());
}

/* Check if the segment id of a node is in range */
bool RRGraph::validate_node_segment(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    /* Only CHANX and CHANY requires a valid segment id */
    if ((CHANX == node_type(node))
        || (CHANY == node_type(node))) {
        return valid_segment_id(node_segments_[node]);
    } else {
        return true;
    }
}

/* Check if the segment id of every node is in range */
bool RRGraph::validate_node_segments() const {
    bool all_valid = true;
    for (auto node : nodes()) {
        if (true == validate_node_segment(node)) {
            continue;
        }
        /* Reach here it means we find an invalid segment id */
        all_valid = false;
        /* Print a warning! */
        VTR_LOG_WARN("Node %d has an invalid segment id (%d)!\n",
                     size_t(node), size_t(node_segment(node)));
    }
    return all_valid;
}

/* Check if the switch id of a edge is in range */
bool RRGraph::validate_edge_switch(const RREdgeId& edge) const {
    VTR_ASSERT_SAFE(valid_edge_id(edge));
    return valid_switch_id(edge_switches_[edge]);
}

/* Check if the switch id of every edge is in range */
bool RRGraph::validate_edge_switches() const {
    bool all_valid = true;
    for (auto edge : edges()) {
        if (true == validate_edge_switch(edge)) {
            continue;
        }
        /* Reach here it means we find an invalid segment id */
        all_valid = false;
        /* Print a warning! */
        VTR_LOG_WARN("Edge %d has an invalid switch id (%d)!\n",
                     size_t(edge), size_t(edge_switch(edge)));
    }
    return all_valid;
}

/* Check if a node is in the list of source_nodes of a edge */
bool RRGraph::validate_node_is_edge_src(const RRNodeId& node, const RREdgeId& edge) const {
    /* Assure a valid node id */
    VTR_ASSERT_SAFE(valid_node_id(node));
    /* assure a valid edge id */
    VTR_ASSERT_SAFE(valid_edge_id(edge));
    /* find if the node is the src */
    if (node == edge_src_node(edge)) {
        return true; /* confirmed source node*/
    } else {
        return false; /* not a source */
    }
}

/* Check if a node is in the list of sink_nodes of a edge */
bool RRGraph::validate_node_is_edge_sink(const RRNodeId& node, const RREdgeId& edge) const {
    /* Assure a valid node id */
    VTR_ASSERT_SAFE(valid_node_id(node));
    /* assure a valid edge id */
    VTR_ASSERT_SAFE(valid_edge_id(edge));
    /* find if the node is the sink */
    if (node == edge_sink_node(edge)) {
        return true; /* confirmed source node*/
    } else {
        return false; /* not a source */
    }
}

/* This function will check if a node has valid input edges
 * 1. Check the edge ids are valid
 * 2. Check the node is in the list of edge_sink_node
 */
bool RRGraph::validate_node_in_edges(const RRNodeId& node) const {
    bool all_valid = true;
    /* Assure a valid node id */
    VTR_ASSERT_SAFE(valid_node_id(node));
    /* Check each edge */
    for (auto edge : node_in_edges(node)) {
        /* assure a valid edge id */
        VTR_ASSERT_SAFE(valid_edge_id(edge));
        /* check the node is in the list of edge_sink_node */
        if (true == validate_node_is_edge_sink(node, edge)) {
            continue;
        }
        /* Reach here, it means there is something wrong! 
         * Print a warning  
         */
        VTR_LOG_WARN("Edge %d is in the input edge list of node %d while the node is not in edge's sink node list!\n",
                     size_t(edge), size_t(node));
        all_valid = false;
    }

    return all_valid;
}

/* This function will check if a node has valid output edges
 * 1. Check the edge ids are valid
 * 2. Check the node is in the list of edge_source_node
 */
bool RRGraph::validate_node_out_edges(const RRNodeId& node) const {
    bool all_valid = true;
    /* Assure a valid node id */
    VTR_ASSERT_SAFE(valid_node_id(node));
    /* Check each edge */
    for (auto edge : node_out_edges(node)) {
        /* assure a valid edge id */
        VTR_ASSERT_SAFE(valid_edge_id(edge));
        /* check the node is in the list of edge_sink_node */
        if (true == validate_node_is_edge_src(node, edge)) {
            continue;
        }
        /* Reach here, it means there is something wrong! 
         * Print a warning  
         */
        VTR_LOG_WARN("Edge %d is in the output edge list of node %d while the node is not in edge's source node list!\n",
                     size_t(edge), size_t(node));
        all_valid = false;
    }

    return all_valid;
}

/* check all the edges of a node */
bool RRGraph::validate_node_edges(const RRNodeId& node) const {
    bool all_valid = true;

    if (false == validate_node_in_edges(node)) {
        all_valid = false;
    }
    if (false == validate_node_out_edges(node)) {
        all_valid = false;
    }

    return all_valid;
}

/* check if all the nodes' input edges are valid */
bool RRGraph::validate_nodes_in_edges() const {
    bool all_valid = true;
    for (auto node : nodes()) {
        if (true == validate_node_in_edges(node)) {
            continue;
        }
        /* Reach here, it means there is something wrong! 
         * Print a warning  
         */
        all_valid = false;
    }
    return all_valid;
}

/* check if all the nodes' output edges are valid */
bool RRGraph::validate_nodes_out_edges() const {
    bool all_valid = true;
    for (auto node : nodes()) {
        if (true == validate_node_out_edges(node)) {
            continue;
        }
        /* Reach here, it means there is something wrong! 
         * Print a warning  
         */
        all_valid = false;
    }
    return all_valid;
}

/* check all the edges of every node */
bool RRGraph::validate_nodes_edges() const {
    bool all_valid = true;

    if (false == validate_nodes_in_edges()) {
        all_valid = false;
    }
    if (false == validate_nodes_out_edges()) {
        all_valid = false;
    }

    return all_valid;
}

/* Check if source node of a edge is valid */
bool RRGraph::validate_edge_src_node(const RREdgeId& edge) const {
    return valid_node_id(edge_src_node(edge));
}

/* Check if sink node of a edge is valid */
bool RRGraph::validate_edge_sink_node(const RREdgeId& edge) const {
    return valid_node_id(edge_sink_node(edge));
}

/* Check if source nodes of a edge are all valid */
bool RRGraph::validate_edge_src_nodes() const {
    bool all_valid = true;
    for (auto edge : edges()) {
        if (true == validate_edge_src_node(edge)) {
            continue;
        }
        /* Reach here, it means there is something wrong! 
         * Print a warning  
         */
        VTR_LOG_WARN("Edge %d has a invalid source node %d!\n",
                     size_t(edge), size_t(edge_src_node(edge)));
        all_valid = false;
    }
    return all_valid;
}

/* Check if source nodes of a edge are all valid */
bool RRGraph::validate_edge_sink_nodes() const {
    bool all_valid = true;
    for (auto edge : edges()) {
        if (true == validate_edge_sink_node(edge)) {
            continue;
        }
        /* Reach here, it means there is something wrong! 
         * Print a warning  
         */
        VTR_LOG_WARN("Edge %d has a invalid sink node %d!\n",
                     size_t(edge), size_t(edge_sink_node(edge)));
        all_valid = false;
    }
    return all_valid;
}

/* This function should be used when nodes, edges, switches and segments have been used after 
 * We will build the fast_lookup, partition edges and check 
 * This function run fundamental and optional checks on internal data 
 * Errors are thrown if fundamental checking fails
 * Warnings are thrown if optional checking fails
 */
bool RRGraph::validate() const {
    bool check_flag = true;
    size_t num_err = 0;

    initialize_fast_node_lookup();

    /* Validate the sizes of nodes and node-related vectors 
     * Validate the sizes of edges and edge-related vectors 
     */
    if (false == validate_sizes()) {
        VTR_LOG_WARN("Fail in validating node- and edge-related vector sizes!\n");
        check_flag = false;
        num_err++;
    }

    /* Fundamental check */
    if (false == validate_nodes_edges()) {
        VTR_LOG_WARN("Fail in validating edges connected to each node!\n");
        check_flag = false;
        num_err++;
    }

    if (false == validate_node_segments()) {
        VTR_LOG_WARN("Fail in validating segment IDs of nodes !\n");
        check_flag = false;
        num_err++;
    }

    if (false == validate_edge_switches()) {
        VTR_LOG_WARN("Fail in validating switch IDs of edges !\n");
        check_flag = false;
        num_err++;
    }

    /* Error out if there is any fatal errors found */
    VTR_LOG_ERROR("Routing Resource graph is not valid due to %d fatal errors !\n",
                  num_err);

    return check_flag;
}

bool RRGraph::is_dirty() const {
    return dirty_;
}

void RRGraph::set_dirty() {
    dirty_ = true;
}

void RRGraph::clear_dirty() {
    dirty_ = false;
}

/* Reserve a list of nodes */
void RRGraph::reserve_nodes(const int& num_nodes) {
    /* Reserve the full set of vectors related to nodes */
    /* Basic information */
    this->node_ids_.reserve(num_nodes);
    this->node_types_.reserve(num_nodes);
    this->node_bounding_boxes_.reserve(num_nodes);
    this->node_capacities_.reserve(num_nodes);
    this->node_ptc_nums_.reserve(num_nodes);
    this->node_directions_.reserve(num_nodes);
    this->node_sides_.reserve(num_nodes);
    this->node_Rs_.reserve(num_nodes);
    this->node_Cs_.reserve(num_nodes);
    this->node_segments_.reserve(num_nodes);
    this->node_num_non_configurable_in_edges_.reserve(num_nodes);
    this->node_num_non_configurable_out_edges_.reserve(num_nodes);

    /* Edge-relate vectors */
    this->node_in_edges_.reserve(num_nodes);
    this->node_out_edges_.reserve(num_nodes);
}

/* Reserve a list of edges */
void RRGraph::reserve_edges(const int& num_edges) {
    /* Reserve the full set of vectors related to edges */
    this->edge_ids_.reserve(num_edges);
    this->edge_src_nodes_.reserve(num_edges);
    this->edge_sink_nodes_.reserve(num_edges);
    this->edge_switches_.reserve(num_edges);
}

/* Reserve a list of switches */
void RRGraph::reserve_switches(const int& num_switches) {
    this->switch_ids_.reserve(num_switches);
    this->switches_.reserve(num_switches);
}

/* Reserve a list of segments */
void RRGraph::reserve_segments(const int& num_segments) {
    this->segment_ids_.reserve(num_segments);
    this->segments_.reserve(num_segments);
}

/* Mutators */
RRNodeId RRGraph::create_node(const t_rr_type& type) {
    //Allocate an ID
    RRNodeId node_id = RRNodeId(node_ids_.size());

    //Initialize the attributes
    node_ids_.push_back(node_id);
    node_types_.push_back(type);

    node_bounding_boxes_.emplace_back(-1, -1, -1, -1);

    node_capacities_.push_back(-1);
    node_ptc_nums_.push_back(-1);
    node_cost_indices_.push_back(-1);
    node_directions_.push_back(NO_DIRECTION);
    node_sides_.push_back(NUM_SIDES);
    node_Rs_.push_back(0.);
    node_Cs_.push_back(0.);

    node_in_edges_.emplace_back();  //Initially empty
    node_out_edges_.emplace_back(); //Initially empty

    node_num_non_configurable_in_edges_.emplace_back();  //Initially empty
    node_num_non_configurable_out_edges_.emplace_back(); //Initially empty

    invalidate_fast_node_lookup();

    VTR_ASSERT(validate_sizes());

    return node_id;
}

RREdgeId RRGraph::create_edge(const RRNodeId& source, const RRNodeId& sink, const RRSwitchId& switch_id) {
    VTR_ASSERT(valid_node_id(source));
    VTR_ASSERT(valid_node_id(sink));
    VTR_ASSERT(valid_switch_id(switch_id));

    //Allocate an ID
    RREdgeId edge_id = RREdgeId(edge_ids_.size());

    //Initialize the attributes
    edge_ids_.push_back(edge_id);

    edge_src_nodes_.push_back(source);
    edge_sink_nodes_.push_back(sink);
    edge_switches_.push_back(switch_id);

    //Add the edge to the nodes
    node_out_edges_[source].push_back(edge_id);
    node_in_edges_[sink].push_back(edge_id);

    VTR_ASSERT(validate_sizes());

    return edge_id;
}

RRSwitchId RRGraph::create_switch(const t_rr_switch_inf& switch_info) {
    //Allocate an ID
    RRSwitchId switch_id = RRSwitchId(switch_ids_.size());
    switch_ids_.push_back(switch_id);

    switches_.push_back(switch_info);

    return switch_id;
}

/* Create segment */
RRSegmentId RRGraph::create_segment(const t_segment_inf& segment_info) {
    //Allocate an ID
    RRSegmentId segment_id = RRSegmentId(segment_ids_.size());
    segment_ids_.push_back(segment_id);

    segments_.push_back(segment_info);

    return segment_id;
}

/* This function just marks the node to be removed with an INVALID id 
 * It also disconnects and mark the incoming and outcoming edges to be INVALID()
 * And then set the RRGraph as polluted (dirty_flag = true)
 * The compress() function should be called to physically remove the node 
 */
void RRGraph::remove_node(const RRNodeId& node) {
    //Invalidate all connected edges
    // TODO: consider removal of self-loop edges?
    for (auto edge : node_in_edges(node)) {
        remove_edge(edge);
    }
    for (auto edge : node_out_edges(node)) {
        remove_edge(edge);
    }

    //Mark node invalid
    node_ids_[node] = RRNodeId::INVALID();

    //Invalidate the node look-up
    invalidate_fast_node_lookup();

    set_dirty();
}

/* This function just marks the edge to be removed with an INVALID id 
 * And then set the RRGraph as polluted (dirty_flag = true)
 * The compress() function should be called to physically remove the edge 
 */
void RRGraph::remove_edge(const RREdgeId& edge) {
    RRNodeId src_node = edge_src_node(edge);
    RRNodeId sink_node = edge_sink_node(edge);

    //Invalidate node to edge references
    // TODO: consider making this optional (e.g. if called from remove_node)
    for (size_t i = 0; i < node_out_edges_[src_node].size(); ++i) {
        if (node_out_edges_[src_node][i] == edge) {
            node_out_edges_[src_node][i] = RREdgeId::INVALID();
            break;
        }
    }
    for (size_t i = 0; i < node_in_edges_[sink_node].size(); ++i) {
        if (node_in_edges_[sink_node][i] == edge) {
            node_in_edges_[sink_node][i] = RREdgeId::INVALID();
            break;
        }
    }

    //Mark edge invalid
    edge_ids_[edge] = RREdgeId::INVALID();

    set_dirty();
}

void RRGraph::set_node_xlow(const RRNodeId& node, const short& xlow) {
    VTR_ASSERT(valid_node_id(node));

    node_bounding_boxes_[node].set_xmin(xlow);
}

void RRGraph::set_node_ylow(const RRNodeId& node, const short& ylow) {
    VTR_ASSERT(valid_node_id(node));

    node_bounding_boxes_[node].set_ymin(ylow);
}

void RRGraph::set_node_xhigh(const RRNodeId& node, const short& xhigh) {
    VTR_ASSERT(valid_node_id(node));

    node_bounding_boxes_[node].set_xmax(xhigh);
}

void RRGraph::set_node_yhigh(const RRNodeId& node, const short& yhigh) {
    VTR_ASSERT(valid_node_id(node));

    node_bounding_boxes_[node].set_ymax(yhigh);
}

void RRGraph::set_node_bounding_box(const RRNodeId& node, const vtr::Rect<short>& bb) {
    VTR_ASSERT(valid_node_id(node));

    node_bounding_boxes_[node] = bb;
}

void RRGraph::set_node_capacity(const RRNodeId& node, const short& capacity) {
    VTR_ASSERT(valid_node_id(node));

    node_capacities_[node] = capacity;
}

void RRGraph::set_node_ptc_num(const RRNodeId& node, const short& ptc) {
    VTR_ASSERT(valid_node_id(node));

    node_ptc_nums_[node] = ptc;
}

void RRGraph::set_node_pin_num(const RRNodeId& node, const short& pin_id) {
    VTR_ASSERT(valid_node_id(node));
    VTR_ASSERT_MSG(node_type(node) == IPIN || node_type(node) == OPIN, "Pin number valid only for IPIN/OPIN RR nodes");

    set_node_ptc_num(node, pin_id);
}

void RRGraph::set_node_track_num(const RRNodeId& node, const short& track_id) {
    VTR_ASSERT(valid_node_id(node));
    VTR_ASSERT_MSG(node_type(node) == CHANX || node_type(node) == CHANY, "Track number valid only for CHANX/CHANY RR nodes");

    set_node_ptc_num(node, track_id);
}

void RRGraph::set_node_class_num(const RRNodeId& node, const short& class_id) {
    VTR_ASSERT(valid_node_id(node));
    VTR_ASSERT_MSG(node_type(node) == SOURCE || node_type(node) == SINK, "Class number valid only for SOURCE/SINK RR nodes");

    set_node_ptc_num(node, class_id);
}

void RRGraph::set_node_cost_index(const RRNodeId& node, const short& cost_index) {
    VTR_ASSERT(valid_node_id(node));
    node_cost_indices_[node] = cost_index;
}

void RRGraph::set_node_direction(const RRNodeId& node, const e_direction& direction) {
    VTR_ASSERT(valid_node_id(node));
    VTR_ASSERT_MSG(node_type(node) == CHANX || node_type(node) == CHANY, "Direct can only be specified on CHANX/CNAY rr nodes");

    node_directions_[node] = direction;
}

void RRGraph::set_node_side(const RRNodeId& node, const e_side& side) {
    VTR_ASSERT(valid_node_id(node));
    VTR_ASSERT_MSG(node_type(node) == IPIN || node_type(node) == OPIN, "Side can only be specified on IPIN/OPIN rr nodes");

    node_sides_[node] = side;
}

void RRGraph::set_node_R(const RRNodeId& node, const float& R) {
    VTR_ASSERT(valid_node_id(node));

    node_Rs_[node] = R;
}

void RRGraph::set_node_C(const RRNodeId& node, const float& C) {
    VTR_ASSERT(valid_node_id(node));

    node_Cs_[node] = C;
}

/*
 * Set a segment id for a node in rr_graph 
 */
void RRGraph::set_node_segment(const RRNodeId& node, const RRSegmentId& segment_id) {
    VTR_ASSERT(valid_node_id(node));

    /* Only CHANX and CHANY requires a valid segment id */
    if ((CHANX == node_type(node))
        || (CHANY == node_type(node))) {
        VTR_ASSERT(valid_segment_id(segment_id));
    }

    node_segments_[node] = segment_id;
}

/* For a given node in a rr_graph
 * classify the edges of each node to be configurable (1st part) and non-configurable (2nd part) 
 */
void RRGraph::partition_node_in_edges(const RRNodeId& node) {
    //Partition the edges so the first set of edges are all configurable, and the later are not
    auto first_non_config_edge = std::partition(node_in_edges_[node].begin(), node_in_edges_[node].end(),
                                                [&](const RREdgeId edge) { return edge_is_configurable(edge); }); /* Condition to partition edges */

    size_t num_conf_edges = std::distance(node_in_edges_[node].begin(), first_non_config_edge);
    size_t num_non_conf_edges = node_in_edges_[node].size() - num_conf_edges; //Note we calculate using the size_t to get full range

    /* Check that within allowable range (no overflow when stored as num_non_configurable_edges_
     */
    VTR_ASSERT_MSG(num_non_conf_edges <= node_in_edges_[node].size(),
                   "Exceeded RR node maximum number of non-configurable input edges");

    node_num_non_configurable_in_edges_[node] = num_non_conf_edges; //Narrowing
}

/* For a given node in a rr_graph
 * classify the edges of each node to be configurable (1st part) and non-configurable (2nd part) 
 */
void RRGraph::partition_node_out_edges(const RRNodeId& node) {
    //Partition the edges so the first set of edges are all configurable, and the later are not
    auto first_non_config_edge = std::partition(node_out_edges_[node].begin(), node_out_edges_[node].end(),
                                                [&](const RREdgeId edge) { return edge_is_configurable(edge); }); /* Condition to partition edges */

    size_t num_conf_edges = std::distance(node_out_edges_[node].begin(), first_non_config_edge);
    size_t num_non_conf_edges = node_out_edges_[node].size() - num_conf_edges; //Note we calculate using the size_t to get full range

    /* Check that within allowable range (no overflow when stored as num_non_configurable_edges_
     */
    VTR_ASSERT_MSG(num_non_conf_edges <= node_out_edges_[node].size(),
                   "Exceeded RR node maximum number of non-configurable output edges");

    node_num_non_configurable_out_edges_[node] = num_non_conf_edges; //Narrowing
}

/* For all nodes in a rr_graph  
 * classify the input edges of each node to be configurable (1st part) and non-configurable (2nd part) 
 */
void RRGraph::partition_in_edges() {
    /* For each node */
    for (auto node : nodes()) {
        this->partition_node_in_edges(node);
    }
}

/* For all nodes in a rr_graph  
 * classify the output edges of each node to be configurable (1st part) and non-configurable (2nd part) 
 */
void RRGraph::partition_out_edges() {
    /* For each node */
    for (auto node : nodes()) {
        this->partition_node_out_edges(node);
    }
}

/* For all nodes in a rr_graph  
 * classify both input and output edges of each node 
 * to be configurable (1st part) and non-configurable (2nd part) 
 */
void RRGraph::partition_edges() {
    /* Partition input edges */
    this->partition_in_edges();
    /* Partition output edges */
    this->partition_out_edges();
}

void RRGraph::build_fast_node_lookup() const {
    invalidate_fast_node_lookup();

    for (auto node : nodes()) {
        size_t x = node_xlow(node);
        if (x >= node_lookup_.size()) {
            node_lookup_.resize(x + 1);
        }

        size_t y = node_ylow(node);
        if (y >= node_lookup_[x].size()) {
            node_lookup_[x].resize(y + 1);
        }

        size_t itype = node_type(node);
        if (itype >= node_lookup_[x][y].size()) {
            node_lookup_[x][y].resize(itype + 1);
        }

        size_t ptc = node_ptc_num(node);
        if (ptc >= node_lookup_[x][y][itype].size()) {
            node_lookup_[x][y][itype].resize(ptc + 1);
        }

        size_t iside = -1;
        if (node_type(node) == OPIN || node_type(node) == IPIN) {
            iside = node_side(node);
        } else {
            iside = NUM_SIDES;
        }

        if (iside >= node_lookup_[x][y][itype][ptc].size()) {
            node_lookup_[x][y][itype][ptc].resize(iside + 1);
        }

        //Save node in lookup
        node_lookup_[x][y][itype][ptc][iside] = node;
    }
}

void RRGraph::invalidate_fast_node_lookup() const {
    node_lookup_.clear();
}

bool RRGraph::valid_fast_node_lookup() const {
    return !node_lookup_.empty();
}

void RRGraph::initialize_fast_node_lookup() const {
    if (!valid_fast_node_lookup()) {
        build_fast_node_lookup();
    }
}

bool RRGraph::valid_node_id(const RRNodeId& node) const {
    return size_t(node) < node_ids_.size() && node_ids_[node] == node;
}

bool RRGraph::valid_edge_id(const RREdgeId& edge) const {
    return size_t(edge) < edge_ids_.size() && edge_ids_[edge] == edge;
}

/* check if a given switch id is valid or not */
bool RRGraph::valid_switch_id(const RRSwitchId& switch_id) const {
    /* TODO: should we check the index of switch[id] matches ? */
    return size_t(switch_id) < switches_.size();
}

/* check if a given segment id is valid or not */
bool RRGraph::valid_segment_id(const RRSegmentId& segment_id) const {
    /* TODO: should we check the index of segment[id] matches ? */
    return size_t(segment_id) < segments_.size();
}

/**
 * Internal checking codes to ensure data consistency
 * If you add any internal data to RRGraph and update create_node/edge etc. 
 * you need to update the validate_sizes() here to make sure these
 * internal vectors are aligned to the id vectors
 */
bool RRGraph::validate_sizes() const {
    return validate_node_sizes()
           && validate_edge_sizes()
           && validate_switch_sizes()
           && validate_segment_sizes();
}

bool RRGraph::validate_node_sizes() const {
    return node_types_.size() == node_ids_.size()
           && node_bounding_boxes_.size() == node_ids_.size()
           && node_capacities_.size() == node_ids_.size()
           && node_ptc_nums_.size() == node_ids_.size()
           && node_cost_indices_.size() == node_ids_.size()
           && node_directions_.size() == node_ids_.size()
           && node_sides_.size() == node_ids_.size()
           && node_Rs_.size() == node_ids_.size()
           && node_Cs_.size() == node_ids_.size()
           && node_segments_.size() == node_ids_.size()
           && node_num_non_configurable_in_edges_.size() == node_ids_.size()
           && node_num_non_configurable_out_edges_.size() == node_ids_.size()
           && node_in_edges_.size() == node_ids_.size()
           && node_out_edges_.size() == node_ids_.size();
}

bool RRGraph::validate_edge_sizes() const {
    return edge_src_nodes_.size() == edge_ids_.size()
           && edge_sink_nodes_.size() == edge_ids_.size()
           && edge_switches_.size() == edge_ids_.size();
}

bool RRGraph::validate_switch_sizes() const {
    return switches_.size() == switch_ids_.size();
}

bool RRGraph::validate_segment_sizes() const {
    return segments_.size() == segment_ids_.size();
}

void RRGraph::compress() {
    vtr::vector<RRNodeId, RRNodeId> node_id_map(node_ids_.size());
    vtr::vector<RREdgeId, RREdgeId> edge_id_map(edge_ids_.size());

    build_id_maps(node_id_map, edge_id_map);

    clean_nodes(node_id_map);
    clean_edges(edge_id_map);

    rebuild_node_refs(edge_id_map);

    invalidate_fast_node_lookup();

    clear_dirty();
}

void RRGraph::build_id_maps(vtr::vector<RRNodeId, RRNodeId>& node_id_map,
                            vtr::vector<RREdgeId, RREdgeId>& edge_id_map) {
    node_id_map = compress_ids(node_ids_);
    edge_id_map = compress_ids(edge_ids_);
}

void RRGraph::clean_nodes(const vtr::vector<RRNodeId, RRNodeId>& node_id_map) {
    node_ids_ = clean_and_reorder_ids(node_id_map);

    node_types_ = clean_and_reorder_values(node_types_, node_id_map);

    node_bounding_boxes_ = clean_and_reorder_values(node_bounding_boxes_, node_id_map);

    node_capacities_ = clean_and_reorder_values(node_capacities_, node_id_map);
    node_ptc_nums_ = clean_and_reorder_values(node_ptc_nums_, node_id_map);
    node_cost_indices_ = clean_and_reorder_values(node_cost_indices_, node_id_map);
    node_directions_ = clean_and_reorder_values(node_directions_, node_id_map);
    node_sides_ = clean_and_reorder_values(node_sides_, node_id_map);
    node_Rs_ = clean_and_reorder_values(node_Rs_, node_id_map);
    node_Cs_ = clean_and_reorder_values(node_Cs_, node_id_map);

    VTR_ASSERT(validate_node_sizes());

    VTR_ASSERT_MSG(are_contiguous(node_ids_), "Ids should be contiguous");
    VTR_ASSERT_MSG(all_valid(node_ids_), "All Ids should be valid");
}

void RRGraph::clean_edges(const vtr::vector<RREdgeId, RREdgeId>& edge_id_map) {
    edge_ids_ = clean_and_reorder_ids(edge_id_map);

    edge_src_nodes_ = clean_and_reorder_values(edge_src_nodes_, edge_id_map);
    edge_sink_nodes_ = clean_and_reorder_values(edge_sink_nodes_, edge_id_map);
    edge_switches_ = clean_and_reorder_values(edge_switches_, edge_id_map);

    VTR_ASSERT(validate_edge_sizes());

    VTR_ASSERT_MSG(are_contiguous(edge_ids_), "Ids should be contiguous");
    VTR_ASSERT_MSG(all_valid(edge_ids_), "All Ids should be valid");
}

void RRGraph::rebuild_node_refs(const vtr::vector<RREdgeId, RREdgeId>& edge_id_map) {
    for (const auto& node : nodes()) {
        node_in_edges_[node] = update_valid_refs(node_in_edges_[node], edge_id_map);
        node_out_edges_[node] = update_valid_refs(node_out_edges_[node], edge_id_map);

        VTR_ASSERT_MSG(all_valid(node_in_edges_[node]), "All Ids should be valid");
        VTR_ASSERT_MSG(all_valid(node_out_edges_[node]), "All Ids should be valid");
    }
}

/* Empty all the vectors related to nodes */
void RRGraph::clear_nodes() {
    node_ids_.clear();

    node_types_.clear();
    node_bounding_boxes_.clear();

    node_capacities_.clear();
    node_ptc_nums_.clear();
    node_cost_indices_.clear();
    node_directions_.clear();
    node_sides_.clear();
    node_Rs_.clear();
    node_Cs_.clear();
    node_segments_.clear();

    node_num_non_configurable_in_edges_.clear();
    node_num_non_configurable_out_edges_.clear();

    node_in_edges_.clear();
    node_out_edges_.clear();

    /* clean node_look_up */
    node_lookup_.clear();
}

/* Empty all the vectors related to edges */
void RRGraph::clear_edges() {
    edge_ids_.clear();
    edge_src_nodes_.clear();
    edge_sink_nodes_.clear();
    edge_switches_.clear();
}

/* Empty all the vectors related to switches */
void RRGraph::clear_switches() {
    switch_ids_.clear();
    switches_.clear();
}

/* Empty all the vectors related to segments */
void RRGraph::clear_segments() {
    segment_ids_.clear();
    segments_.clear();
}

/* Clean the rr_graph */
void RRGraph::clear() {
    clear_nodes();
    clear_edges();
    clear_switches();
    clear_segments();
}
