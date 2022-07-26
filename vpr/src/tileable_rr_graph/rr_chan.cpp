/************************************************************************
 * Member functions for class RRChan
 ***********************************************************************/
#include "vtr_log.h"
#include "vtr_assert.h"
#include "rr_chan.h"

/* namespace openfpga begins */
namespace openfpga {

/************************************************************************
 * Constructors
 ***********************************************************************/
/* default constructor */
RRChan::RRChan() {
  type_ = NUM_RR_TYPES;
  nodes_.resize(0);
  node_segments_.resize(0);
}

/************************************************************************
 * Accessors
 ***********************************************************************/
t_rr_type RRChan::get_type() const {
  return type_;
}

/* get the number of tracks in this channel */
size_t RRChan::get_chan_width() const { 
  return nodes_.size();
}

/* get the track_id of a node */
int RRChan::get_node_track_id(const RRNodeId& node) const {
  /* if the given node is NULL, we return an invalid id */
  if (RRNodeId::INVALID() == node) {
    return -1;
  }
  /* check each member and return if we find a match in content */
  std::vector<RRNodeId>::const_iterator it = std::find(nodes_.begin(), nodes_.end(), node);
  if (nodes_.end() == it) {
    return -1;
  }
  return it - nodes_.begin();
}

/* get the rr_node with the track_id */
RRNodeId RRChan::get_node(const size_t& track_num) const {
  if ( false == valid_node_id(track_num) ) {
    return RRNodeId::INVALID();
  }
  return nodes_[track_num];
} 

/* get the segment id of a node */
RRSegmentId RRChan::get_node_segment(const RRNodeId& node) const {
  int node_id = get_node_track_id(node);
  if ( false == valid_node_id(node_id)) {
    return RRSegmentId::INVALID();
  }
  return get_node_segment(node_id); 
}

/* get the segment id of a node */
RRSegmentId RRChan::get_node_segment(const size_t& track_num) const {
  if ( false == valid_node_id(track_num)) {
    return RRSegmentId::INVALID();
  }
  return node_segments_[track_num]; 
}

/* evaluate if two RRChan is mirror to each other */
bool RRChan::is_mirror(const RRGraph& rr_graph, const RRChan& cand) const {
  /* If any following element does not match, it is not mirror */
  /* 1. type  */
  if (this->get_type() != cand.get_type()) {
    return false;
  }
  /* 2. track_width  */
  if (this->get_chan_width() != cand.get_chan_width()) {
    return false;
  }
  /* 3. for each node */
  for (size_t inode = 0; inode < this->get_chan_width(); ++inode) {
    /* 3.1 check node type */
    if (rr_graph.node_type(this->get_node(inode)) != rr_graph.node_type(cand.get_node(inode))) {
      return false;
    }
    /* 3.2 check node directionality */
    if (rr_graph.node_direction(this->get_node(inode)) != rr_graph.node_direction(cand.get_node(inode))) {
      return false;
    }
    /* 3.3 check node segment */
    if (this->get_node_segment(inode) != cand.get_node_segment(inode)) {
      return false;
    }
  }
 
  return true;
}

/* Get a list of segments used in this routing channel */
std::vector<RRSegmentId> RRChan::get_segment_ids() const { 
  std::vector<RRSegmentId> seg_list;

  /* make sure a clean start */
  seg_list.clear();

  /* Traverse node_segments */
  for (size_t inode = 0; inode < get_chan_width(); ++inode) {
    std::vector<RRSegmentId>::iterator it;
    /* Try to find the node_segment id in the list */
    it = find(seg_list.begin(), seg_list.end(), node_segments_[inode]);
    if ( it == seg_list.end() ) {
      /* Not found, add it to the list */
      seg_list.push_back(node_segments_[inode]);
    }
  }

  return seg_list;
}

/* Get a list of nodes whose segment_id is specified  */
std::vector<size_t> RRChan::get_node_ids_by_segment_ids(const RRSegmentId& seg_id) const {
  std::vector<size_t> node_list;

  /* make sure a clean start */
  node_list.clear();

  /* Traverse node_segments */
  for (size_t inode = 0; inode < get_chan_width(); ++inode) {
    /* Try to find the node_segment id in the list */
    if ( seg_id == node_segments_[inode] ) {
      node_list.push_back(inode);
    }
  }

  return node_list;
} 

/************************************************************************
 * Mutators
 ***********************************************************************/
void RRChan::set(const RRChan& rr_chan) {
  /* Ensure a clean start */
  this->clear();
  /* Assign type of this routing channel */
  this->type_ = rr_chan.get_type();
  /* Copy node and node_segments */
  this->nodes_.resize(rr_chan.get_chan_width());
  this->node_segments_.resize(rr_chan.get_chan_width());
  for (size_t inode = 0; inode < rr_chan.get_chan_width(); ++inode) { 
    this->nodes_[inode] = rr_chan.get_node(inode);
    this->node_segments_[inode] = rr_chan.get_node_segment(inode);
  }
  return;
}

/* modify type */
void RRChan::set_type(const t_rr_type& type) {
  VTR_ASSERT(valid_type(type));
  type_ = type;
} 

/* Reserve node list */
void RRChan::reserve_node(const size_t& node_size) {
  nodes_.reserve(node_size); /* reserve to the maximum */
  node_segments_.reserve(node_size); /* reserve to the maximum */
}

/* add a node to the array */
void RRChan::add_node(const RRGraph& rr_graph, const RRNodeId& node, const RRSegmentId& node_segment) {
  /* fill the dedicated element in the vector */
  nodes_.push_back(node);
  node_segments_.push_back(node_segment);

  if (NUM_RR_TYPES == type_) {
    type_ = rr_graph.node_type(node);
  } else {
    VTR_ASSERT(type_ == rr_graph.node_type(node));
  }

  VTR_ASSERT(valid_node_type(rr_graph, node));
}

/* Clear content */
void RRChan::clear() {
  nodes_.clear();
  node_segments_.clear();
}

/************************************************************************
 * Internal validators
 ***********************************************************************/
/* for type, only valid type is CHANX and CHANY */
bool RRChan::valid_type(const t_rr_type& type) const {
  if ((CHANX == type) || (CHANY == type)) {
    return true;
  }
  return false;
}  

/* Check each node, see if the node type is consistent with the type */
bool RRChan::valid_node_type(const RRGraph& rr_graph, const RRNodeId& node) const {
  valid_type(rr_graph.node_type(node));
  if (NUM_RR_TYPES == type_) {
    return true;
  }
  valid_type(type_);
  if (type_ != rr_graph.node_type(node)) {
    return false;
  }
  return true;
}

/* check if the node id is valid */
bool RRChan::valid_node_id(const size_t& node_id) const {
  if (node_id < nodes_.size()) {
    return true;
  }

  return false;
}

} /* End namespace openfpga*/
