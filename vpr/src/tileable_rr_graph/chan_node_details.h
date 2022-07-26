#ifndef CHAN_NODE_DETAILS_H
#define CHAN_NODE_DETAILS_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <vector>
#include "vpr_types.h"
#include "rr_node_types.h"
#include "rr_graph_types.h"

/************************************************************************
 *  This file contains a class to model the details of routing node
 *  in a channel:
 *  1. segment information: length, frequency etc.
 *  2. starting point of segment
 *  3. ending point of segment
 *  4. potentail track_id(ptc_num) of each segment
 ***********************************************************************/

/* begin namespace openfpga */
namespace openfpga {

/************************************************************************
 *  ChanNodeDetails records segment length, directionality and starting of routing tracks
 *    +---------------------------------+
 *    | Index | Direction | Start Point |
 *    +---------------------------------+
 *    |   0   | --------> |   Yes       |
 *    +---------------------------------+
 ***********************************************************************/

class ChanNodeDetails {
  public : /* Constructor */
    ChanNodeDetails(const ChanNodeDetails&); /* Duplication */
    ChanNodeDetails(); /* Initilization */
  public: /* Accessors */
    size_t get_chan_width() const;
    size_t get_track_node_id(const size_t& track_id) const;
    std::vector<size_t> get_track_node_ids() const;
    Direction get_track_direction(const size_t& track_id) const;
    size_t get_track_segment_length(const size_t& track_id) const;
    size_t get_track_segment_id(const size_t& track_id) const;
    bool is_track_start(const size_t& track_id) const;
    bool is_track_end(const size_t& track_id) const;
    std::vector<size_t> get_seg_group(const size_t& track_id) const;
    std::vector<size_t> get_seg_group_node_id(const std::vector<size_t>& seg_group) const;
    size_t get_num_starting_tracks(const Direction& track_direction) const;
    size_t get_num_ending_tracks(const Direction& track_direction) const;
  public: /* Mutators */
    void reserve(const size_t& chan_width); /* Reserve the capacitcy of vectors */
    void add_track(const size_t& track_node_id, const Direction& track_direction,
                   const size_t& seg_id, const size_t& seg_length,
                   const size_t& is_start, const size_t& is_end);
    void set_track_node_id(const size_t& track_index, const size_t& track_node_id);
    void set_track_node_ids(const std::vector<size_t>& track_node_ids);
    void set_tracks_start(const Direction& track_direction);
    void set_tracks_end(const Direction& track_direction);
    void rotate_track_node_id(const size_t& offset,
                              const Direction& track_direction,
                              const bool& counter_rotate); /* rotate the track_node_id by an offset */
    void clear();
  private: /* validators */
    bool validate_chan_width() const;
    bool validate_track_id(const size_t& track_id) const;
  private: /* Internal data */ 
    std::vector<size_t> track_node_ids_; /* indices of each track */
    std::vector<Direction> track_direction_; /* direction of each track */
    std::vector<size_t> seg_ids_; /* id of segment of each track */
    std::vector<size_t> seg_length_; /* Length of each segment */
    std::vector<bool>   track_start_; /* flag to identify if this is the starting point of the track */
    std::vector<bool>   track_end_; /* flag to identify if this is the ending point of the track */
};

} /* end namespace openfpga */

#endif 
