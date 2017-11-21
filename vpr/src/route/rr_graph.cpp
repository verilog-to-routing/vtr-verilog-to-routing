#include "rr_graph.h"
#include "graph_utils.h"
#include "vtr_util.h"
#include "vpr_error.h"
#include <algorithm>

//Accessors
RRGraph::node_range RRGraph::nodes() const {
    return vtr::make_range(node_ids_.begin(), node_ids_.end());
}

RRGraph::edge_range RRGraph::edges() const {
    return vtr::make_range(edge_ids_.begin(), edge_ids_.end());
}

//Node attributes
t_rr_type RRGraph::node_type(RRNodeId node) const {
    VTR_ASSERT(valid_node_id(node));
    return node_types_[node];
}

short RRGraph::node_xlow(RRNodeId node) const {
    return node_bounding_box(node).xmin();
}

short RRGraph::node_ylow(RRNodeId node) const {
    return node_bounding_box(node).ymin();
}

short RRGraph::node_xhigh(RRNodeId node) const {
    return node_bounding_box(node).xmax();
}

short RRGraph::node_yhigh(RRNodeId node) const {
    return node_bounding_box(node).ymax();
}

short RRGraph::node_length(RRNodeId node) const {
    return std::max(node_xhigh(node) - node_xlow(node), node_yhigh(node) - node_ylow(node));
}

vtr::Rect<short> RRGraph::node_bounding_box(RRNodeId node) const {
    VTR_ASSERT(valid_node_id(node));
    return node_bounding_boxes_[node];
}

short RRGraph::node_fan_in(RRNodeId node) const {
    return node_in_edges(node).size();
}

short RRGraph::node_fan_out(RRNodeId node) const {
    return node_out_edges(node).size();
}

short RRGraph::node_capacity(RRNodeId node) const {
    VTR_ASSERT(valid_node_id(node));
    return node_capacities_[node];
}

short RRGraph::node_ptc_num(RRNodeId node) const {
    VTR_ASSERT(valid_node_id(node));
    return node_ptc_nums_[node];
}

short RRGraph::node_pin_num(RRNodeId node) const {
    VTR_ASSERT_MSG(node_type(node) == IPIN || node_type(node) == OPIN, "Pin number valid only for IPIN/OPIN RR nodes");
    return node_ptc_num(node);
}

short RRGraph::node_track_num(RRNodeId node) const {
    VTR_ASSERT_MSG(node_type(node) == CHANX || node_type(node) == CHANY, "Track number valid only for CHANX/CHANY RR nodes");
    return node_ptc_num(node);
}

short RRGraph::node_class_num(RRNodeId node) const {
    VTR_ASSERT_MSG(node_type(node) == SOURCE || node_type(node) == SINK, "Class number valid only for SOURCE/SINK RR nodes");
    return node_ptc_num(node);
}

short RRGraph::node_cost_index(RRNodeId node) const {
    VTR_ASSERT(valid_node_id(node));
    return node_cost_indices_[node];
}

e_direction RRGraph::node_direction(RRNodeId node) const {
    VTR_ASSERT_MSG(node_type(node) == CHANX || node_type(node) == CHANY, "Direction valid only for SOURCE/SINK RR nodes");
    return node_directions_[node];
}

e_side RRGraph::node_side(RRNodeId node) const {
    VTR_ASSERT_MSG(node_type(node) == IPIN || node_type(node) == OPIN, "Direction valid only for IPIN/OPIN RR nodes");
    return node_sides_[node];
}

float RRGraph::node_R(RRNodeId node) const {
    VTR_ASSERT(valid_node_id(node));
    return node_Rs_[node];
}

float RRGraph::node_C(RRNodeId node) const {
    VTR_ASSERT(valid_node_id(node));
    return node_Cs_[node];
}

RRGraph::edge_range RRGraph::node_out_edges(RRNodeId node) const {
    VTR_ASSERT(valid_node_id(node));
    return vtr::make_range(node_out_edges_[node].begin(), node_out_edges_[node].end());
}

RRGraph::edge_range RRGraph::node_in_edges(RRNodeId node) const {
    VTR_ASSERT(valid_node_id(node));
    return vtr::make_range(node_in_edges_[node].begin(), node_in_edges_[node].end());
}

//Edge attributes
RRNodeId RRGraph::edge_src_node(RREdgeId edge) const {
    VTR_ASSERT(valid_edge_id(edge));
    return edge_src_nodes_[edge];
}
RRNodeId RRGraph::edge_sink_node(RREdgeId edge) const {
    VTR_ASSERT(valid_edge_id(edge));
    return edge_sink_nodes_[edge];
}

RRSwitchId RRGraph::edge_switch(RREdgeId edge) const {
    VTR_ASSERT(valid_edge_id(edge));
    return edge_switches_[edge];
}

RREdgeId RRGraph::find_edge(RRNodeId src_node, RRNodeId sink_node) const {
    for (auto edge : node_out_edges(src_node)) {
        if (edge_sink_node(edge) == sink_node) {
            return edge;
        }
    }

    //Not found
    return RREdgeId::INVALID();
}


//Mutators
RRNodeId RRGraph::create_node(t_rr_type type) {
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

    node_in_edges_.emplace_back(); //Initially empty
    node_out_edges_.emplace_back(); //Initially empty

    VTR_ASSERT(validate_sizes());

    return node_id;
}

RREdgeId RRGraph::create_edge(RRNodeId source, RRNodeId sink, RRSwitchId switch_id) {
    VTR_ASSERT(valid_node_id(source));
    VTR_ASSERT(valid_node_id(sink));

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

RRSwitchId RRGraph::create_switch(t_rr_switch_inf switch_info) {
    //Allocate an ID
    RRSwitchId switch_id = RRSwitchId(switch_ids_.size());
    switch_ids_.push_back(switch_id);

    switches_.push_back(switch_info);

    return switch_id;
}

void RRGraph::remove_node(RRNodeId node) {
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

    dirty_ = true;
}

void RRGraph::remove_edge(RREdgeId edge) {
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

    dirty_ = true;
}

void RRGraph::set_node_xlow(RRNodeId node, short xlow) {
    VTR_ASSERT(valid_node_id(node));

    auto& orig_bb = node_bounding_boxes_[node];
    node_bounding_boxes_[node] = vtr::Rect<short>(xlow, orig_bb.ymin(), orig_bb.xmax(), orig_bb.ymax());
}

void RRGraph::set_node_ylow(RRNodeId node, short ylow) {
    VTR_ASSERT(valid_node_id(node));

    auto& orig_bb = node_bounding_boxes_[node];
    node_bounding_boxes_[node] = vtr::Rect<short>(orig_bb.xmin(), ylow, orig_bb.xmax(), orig_bb.ymax());
}

void RRGraph::set_node_xhigh(RRNodeId node, short xhigh) {
    VTR_ASSERT(valid_node_id(node));

    auto& orig_bb = node_bounding_boxes_[node];
    node_bounding_boxes_[node] = vtr::Rect<short>(orig_bb.xmin(), orig_bb.ymin(), xhigh, orig_bb.ymax());
}

void RRGraph::set_node_yhigh(RRNodeId node, short yhigh) {
    VTR_ASSERT(valid_node_id(node));

    auto& orig_bb = node_bounding_boxes_[node];
    node_bounding_boxes_[node] = vtr::Rect<short>(orig_bb.xmin(), orig_bb.ymin(), orig_bb.xmax(), yhigh);
}

void RRGraph::set_node_bounding_box(RRNodeId node, vtr::Rect<short> bb) {
    VTR_ASSERT(valid_node_id(node));
    
    node_bounding_boxes_[node] = bb;
}

void RRGraph::set_node_capacity(RRNodeId node, short capacity) {
    VTR_ASSERT(valid_node_id(node));

    node_capacities_[node] = capacity;
}


void RRGraph::set_node_ptc_num(RRNodeId node, short ptc) {
    VTR_ASSERT(valid_node_id(node));

    node_ptc_nums_[node] = ptc;
}

void RRGraph::set_node_pin_num(RRNodeId node, short pin_id) {
    VTR_ASSERT(valid_node_id(node));
    VTR_ASSERT_MSG(node_type(node) == IPIN || node_type(node) == OPIN, "Pin number valid only for IPIN/OPIN RR nodes");

    set_node_ptc_num(node, pin_id);
}

void RRGraph::set_node_track_num(RRNodeId node, short track_id) {
    VTR_ASSERT(valid_node_id(node));
    VTR_ASSERT_MSG(node_type(node) == CHANX || node_type(node) == CHANY, "Track number valid only for CHANX/CHANY RR nodes");

    set_node_ptc_num(node, track_id);
}

void RRGraph::set_node_class_num(RRNodeId node, short class_id) {
    VTR_ASSERT(valid_node_id(node));
    VTR_ASSERT_MSG(node_type(node) == SOURCE || node_type(node) == SINK, "Class number valid only for SOURCE/SINK RR nodes");

    set_node_ptc_num(node, class_id);
}


void RRGraph::set_node_cost_index(RRNodeId node, short cost_index) {
    VTR_ASSERT(valid_node_id(node));
    node_cost_indices_[node] = cost_index;
}

void RRGraph::set_node_direction(RRNodeId node, e_direction direction) {
    VTR_ASSERT(valid_node_id(node));
    VTR_ASSERT_MSG(node_type(node) == CHANX || node_type(node) == CHANY, "Direct can only be specified on CHANX/CNAY rr nodes");

    node_directions_[node] = direction;
}

void RRGraph::set_node_side(RRNodeId node, e_side side) {
    VTR_ASSERT(valid_node_id(node));
    VTR_ASSERT_MSG(node_type(node) == IPIN || node_type(node) == OPIN, "Side can only be specified on IPIN/OPIN rr nodes");

    node_sides_[node] = side;
}

void RRGraph::set_node_R(RRNodeId node, float R) {
    VTR_ASSERT(valid_node_id(node));

    node_Rs_[node] = R;
}

void RRGraph::set_node_C(RRNodeId node, float C) {
    VTR_ASSERT(valid_node_id(node));

    node_Cs_[node] = C;
}

bool RRGraph::validate() {
    bool success = true;
    success &= validate_sizes();
    success &= validate_crossrefs();
    success &= validate_invariants();

    return success;
}

bool RRGraph::valid_node_id(RRNodeId node) const {
    return size_t(node) < node_ids_.size() && node_ids_[node] == node;
}

bool RRGraph::valid_edge_id(RREdgeId edge) const {
    return size_t(edge) < edge_ids_.size() && edge_ids_[edge] == edge;
}

bool RRGraph::validate_sizes() const {
    return validate_node_sizes() && validate_edge_sizes();
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
        && node_in_edges_.size() == node_ids_.size()
        && node_out_edges_.size() == node_ids_.size();
}

bool RRGraph::validate_edge_sizes() const {
    return edge_src_nodes_.size() == edge_ids_.size()
        && edge_sink_nodes_.size() == edge_ids_.size()
        && edge_switches_.size() == edge_ids_.size();
}

bool RRGraph::validate_crossrefs() const {
    bool success = true;

    success &= validate_node_edge_crossrefs();

    return success;

}

bool RRGraph::validate_node_edge_crossrefs() const {

    //Check Forward (node to edge)
    for (auto node : nodes()) {
        //For each node edge check that the source/sink are consistent
        for (auto edge : node_in_edges(node)) {
            auto sink_node = edge_sink_node(edge);
            if (sink_node != node) {
                std::string msg = vtr::string_fmt("Mismatched node in-edge cross referrence: RR Edge %zu, RR Node != RR Edge Sink: (%zu != %zu)", size_t(edge), size_t(node), size_t(sink_node));
                VPR_THROW(VPR_ERROR_RR_GRAPH, msg.c_str());
            }
        }

        for (auto edge : node_out_edges(node)) {
            auto src_node = edge_src_node(edge);
            if (src_node != node) {
                std::string msg = vtr::string_fmt("Mismatched node out-edge cross referrence: RR Edge %zu, RR Node != RR Edge Source: (%zu != %zu)", size_t(edge), size_t(node), size_t(src_node));
                VPR_THROW(VPR_ERROR_RR_GRAPH, msg.c_str());
            }
        }
    }

    //Check Backward (edge to node)
    for (auto edge : edges()) {
        //For each edge, check that the edge exists in the appropriate node in/out edge list
        auto src_node = edge_src_node(edge);
        auto sink_node = edge_sink_node(edge);
        
        auto src_out_edges = node_out_edges(src_node);
        auto sink_in_edges = node_in_edges(sink_node);

        auto itr = std::find(src_out_edges.begin(), src_out_edges.end(), edge);
        if (itr == src_out_edges.end()) { //Not found
            std::string msg = vtr::string_fmt("Mismatched edge node cross reference:"
                                              " RR Edge %zu not found with associated source RR Node %zu",
                                              size_t(edge), size_t(src_node));
            VPR_THROW(VPR_ERROR_RR_GRAPH, msg.c_str());
        }

        itr = std::find(sink_in_edges.begin(), sink_in_edges.end(), edge);
        if (itr == sink_in_edges.end()) { //Not found
            std::string msg = vtr::string_fmt("Mismatched edge node cross reference:"
                                              " RR Edge %zu not found with associated sink RR Node %zu",
                                              size_t(edge), size_t(sink_node));
            VPR_THROW(VPR_ERROR_RR_GRAPH, msg.c_str());
        }
    }

    return true;
}

bool RRGraph::validate_invariants() const {
    bool success = true;

    success &= validate_unique_edges_invariant();

    return success;
}

bool RRGraph::validate_unique_edges_invariant() const {
    //Check that there is only ever a single (unidirectional) edge between two nodes
    std::map<std::pair<RRNodeId,RRNodeId>, std::vector<RREdgeId>> edge_map;
    for (auto edge : edges()) {
        auto src_node = edge_src_node(edge);
        auto sink_node = edge_sink_node(edge);

        //Save the edge
        edge_map[std::make_pair(src_node, sink_node)].push_back(edge);
    }

    //Report any multiple edges
    for (auto kv : edge_map) {
        if (kv.second.size() > 1) {

            std::string msg = vtr::string_fmt("Multiple edges found from RR Node %zu -> %zu", 
                                              size_t(kv.first.first), size_t(kv.first.second));

            msg += " (Edges: ";
            auto mult_edges = kv.second;
            for (size_t i = 0; i < mult_edges.size(); ++i) {
                msg += std::to_string(size_t(mult_edges[i]));
                if (i != mult_edges.size() - 1) {
                    msg += ", ";
                }
            }
            msg += ")";

            VPR_THROW(VPR_ERROR_RR_GRAPH, msg.c_str());
        }
    }

    return true;
}

void RRGraph::compress() {
    vtr::vector<RRNodeId,RRNodeId> node_id_map(node_ids_.size());
    vtr::vector<RREdgeId,RREdgeId> edge_id_map(edge_ids_.size());

    build_id_maps(node_id_map, edge_id_map);

    clean_nodes(node_id_map);
    clean_edges(edge_id_map);

    rebuild_node_refs(edge_id_map);

    dirty_ = false;
}

void RRGraph::build_id_maps(vtr::vector<RRNodeId,RRNodeId>& node_id_map,
                            vtr::vector<RREdgeId,RREdgeId>& edge_id_map) {
    node_id_map = compress_ids(node_ids_);
    edge_id_map = compress_ids(edge_ids_);
}

void RRGraph::clean_nodes(const vtr::vector<RRNodeId,RRNodeId>& node_id_map) {
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

void RRGraph::clean_edges(const vtr::vector<RREdgeId,RREdgeId>& edge_id_map) {
    edge_ids_ = clean_and_reorder_ids(edge_id_map);

    edge_src_nodes_ = clean_and_reorder_values(edge_src_nodes_, edge_id_map);
    edge_sink_nodes_ = clean_and_reorder_values(edge_sink_nodes_, edge_id_map);
    edge_switches_ = clean_and_reorder_values(edge_switches_, edge_id_map);

    VTR_ASSERT(validate_edge_sizes());

    VTR_ASSERT_MSG(are_contiguous(edge_ids_), "Ids should be contiguous");
    VTR_ASSERT_MSG(all_valid(edge_ids_), "All Ids should be valid");
}

void RRGraph::rebuild_node_refs(const vtr::vector<RREdgeId,RREdgeId>& edge_id_map) {
    for (const auto& node : nodes()) {
        node_in_edges_[node] = update_valid_refs(node_in_edges_[node], edge_id_map);
        node_out_edges_[node] = update_valid_refs(node_out_edges_[node], edge_id_map);

        VTR_ASSERT_MSG(all_valid(node_in_edges_[node]), "All Ids should be valid");
        VTR_ASSERT_MSG(all_valid(node_out_edges_[node]), "All Ids should be valid");
    }
}

