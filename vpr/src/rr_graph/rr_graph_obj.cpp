/*
 * Member Functions of rr_node,
 * include mutators, accessors and utility functions 
 */
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
t_rr_type RRGraph::node_type(RRNodeId node) const {
  VTR_ASSERT_SAFE(valid_node_id(node));
  return node_types_[node];
}

size_t RRGraph::node_index(RRNodeId node) const {
  VTR_ASSERT_SAFE(valid_node_id(node));
  return size_t(node); 
}

const char* RRGraph::node_type_string(RRNodeId node) const {
  VTR_ASSERT_SAFE(valid_node_id(node));
  return rr_node_typename[node_type(node)];
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
  VTR_ASSERT_SAFE(valid_node_id(node));
  return node_bounding_boxes_[node];
}

short RRGraph::node_fan_in(RRNodeId node) const {
  return node_in_edges(node).size();
}

short RRGraph::node_fan_out(RRNodeId node) const {
  return node_out_edges(node).size();
}

short RRGraph::node_capacity(RRNodeId node) const {
  VTR_ASSERT_SAFE(valid_node_id(node));
  return node_capacities_[node];
}

short RRGraph::node_ptc_num(RRNodeId node) const {
  VTR_ASSERT_SAFE(valid_node_id(node));
  return node_ptc_nums_[node];
}

short RRGraph::node_pin_num(RRNodeId node) const {
  VTR_ASSERT_MSG(node_type(node) == IPIN || node_type(node) == OPIN, 
             "Pin number valid only for IPIN/OPIN RR nodes");
  return node_ptc_num(node);
}

short RRGraph::node_track_num(RRNodeId node) const {
  VTR_ASSERT_MSG(node_type(node) == CHANX || node_type(node) == CHANY, 
             "Track number valid only for CHANX/CHANY RR nodes");
  return node_ptc_num(node);
}

short RRGraph::node_class_num(RRNodeId node) const {
  VTR_ASSERT_MSG(node_type(node) == SOURCE || node_type(node) == SINK, "Class number valid only for SOURCE/SINK RR nodes");
  return node_ptc_num(node);
}

short RRGraph::node_cost_index(RRNodeId node) const {
  VTR_ASSERT_SAFE(valid_node_id(node));
  return node_cost_indices_[node];
}

e_direction RRGraph::node_direction(RRNodeId node) const {
  VTR_ASSERT_SAFE(valid_node_id(node));
  VTR_ASSERT_MSG(node_type(node) == CHANX || node_type(node) == CHANY, "Direction valid only for CHANX/CHANY RR nodes");
  return node_directions_[node];
}

const char* RRGraph::node_direction_string(RRNodeId node) const {
  VTR_ASSERT_SAFE(valid_node_id(node));
  VTR_ASSERT_MSG(node_type(node) == CHANX || node_type(node) == CHANY, "Direction valid only for CHANX/CHANY RR nodes");

  if (INC_DIRECTION == node_direction(node)) {
    return "INC_DIR";
  } else if (DEC_DIRECTION == node_direction(node)) {
    return "DEC_DIR";
  } else if (BI_DIRECTION == node_direction(node)) {
    return "BI_DIR";
  }

  VTR_ASSERT(NO_DIRECTION == node_direction(node));
  return "NO_DIR";
}

e_side RRGraph::node_side(RRNodeId node) const {
  VTR_ASSERT_SAFE(valid_node_id(node));
  VTR_ASSERT_MSG(node_type(node) == IPIN || node_type(node) == OPIN, "Direction valid only for IPIN/OPIN RR nodes");
  return node_sides_[node];
}

const char* RRGraph::node_side_string(RRNodeId node) const {
  VTR_ASSERT_SAFE(valid_node_id(node));
  VTR_ASSERT_MSG(node_type(node) == IPIN || node_type(node) == OPIN, "Direction valid only for IPIN/OPIN RR nodes");
  return SIDE_STRING[node_side(node)];
}

/* Get the resistance of a node */
float RRGraph::node_R(RRNodeId node) const {
  VTR_ASSERT_SAFE(valid_node_id(node));
  return node_Rs_[node];
}

/* Get the capacitance of a node */
float RRGraph::node_C(RRNodeId node) const {
  VTR_ASSERT_SAFE(valid_node_id(node));
  return node_Cs_[node];
}

/*
 * Get a segment id of a node in rr_graph 
 */
short RRGraph::node_segment_id(RRNodeId node) const {
  VTR_ASSERT_SAFE(valid_node_id(node));

  return node_segment_ids_[node];
}

/* 
 * Get the number of configurable input edges of a node
 * TODO: we would use the node_num_configurable_in_edges() 
 * when the rr_graph edges have been partitioned 
 * This can avoid unneccessary walkthrough
 */
short RRGraph::node_num_configurable_in_edges(RRNodeId node) const {
  /* Ensure a valid node id */
  VTR_ASSERT_SAFE(valid_node_id(node));

  /* Each all the edges ends at this node */
  short iedge = 0;
  for (auto edge : node_in_edges(node)) {
    if (true == edge_is_configurable(edge)) {
      ++iedge;
    }
  }

  return iedge;
}

/* 
 * Get the number of configurable output edges of a node
 * TODO: we would use the node_num_configurable_out_edges() 
 * when the rr_graph edges have been partitioned 
 * This can avoid unneccessary walkthrough
 */
short RRGraph::node_num_configurable_out_edges(RRNodeId node) const {
  /* Ensure a valid node id */
  VTR_ASSERT_SAFE(valid_node_id(node));

  /* Each all the edges ends at this node */
  short iedge = 0;
  for (auto edge : node_out_edges(node)) {
    if (true == edge_is_configurable(edge)) {
      ++iedge;
    }
  }

  return iedge;
}

/* 
 * Get the number of non-configurable input edges of a node
 * TODO: we would use the node_num_configurable_in_edges() 
 * when the rr_graph edges have been partitioned 
 * This can avoid unneccessary walkthrough
 */
short RRGraph::node_num_non_configurable_in_edges(RRNodeId node) const {
  /* Ensure a valid node id */
  VTR_ASSERT_SAFE(valid_node_id(node));

  /* Each all the edges ends at this node */
  short iedge = 0;
  for (auto edge : node_in_edges(node)) {
    if (true == edge_is_non_configurable(edge)) {
      ++iedge;
    }
  }

  return iedge;
}

/* 
 * Get the number of non-configurable output edges of a node
 * TODO: we would use the node_num_configurable_out_edges() 
 * when the rr_graph edges have been partitioned 
 * This can avoid unneccessary walkthrough
 */
short RRGraph::node_num_non_configurable_out_edges(RRNodeId node) const {
  /* Ensure a valid node id */
  VTR_ASSERT_SAFE(valid_node_id(node));

  /* Each all the edges ends at this node */
  short iedge = 0;
  for (auto edge : node_out_edges(node)) {
    if (true == edge_is_non_configurable(edge)) {
      ++iedge;
    }  
  }

  return iedge;
}

/* Get the list of configurable edges from the input edges of a given node 
 * And return the range(iterators) of the list 
 */
RRGraph::edge_range RRGraph::node_configurable_in_edges(RRNodeId node) const {
  /* Make sure we will access a valid node */
  VTR_ASSERT_SAFE(valid_node_id(node));

  /* By default the configurable edges will be stored at the first part of the edge list (0 to XX) */
  auto begin = node_in_edges(node).begin();  

  /* By default the non-configurable edges will be stored at second part of the edge list (XX to end) */
  auto end = node_in_edges(node).end() -  node_num_non_configurable_in_edges_[node];  

  return vtr::make_range(begin, end); 
}

/* Get the list of configurable edges from the input edges of a given node 
 * And return the range(iterators) of the list 
 */
RRGraph::edge_range RRGraph::node_non_configurable_in_edges(RRNodeId node) const {
  /* Make sure we will access a valid node */
  VTR_ASSERT_SAFE(valid_node_id(node));

  /* By default the configurable edges will be stored at the first part of the edge list (0 to XX) */
  auto begin = node_in_edges(node).begin() + node_num_non_configurable_in_edges_[node];  

  /* By default the non-configurable edges will be stored at second part of the edge list (XX to end) */
  auto end = node_in_edges(node).end();  

  return vtr::make_range(begin, end); 
}

/* Get the list of configurable edges from the input edges of a given node 
 * And return the range(iterators) of the list 
 */
RRGraph::edge_range RRGraph::node_configurable_out_edges(RRNodeId node) const {
  /* Make sure we will access a valid node */
  VTR_ASSERT_SAFE(valid_node_id(node));

  /* By default the configurable edges will be stored at the first part of the edge list (0 to XX) */
  auto begin = node_in_edges(node).begin();  

  /* By default the non-configurable edges will be stored at second part of the edge list (XX to end) */
  auto end = node_in_edges(node).end() - node_num_non_configurable_out_edges_[node];  

  return vtr::make_range(begin, end); 
}

/* Get the list of configurable edges from the input edges of a given node 
 * And return the range(iterators) of the list 
 */
RRGraph::edge_range RRGraph::node_non_configurable_out_edges(RRNodeId node) const {
  /* Make sure we will access a valid node */
  VTR_ASSERT_SAFE(valid_node_id(node));

  /* By default the configurable edges will be stored at the first part of the edge list (0 to XX) */
  auto begin = node_in_edges(node).begin() + node_num_non_configurable_out_edges_[node];  

  /* By default the non-configurable edges will be stored at second part of the edge list (XX to end) */
  auto end = node_in_edges(node).end();  

  return vtr::make_range(begin, end); 
}

RRGraph::edge_range RRGraph::node_out_edges(RRNodeId node) const {
  VTR_ASSERT_SAFE(valid_node_id(node));
  return vtr::make_range(node_out_edges_[node].begin(), node_out_edges_[node].end());
}

RRGraph::edge_range RRGraph::node_in_edges(RRNodeId node) const {
  VTR_ASSERT_SAFE(valid_node_id(node));
  return vtr::make_range(node_in_edges_[node].begin(), node_in_edges_[node].end());
}

//Edge attributes
size_t RRGraph::edge_index(RREdgeId edge) const {
  VTR_ASSERT_SAFE(valid_edge_id(edge));
  return size_t(edge); 
}

RRNodeId RRGraph::edge_src_node(RREdgeId edge) const {
  VTR_ASSERT_SAFE(valid_edge_id(edge));
  return edge_src_nodes_[edge];
}
RRNodeId RRGraph::edge_sink_node(RREdgeId edge) const {
  VTR_ASSERT_SAFE(valid_edge_id(edge));
  return edge_sink_nodes_[edge];
}

RRSwitchId RRGraph::edge_switch(RREdgeId edge) const {
  VTR_ASSERT_SAFE(valid_edge_id(edge));
  return edge_switches_[edge];
}

/* Check if the edge is a configurable edge (programmble) */
bool RRGraph::edge_is_configurable(RREdgeId edge) const {
  /* Make sure we have a valid edge id */
  VTR_ASSERT_SAFE(valid_edge_id(edge));

  auto iswitch = edge_switch(edge);

  return switches_[iswitch].configurable();
}

/* Check if the edge is a non-configurable edge (hardwired) */
bool RRGraph::edge_is_non_configurable(RREdgeId edge) const {
  /* Make sure we have a valid edge id */
  VTR_ASSERT_SAFE(valid_edge_id(edge));
  return !edge_is_configurable(edge);
}

size_t RRGraph::switch_index(RRSwitchId switch_id) const {
  VTR_ASSERT_SAFE(valid_switch_id(switch_id));
  return size_t(switch_id); 
}

/*
 * Get a switch from the rr_switch list with a given id  
 */
const t_rr_switch_inf& RRGraph::get_switch(RRSwitchId switch_id) const {
  VTR_ASSERT_SAFE(valid_switch_id(switch_id));
  return switches_[switch_id]; 
}

size_t RRGraph::segment_index(RRSegmentId segment_id) const {
  VTR_ASSERT_SAFE(valid_segment_id(segment_id));
  return size_t(segment_id); 
}

/*
 * Get a segment from the segment list with a given id  
 */
const t_segment_inf& RRGraph::get_segment(RRSegmentId segment_id) const {
  VTR_ASSERT_SAFE(valid_segment_id(segment_id));
  return segments_[segment_id]; 
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

RRNodeId RRGraph::find_node(short x, short y, t_rr_type type, int ptc, e_side side) const {
  if (!valid_fast_node_lookup()) {
    build_fast_node_lookup();
  }
  size_t itype = type;
  size_t iside = side;
  return node_lookup_[x][y][itype][ptc][iside];
}

RRGraph::node_range RRGraph::find_nodes(short x, short y, t_rr_type type, int ptc) const {
  if (!valid_fast_node_lookup()) {
    build_fast_node_lookup();
  }

  const auto& matching_nodes = node_lookup_[x][y][type][ptc];

  return vtr::make_range(matching_nodes.begin(), matching_nodes.end());
}

/* This function aims to print basic information about a node */
void RRGraph::print_node(RRNodeId node) const {

  VTR_LOG("Node id: %d\n", node_index(node)); 
  VTR_LOG("Node type: %s\n", node_type_string(node)); 
  VTR_LOG("Node xlow: %d\n", node_xlow(node)); 
  VTR_LOG("Node ylow: %d\n", node_ylow(node)); 
  VTR_LOG("Node xhigh: %d\n", node_xhigh(node)); 
  VTR_LOG("Node yhigh: %d\n", node_yhigh(node)); 
  VTR_LOG("Node ptc: %d\n", node_ptc_num(node)); 
  VTR_LOG("Node num in_edges: %d\n", node_in_edges(node).size()); 
  VTR_LOG("Node num out_edges: %d\n", node_out_edges(node).size()); 

  return;
}

/* This function aims at checking any duplicated edges (with same EdgeId) 
 * of a given node. 
 * We will walkthrough the input edges of a node and see if there is any duplication
 */
bool RRGraph::check_node_duplicated_edges(RRNodeId node) const {
  bool no_duplication = true;
  /* Check each input edges */
  for (size_t iedge = 0; iedge < node_in_edges_[node].size(); ++iedge) {
    for (size_t jedge = iedge + 1; jedge < node_in_edges_[node].size(); ++jedge) {
      if (node_in_edges_[node][iedge] != node_in_edges_[node][jedge]) {
        /* Normal case, continue */
        continue;
      }
      /* Reach here it means we find some duplicated edges and report errors */ 
      /* Print a warning! */
      VTR_LOG_WARN("Node %s has duplicated input edges (edgeId:%d and %d)!\n", 
                   node, iedge, jedge);
      this->print_node(node);
      no_duplication = false;
    }
  }
  
  return no_duplication;
}

/* Check the whole Routing Resource Graph  
 * identify and report any duplicated edges between two nodes 
 */
bool RRGraph::check_duplicated_edges() const {
  bool no_duplication = true;
  /* For each node:
   * Search input edges, see there are two edges with same id or address 
   */
  for (auto node : nodes()) {
    if (true == check_node_duplicated_edges(node)) {
      no_duplication = false;
    }
  }

  return no_duplication;
}

/* 
 * identify and report any duplicated edges between two nodes 
 */
bool RRGraph::check_dangling_nodes() const {
  bool no_dangling = true;
  /* For each node: 
   * check if the number of input edges and output edges are both 0
   * If so, this is a dangling nodes and report 
   */
  for (auto node : nodes()) {
    if  (  (0 == this->node_fan_in(node))
       &&(0 == this->node_fan_out(node)) ) {
      /* Print a warning! */
      VTR_LOG_WARN("Node %s is dangling (zero fan-in and zero fan-out)!\n", 
                   node);
      VTR_LOG_WARN("Node details for debugging:\n");
      this->print_node(node);
      no_dangling = false;
    }
  } 

  return no_dangling;
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
void RRGraph::reserve_nodes(int num_nodes) {
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
  this->node_segment_ids_.reserve(num_nodes);  
  this->node_num_non_configurable_in_edges_.reserve(num_nodes); 
  this->node_num_non_configurable_out_edges_.reserve(num_nodes); 
 
  /* Edge-relate vectors */
  this->node_in_edges_.reserve(num_nodes);  
  this->node_out_edges_.reserve(num_nodes);  

  return; 
}

/* Reserve a list of edges */
void RRGraph::reserve_edges(int num_edges) {
  /* Reserve the full set of vectors related to edges */
  this->edge_ids_.reserve(num_edges);  
  this->edge_src_nodes_.reserve(num_edges);  
  this->edge_sink_nodes_.reserve(num_edges);  
  this->edge_switches_.reserve(num_edges);  

  return; 
}

/* Reserve a list of switches */
void RRGraph::reserve_switches(int num_switches) {
  this->switch_ids_.reserve(num_switches);  
  this->switches_.reserve(num_switches);  

  return; 
}

/* Reserve a list of segments */
void RRGraph::reserve_segments(int num_segments) {

  this->segment_ids_.reserve(num_segments);  
  this->segments_.reserve(num_segments);  

  return; 
}

/* Mutators */
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

  node_num_non_configurable_in_edges_.emplace_back(); //Initially empty
  node_num_non_configurable_out_edges_.emplace_back(); //Initially empty

  invalidate_fast_node_lookup();

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

/* Create segment */
RRSegmentId RRGraph::create_segment(t_segment_inf segment_info) {
  //Allocate an ID
  RRSegmentId segment_id = RRSegmentId(segment_ids_.size());
  segment_ids_.push_back(segment_id);

  segments_.push_back(segment_info);

  return segment_id;
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

  //Invalidate the node look-up
  invalidate_fast_node_lookup();

  set_dirty();
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

  set_dirty();
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

/*
 * Set a segment id for a node in rr_graph 
 */
void RRGraph::set_node_segment_id(RRNodeId node, short segment_id) {
  VTR_ASSERT(valid_node_id(node));

  node_segment_ids_[node] = segment_id;

  return; 
}

/* For a given node in a rr_graph
 * classify the edges of each node to be configurable (1st part) and non-configurable (2nd part) 
 */
void RRGraph::partition_node_in_edges(RRNodeId node) {

  //Partition the edges so the first set of edges are all configurable, and the later are not
  auto first_non_config_edge = std::partition(node_in_edges_[node].begin(), node_in_edges_[node].end(), 
                                              [&](const RREdgeId edge) { return edge_is_configurable(edge); } ); /* Condition to partition edges */

  size_t num_conf_edges = std::distance(node_in_edges_[node].begin(), first_non_config_edge);
  size_t num_non_conf_edges = node_in_edges_[node].size() - num_conf_edges; //Note we calculate using the size_t to get full range

  /* Check that within allowable range (no overflow when stored as num_non_configurable_edges_
   */
  VTR_ASSERT_MSG(num_non_conf_edges <= 
                 node_in_edges_[node].size(),
                 "Exceeded RR node maximum number of non-configurable input edges");

  node_num_non_configurable_in_edges_[node] = num_non_conf_edges; //Narrowing

  return;
}

/* For a given node in a rr_graph
 * classify the edges of each node to be configurable (1st part) and non-configurable (2nd part) 
 */
void RRGraph::partition_node_out_edges(RRNodeId node) {

  //Partition the edges so the first set of edges are all configurable, and the later are not
  auto first_non_config_edge = std::partition(node_out_edges_[node].begin(), node_out_edges_[node].end(),
                                              [&](const RREdgeId edge) { return edge_is_configurable(edge); } ); /* Condition to partition edges */

  size_t num_conf_edges = std::distance(node_out_edges_[node].begin(), first_non_config_edge);
  size_t num_non_conf_edges = node_out_edges_[node].size() - num_conf_edges; //Note we calculate using the size_t to get full range

  /* Check that within allowable range (no overflow when stored as num_non_configurable_edges_
   */
  VTR_ASSERT_MSG(num_non_conf_edges <= 
                 node_out_edges_[node].size(),
                 "Exceeded RR node maximum number of non-configurable output edges");

  node_num_non_configurable_out_edges_[node] = num_non_conf_edges; //Narrowing

  return;
}


/* For all nodes in a rr_graph  
 * classify the input edges of each node to be configurable (1st part) and non-configurable (2nd part) 
 */
void RRGraph::partition_in_edges() {
  /* For each node */
  for (auto node : nodes()) {
   this->partition_node_in_edges(node);
  }

  return;
}

/* For all nodes in a rr_graph  
 * classify the output edges of each node to be configurable (1st part) and non-configurable (2nd part) 
 */
void RRGraph::partition_out_edges() {
  /* For each node */
  for (auto node : nodes()) {
   this->partition_node_out_edges(node);
  }

  return;
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

  return;
}

/* 
 * configure the capacitance of each node with 
 * the capacitance of each incoming edge switch 
 * TODO: adapt from the function rr_graph_externals
 * Other thoughts: we may have to keep this function outside 
 * for the following reasons:
 * 1. It will sum up capaitances from switches for nodes
 *    But it may change depending on architectures
 * 2. It still requires a large amount of device information  
 */
void RRGraph::load_switch_C() {
  return;
} 

void RRGraph::build_fast_node_lookup() const {
  invalidate_fast_node_lookup();

  for (auto node : nodes()) {
    size_t x = node_xlow(node);
    if (x < node_lookup_.size()) {
        node_lookup_.resize(x + 1);
    }

    size_t y = node_ylow(node);
    if (y < node_lookup_[x].size()) {
        node_lookup_[x].resize(y + 1);
    }

    size_t itype = node_type(node);
    if (itype < node_lookup_[x][y].size()) {
        node_lookup_[x][y].resize(itype + 1);
    }

    size_t ptc = node_ptc_num(node);
    if (ptc < node_lookup_[x][y][itype].size()) {
        node_lookup_[x][y][itype].resize(ptc + 1);
    }

    size_t iside = -1;
    if (node_type(node) == OPIN || node_type(node) == IPIN) {
        iside = node_side(node); 
    } else {
        iside = NUM_SIDES;
    }

    if (iside < node_lookup_[x][y][itype][ptc].size()) {
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

/* check if a given switch id is valid or not */
bool RRGraph::valid_switch_id(RRSwitchId switch_id) const {
  /* TODO: should we check the index of switch[id] matches ? */
  return size_t(switch_id) < switches_.size();
}

/* check if a given segment id is valid or not */
bool RRGraph::valid_segment_id(RRSegmentId segment_id) const {
  /* TODO: should we check the index of segment[id] matches ? */
  return size_t(segment_id) < segments_.size();
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
            VTR_LOG(msg.c_str());
        }
    }

    for (auto edge : node_out_edges(node)) {
        auto src_node = edge_src_node(edge);
        if (src_node != node) {
            std::string msg = vtr::string_fmt("Mismatched node out-edge cross referrence: RR Edge %zu, RR Node != RR Edge Source: (%zu != %zu)", size_t(edge), size_t(node), size_t(src_node));
            VTR_LOG(msg.c_str());
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
        VTR_LOG(msg.c_str());
    }

    itr = std::find(sink_in_edges.begin(), sink_in_edges.end(), edge);
    if (itr == sink_in_edges.end()) { //Not found
        std::string msg = vtr::string_fmt("Mismatched edge node cross reference:"
                                          " RR Edge %zu not found with associated sink RR Node %zu",
                                          size_t(edge), size_t(sink_node));
        VTR_LOG(msg.c_str());
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

        VTR_LOG(msg.c_str());
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

  invalidate_fast_node_lookup();

  clear_dirty();
}

void RRGraph::build_id_maps(vtr::vector<RRNodeId,RRNodeId>& node_id_map,
                        vtr::vector<RREdgeId,RREdgeId>& edge_id_map) {
  node_id_map = compress_ids(node_ids_);
  edge_id_map = compress_ids(edge_ids_);
}

void RRGraph::clean_nodes(const vtr::vector<RRNodeId,RRNodeId>& node_id_map) {
  node_ids_ = clean_and_reorder_ids(node_id_map);

  node_types_ = clean_and_reorder_values(node_types_, node_id_map);
  /* FIXME: the following usage of clean_and_reorder_values causes compilation error, 
   * Comment now and see if there is any further errors 
   */
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
  node_segment_ids_.clear();

  node_num_non_configurable_in_edges_.clear();
  node_num_non_configurable_out_edges_.clear();

  node_in_edges_.clear();
  node_out_edges_.clear();

  /* clean node_look_up */
  node_lookup_.clear();

  return;
}

/* Empty all the vectors related to edges */
void RRGraph::clear_edges() {
  edge_ids_.clear();
  edge_src_nodes_.clear();
  edge_sink_nodes_.clear();
  edge_switches_.clear();

  return;
}

/* Empty all the vectors related to switches */
void RRGraph::clear_switches() {
  switch_ids_.clear();
  switches_.clear();

  return;
}

/* Empty all the vectors related to segments */
void RRGraph::clear_segments() {
  segment_ids_.clear();
  segments_.clear();

  return;
}

/* Clean the rr_graph */
void RRGraph::clear() {
  clear_nodes();
  clear_edges();
  clear_switches();
  clear_segments();

  return;
}
