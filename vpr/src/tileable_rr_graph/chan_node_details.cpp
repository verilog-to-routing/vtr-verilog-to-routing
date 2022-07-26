/************************************************************************
 *  This file contains member functions for class ChanNodeDetails
 ***********************************************************************/
#include <algorithm>

/* Headers from vtrutil library */
#include "vtr_assert.h"

#include "chan_node_details.h"

/* begin namespace openfpga */
namespace openfpga {

/************************************************************************
 *  Constructors 
 ***********************************************************************/
ChanNodeDetails::ChanNodeDetails(const ChanNodeDetails& src) {
  /* duplicate */
  size_t chan_width = src.get_chan_width();
  this->reserve(chan_width); 
  for (size_t itrack = 0; itrack < chan_width; ++itrack) {
    track_node_ids_.push_back(src.get_track_node_id(itrack));
    track_direction_.push_back(src.get_track_direction(itrack));
    seg_ids_.push_back(src.get_track_segment_id(itrack));
    seg_length_.push_back(src.get_track_segment_length(itrack));
    track_start_.push_back(src.is_track_start(itrack));
    track_end_.push_back(src.is_track_end(itrack));
  }
}

ChanNodeDetails::ChanNodeDetails() {
  this->clear();
}

/************************************************************************
 *  Accessors
 ***********************************************************************/
size_t ChanNodeDetails::get_chan_width() const {
  VTR_ASSERT(validate_chan_width());
  return track_node_ids_.size();
}

size_t ChanNodeDetails::get_track_node_id(const size_t& track_id) const {
  VTR_ASSERT(validate_track_id(track_id));
  return track_node_ids_[track_id];
}

/* Return a copy of vector */
std::vector<size_t> ChanNodeDetails::get_track_node_ids() const {
  std::vector<size_t> copy;
  for (size_t inode = 0; inode < get_chan_width(); ++inode) {
    copy.push_back(track_node_ids_[inode]);
  } 
  return copy;
}

Direction ChanNodeDetails::get_track_direction(const size_t& track_id) const {
  VTR_ASSERT(validate_track_id(track_id));
  return track_direction_[track_id];
}

size_t ChanNodeDetails::get_track_segment_length(const size_t& track_id) const {
  VTR_ASSERT(validate_track_id(track_id));
  return seg_length_[track_id];
}

size_t ChanNodeDetails::get_track_segment_id(const size_t& track_id) const {
  VTR_ASSERT(validate_track_id(track_id));
  return seg_ids_[track_id];
}

bool   ChanNodeDetails::is_track_start(const size_t& track_id) const {
  VTR_ASSERT(validate_track_id(track_id));
  return track_start_[track_id];
}

bool   ChanNodeDetails::is_track_end(const size_t& track_id) const {
  VTR_ASSERT(validate_track_id(track_id));
  return track_end_[track_id];
}

/* Track_id is the starting point of group (whose is_start should be true) 
 * This function will try to find the track_ids with the same directionality as track_id and seg_length
 * A group size is the number of such nodes between the starting points (include the 1st starting point) 
 */
std::vector<size_t> ChanNodeDetails::get_seg_group(const size_t& track_id) const {
  VTR_ASSERT(validate_chan_width());
  VTR_ASSERT(validate_track_id(track_id));
  VTR_ASSERT(is_track_start(track_id));
  
  std::vector<size_t> group;
  /* Make sure a clean start */
  group.clear();

  for (size_t itrack = track_id; itrack < get_chan_width(); ++itrack) {
    if ( (get_track_direction(itrack) != get_track_direction(track_id) )
      || (get_track_segment_id(itrack) != get_track_segment_id(track_id)) ) {
      /* Bypass any nodes in different direction and segment information*/
      continue;
    }
    if ( (false == is_track_start(itrack)) 
      || ( (true == is_track_start(itrack)) && (itrack == track_id)) ) {
      group.push_back(itrack);
      continue;
    }
    /* Stop if this another starting point */
    if (true == is_track_start(itrack)) {
      break;
    }
  }  
  return group;
}

/* Get a list of track_ids with the given list of track indices */
std::vector<size_t> ChanNodeDetails::get_seg_group_node_id(const std::vector<size_t>& seg_group) const {
  std::vector<size_t> group;
  /* Make sure a clean start */
  group.clear();

  for (size_t id = 0; id < seg_group.size(); ++id) {
    VTR_ASSERT(validate_track_id(seg_group[id]));
    group.push_back(get_track_node_id(seg_group[id]));
  }

  return group;
}

/* Get the number of tracks that starts in this routing channel */
size_t ChanNodeDetails::get_num_starting_tracks(const Direction& track_direction) const {
  size_t counter = 0;
  for (size_t itrack = 0; itrack < get_chan_width(); ++itrack) {
    /* Bypass unmatched track_direction */
    if (track_direction != get_track_direction(itrack)) {
      continue;
    }
    if (false == is_track_start(itrack)) {
      continue;
    } 
    counter++;
  }  
  return counter;
}

/* Get the number of tracks that ends in this routing channel */
size_t ChanNodeDetails::get_num_ending_tracks(const Direction& track_direction) const {
  size_t counter = 0;
  for (size_t itrack = 0; itrack < get_chan_width(); ++itrack) {
    /* Bypass unmatched track_direction */
    if (track_direction != get_track_direction(itrack)) {
      continue;
    }
    if (false == is_track_end(itrack)) {
      continue;
    } 
    counter++;
  }  
  return counter;
}


/************************************************************************
 *  Mutators
 ***********************************************************************/
/* Reserve the capacitcy of vectors */
void ChanNodeDetails::reserve(const size_t& chan_width) {
  track_node_ids_.reserve(chan_width);
  track_direction_.reserve(chan_width);
  seg_length_.reserve(chan_width);
  seg_ids_.reserve(chan_width);
  track_start_.reserve(chan_width);
  track_end_.reserve(chan_width);
}

/* Add a track to the channel */
void ChanNodeDetails::add_track(const size_t& track_node_id, const Direction& track_direction,
                                const size_t& seg_id, const size_t& seg_length,
                                const size_t& is_start, const size_t& is_end) {
  track_node_ids_.push_back(track_node_id);
  track_direction_.push_back(track_direction);
  seg_ids_.push_back(seg_id);
  seg_length_.push_back(seg_length);
  track_start_.push_back(is_start);
  track_end_.push_back(is_end);
}

/* Update the node_id of a given track */
void ChanNodeDetails::set_track_node_id(const size_t& track_index, const size_t& track_node_id) {
  VTR_ASSERT(validate_track_id(track_index));
  track_node_ids_[track_index] = track_node_id; 
}

/* Update the node_ids from a vector */
void ChanNodeDetails::set_track_node_ids(const std::vector<size_t>& track_node_ids) {
  /* the size of vector should match chan_width */
  VTR_ASSERT ( get_chan_width() == track_node_ids.size() );
  for (size_t inode = 0; inode < track_node_ids.size(); ++inode) {
    track_node_ids_[inode] = track_node_ids[inode]; 
  }
}

/* Set tracks with a given direction to start */
void ChanNodeDetails::set_tracks_start(const Direction& track_direction) {
  for (size_t inode = 0; inode < get_chan_width(); ++inode) {
    /* Bypass non-match tracks */
    if (track_direction != get_track_direction(inode)) {
      continue; /* Pass condition*/
    }
    track_start_[inode] = true;
  }
}

/* Set tracks with a given direction to end */
void ChanNodeDetails::set_tracks_end(const Direction& track_direction) {
  for (size_t inode = 0; inode < get_chan_width(); ++inode) {
    /* Bypass non-match tracks */
    if (track_direction != get_track_direction(inode)) {
      continue; /* Pass condition*/
    }
    track_end_[inode] = true;
  }
}

/* rotate the track_node_id by an offset */
void ChanNodeDetails::rotate_track_node_id(const size_t& offset, const Direction& track_direction, const bool& counter_rotate) {
  /* Direct return if offset = 0*/
  if (0 == offset) {
    return;
  }
 
  /* Rotate the node_ids by groups
   * A group begins from a track_start and ends before another track_start  
   */
  VTR_ASSERT(validate_chan_width());
  for (size_t itrack = 0; itrack < get_chan_width(); ++itrack) { 
    /* Bypass non-start segment */
    if (false == is_track_start(itrack) ) {
      continue;
    }
    /* Bypass segments do not match track_direction */
    if (track_direction != get_track_direction(itrack) ) {
      continue;
    }
    /* Find the group nodes */
    std::vector<size_t> track_group = get_seg_group(itrack);
    /* Build a vector of the node ids of the tracks */
    std::vector<size_t> track_group_node_id = get_seg_group_node_id(track_group);
    /* adapt offset to the range of track_group_node_id */
    size_t actual_offset = offset % track_group_node_id.size();
    /* Rotate or Counter rotate */
    if (true == counter_rotate) {
      std::rotate(track_group_node_id.rbegin(), track_group_node_id.rbegin() + actual_offset, track_group_node_id.rend());
    } else {
      std::rotate(track_group_node_id.begin(), track_group_node_id.begin() + actual_offset, track_group_node_id.end());
    }
    /* Update the node_ids */
    for (size_t inode = 0; inode < track_group.size(); ++inode) {
      track_node_ids_[track_group[inode]] = track_group_node_id[inode];
    }
  }
  return;
}

void ChanNodeDetails::clear() {
  track_node_ids_.clear();
  track_direction_.clear();
  seg_ids_.clear();
  seg_length_.clear();
  track_start_.clear();
  track_end_.clear();
}

/************************************************************************
 *  Validators 
 ***********************************************************************/
bool ChanNodeDetails::validate_chan_width() const {
  size_t chan_width = track_node_ids_.size();
  if ( (chan_width == track_direction_.size())
     &&(chan_width == seg_ids_.size())
     &&(chan_width == seg_length_.size())
     &&(chan_width == track_start_.size())
     &&(chan_width == track_end_.size()) ) {
    return true;
  }
  return false;
}

bool ChanNodeDetails::validate_track_id(const size_t& track_id) const {
  if ( (track_id < track_node_ids_.size()) 
    && (track_id < track_direction_.size()) 
    && (track_id < seg_ids_.size()) 
    && (track_id < seg_length_.size()) 
    && (track_id < track_start_.size()) 
    && (track_id < track_end_.size()) ) {
    return true;
  } 
  return false;
}

} /* end namespace openfpga */
