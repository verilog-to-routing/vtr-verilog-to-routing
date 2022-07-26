/************************************************************************
 *  This file contains a builder for track-to-track connections inside a 
 *  tileable General Switch Block (GSB).
 ***********************************************************************/
#include <vector>
#include <cmath>
#include <algorithm>

/* Headers from vtrutil library */
#include "vtr_assert.h"
#include "vtr_log.h"

/* Headers from openfpgautil library */
#include "openfpga_side_manager.h"

#include "vpr_utils.h"
#include "rr_graph_obj_util.h"
#include "openfpga_rr_graph_utils.h"
#include "rr_graph_builder_utils.h"
#include "tileable_chan_details_builder.h"
#include "tileable_rr_graph_gsb.h"

/* begin namespace openfpga */
namespace openfpga {

/************************************************************************
 * Internal data structures
 ***********************************************************************/
typedef std::vector<std::vector<int>> t_track_group;

/************************************************************************
 * A enumeration to list the status of a track inside a GSB
 * 1. start; 2. end; 3. passing
 * This is used to group tracks which ease the building of
 * track-to-track mapping matrix
 ***********************************************************************/
enum e_track_status {
  TRACK_START,
  TRACK_END,
  TRACK_PASS,
  NUM_TRACK_STATUS /* just a place holder to get the number of status */
};

/************************************************************************
 * Check if a track starts from this GSB or not 
 * (xlow, ylow) should be same as the GSB side coordinate 
 *
 * Check if a track ends at this GSB or not 
 * (xhigh, yhigh) should be same as the GSB side coordinate 
 ***********************************************************************/
static 
enum e_track_status determine_track_status_of_gsb(const RRGraph& rr_graph,
                                                  const RRGSB& rr_gsb, 
                                                  const enum e_side& gsb_side, 
                                                  const size_t& track_id) {
  enum e_track_status track_status = TRACK_PASS;
  /* Get the rr_node */
  RRNodeId track_node = rr_gsb.get_chan_node(gsb_side, track_id);
  /* Get the coordinates */
  vtr::Point<size_t> side_coordinate = rr_gsb.get_side_block_coordinate(gsb_side); 

  /* Get the coordinate of where the track starts */
  vtr::Point<size_t> track_start = get_track_rr_node_start_coordinate(rr_graph, track_node);

  /* INC_DIRECTION start_track: (xlow, ylow) should be same as the GSB side coordinate */
  /* DEC_DIRECTION start_track: (xhigh, yhigh) should be same as the GSB side coordinate */
  if (  (track_start.x() == side_coordinate.x())
     && (track_start.y() == side_coordinate.y()) 
     && (OUT_PORT == rr_gsb.get_chan_node_direction(gsb_side, track_id)) ) {
    /* Double check: start track should be an OUTPUT PORT of the GSB */
    track_status = TRACK_START;
  }

  /* Get the coordinate of where the track ends */
  vtr::Point<size_t> track_end = get_track_rr_node_end_coordinate(rr_graph, track_node);

  /* INC_DIRECTION end_track: (xhigh, yhigh) should be same as the GSB side coordinate */ 
  /* DEC_DIRECTION end_track: (xlow, ylow) should be same as the GSB side coordinate */ 
  if (  (track_end.x() == side_coordinate.x())
     && (track_end.y() == side_coordinate.y()) 
     && (IN_PORT == rr_gsb.get_chan_node_direction(gsb_side, track_id)) ) {
    /* Double check: end track should be an INPUT PORT of the GSB */
    track_status = TRACK_END;
  }

  return track_status;
}

/************************************************************************
 * Check if the GSB is in the Connection Block (CB) population list of the segment
 * SB population of a L4 wire: 1 0 0 1
 *
 *                            +----+    +----+    +----+    +----+
 *                            | CB |--->| CB |--->| CB |--->| CB |
 *                            +----+    +----+    +----+    +----+
 *  Engage CB connection       Yes       No        No        Yes
 *
 *  We will find the offset between gsb_side_coordinate and (xlow,ylow) of the track
 *  Use the offset to check if the tracks should engage in this GSB connection
 ***********************************************************************/
static 
bool is_gsb_in_track_cb_population(const RRGraph& rr_graph,
                                   const RRGSB& rr_gsb, 
                                   const e_side& gsb_side, 
                                   const int& track_id,
                                   const std::vector<t_segment_inf>& segment_inf) {
  /* Get the rr_node */
  RRNodeId track_node = rr_gsb.get_chan_node(gsb_side, track_id);
  /* Get the coordinates */
  vtr::Point<size_t> side_coordinate = rr_gsb.get_side_block_coordinate(gsb_side); 

  vtr::Point<size_t> track_start = get_track_rr_node_start_coordinate(rr_graph, track_node);

  /* Get the offset */
  size_t offset = std::abs((int)side_coordinate.x() - (int)track_start.x()) 
                + std::abs((int)side_coordinate.y() - (int)track_start.y()); 
  
  /* Get segment id */
  RRSegmentId seg_id = rr_gsb.get_chan_node_segment(gsb_side, track_id);
  /* validate offset */
  VTR_ASSERT(offset < segment_inf[size_t(seg_id)].cb.size());

  /* Get the SB population */
  bool in_cb_population = false;
  if (true == segment_inf[size_t(seg_id)].cb[offset]) {  
    in_cb_population = true;
  }

  return in_cb_population;
}

/************************************************************************
 * Check if the GSB is in the Switch Block (SB) population list of the segment
 * SB population of a L3 wire: 1 0 0 1
 *
 *                            +----+    +----+    +----+    +----+
 *                            | SB |--->| SB |--->| SB |--->| SB |
 *                            +----+    +----+    +----+    +----+
 *  Engage SB connection       Yes       No        No        Yes
 *
 *  We will find the offset between gsb_side_coordinate and (xlow,ylow) of the track
 *  Use the offset to check if the tracks should engage in this GSB connection
 ***********************************************************************/
static 
bool is_gsb_in_track_sb_population(const RRGraph& rr_graph,
                                   const RRGSB& rr_gsb, 
                                   const e_side& gsb_side, 
                                   const int& track_id,
                                   const std::vector<t_segment_inf>& segment_inf) {
  /* Get the rr_node */
  const RRNodeId& track_node = rr_gsb.get_chan_node(gsb_side, track_id);
  /* Get the coordinates */
  vtr::Point<size_t> side_coordinate = rr_gsb.get_side_block_coordinate(gsb_side); 

  vtr::Point<size_t> track_start = get_track_rr_node_start_coordinate(rr_graph, track_node);

  /* Get the offset */
  size_t offset = std::abs((int)side_coordinate.x() - (int)track_start.x()) 
                + std::abs((int)side_coordinate.y() - (int)track_start.y()); 
  
  /* Get segment id */
  RRSegmentId seg_id = rr_gsb.get_chan_node_segment(gsb_side, track_id);
  /* validate offset */
  VTR_ASSERT(offset < segment_inf[size_t(seg_id)].sb.size());

  /* Get the SB population */
  bool in_sb_population = false;
  if (true == segment_inf[size_t(seg_id)].sb[offset]) {  
    in_sb_population = true;
  }

  return in_sb_population;
}

/************************************************************************
 * Create a list of track_id based on the to_track and num_to_tracks
 * We consider the following list [to_track, to_track + Fs/3 - 1]
 * if the [to_track + Fs/3 - 1] exceeds the num_to_tracks, we start over from 0!
***********************************************************************/
static 
std::vector<size_t> get_to_track_list(const int& Fs, const int& to_track, const int& num_to_tracks) {
  std::vector<size_t> to_tracks;

  for (int i = 0; i < Fs; i = i + 3) {  
    /* TODO: currently, for Fs > 3, I always search the next from_track until Fs is satisfied 
     * The optimal track selection should be done in a more scientific way!!! 
     */
    int to_track_i = to_track + i;
    /* make sure the track id is still in range */
    if ( to_track_i > num_to_tracks - 1) {
      to_track_i = to_track_i % num_to_tracks; 
    }
    /* Ensure we are in the range */
    VTR_ASSERT(to_track_i < num_to_tracks);
    /* from track must be connected */
    to_tracks.push_back(to_track_i);
  }
  return to_tracks;
}

/************************************************************************
 * This function aims to return the track indices that drive the from_track
 * in a Switch Block 
 * The track_ids to return will depend on different topologies of SB
 *  SUBSET, UNIVERSAL, and WILTON. 
 ***********************************************************************/
static 
std::vector<size_t> get_switch_block_to_track_id(const e_switch_block_type& switch_block_type, 
                                                 const int& Fs,
                                                 const e_side& from_side,
                                                 const int& from_track,
                                                 const e_side& to_side, 
                                                 const int& num_to_tracks) {

  /* This routine returns the track number to which the from_track should     
   * connect.  It supports any Fs % 3 == 0, switch blocks.
   */
  std::vector<size_t> to_tracks;

  /* TODO: currently, for Fs > 3, I always search the next from_track until Fs is satisfied 
   * The optimal track selection should be done in a more scientific way!!! 
   */
  VTR_ASSERT(0 == Fs % 3);

  /* Adapt from_track to fit in the range of num_to_tracks */
  size_t actual_from_track = from_track % num_to_tracks; 

  switch (switch_block_type) {
  case SUBSET: /* NB:  Global routing uses SUBSET too */
    to_tracks = get_to_track_list(Fs, actual_from_track, num_to_tracks);
    /* Finish, we return */
    return to_tracks;
  case UNIVERSAL:
    if (  (from_side == LEFT)
       || (from_side == RIGHT) ) {  
     /* For the prev_side, to_track is from_track 
      * For the next_side, to_track is num_to_tracks - 1 - from_track 
      * For the opposite_side, to_track is always from_track
      */
      SideManager side_manager(from_side);
      if ( (to_side == side_manager.get_opposite()) 
        || (to_side == side_manager.get_rotate_counterclockwise()) ) {
        to_tracks = get_to_track_list(Fs, actual_from_track, num_to_tracks);
      } else if (to_side == side_manager.get_rotate_clockwise()) {
        to_tracks = get_to_track_list(Fs, num_to_tracks - 1 - actual_from_track, num_to_tracks);
      }
    }

    if (  (from_side ==  TOP)
       || (from_side == BOTTOM) ) {  
    /* For the next_side, to_track is from_track 
     * For the prev_side, to_track is num_to_tracks - 1 - from_track 
     * For the opposite_side, to_track is always from_track
     */
      SideManager side_manager(from_side);
      if ( (to_side == side_manager.get_opposite()) 
        || (to_side == side_manager.get_rotate_clockwise()) ) {
        to_tracks = get_to_track_list(Fs, actual_from_track, num_to_tracks);
      } else if (to_side == side_manager.get_rotate_counterclockwise()) {
        to_tracks = get_to_track_list(Fs, num_to_tracks - 1 - actual_from_track, num_to_tracks);
      }
    }
    /* Finish, we return */
    return to_tracks;
    /* End switch_block_type == UNIVERSAL case. */
  case WILTON:
    /* See S. Wilton Phd thesis, U of T, 1996 p. 103 for details on following. */
    if (from_side == LEFT) {
      if (to_side == RIGHT) { /* CHANX to CHANX */
        to_tracks = get_to_track_list(Fs, actual_from_track, num_to_tracks);
      } else if (to_side == TOP) { /* from CHANX to CHANY */
        to_tracks = get_to_track_list(Fs, (num_to_tracks - actual_from_track ) % num_to_tracks, num_to_tracks);
      } else if (to_side == BOTTOM) {
        to_tracks = get_to_track_list(Fs, (num_to_tracks + actual_from_track - 1) % num_to_tracks, num_to_tracks);
      }
    } else if (from_side == RIGHT) {
      if (to_side == LEFT) { /* CHANX to CHANX */
        to_tracks = get_to_track_list(Fs, actual_from_track, num_to_tracks);
      } else if (to_side == TOP) { /* from CHANX to CHANY */
        to_tracks = get_to_track_list(Fs, (num_to_tracks + actual_from_track - 1) % num_to_tracks, num_to_tracks);
      } else if (to_side == BOTTOM) {
        to_tracks = get_to_track_list(Fs, (2 * num_to_tracks - 2 - actual_from_track) % num_to_tracks, num_to_tracks);
      }
    } else if (from_side == BOTTOM) {
      if (to_side == TOP) { /* CHANY to CHANY */
        to_tracks = get_to_track_list(Fs, actual_from_track, num_to_tracks);
      } else if (to_side == LEFT) { /* from CHANY to CHANX */
        to_tracks = get_to_track_list(Fs, (actual_from_track + 1) % num_to_tracks, num_to_tracks);
      } else if (to_side == RIGHT) {
        to_tracks = get_to_track_list(Fs, (2 * num_to_tracks - 2 - actual_from_track) % num_to_tracks, num_to_tracks);
      }
    } else if (from_side == TOP) {
      if (to_side == BOTTOM) { /* CHANY to CHANY */
        to_tracks = get_to_track_list(Fs, from_track, num_to_tracks);
      } else if (to_side == LEFT) { /* from CHANY to CHANX */
        to_tracks = get_to_track_list(Fs, (num_to_tracks - actual_from_track) % num_to_tracks, num_to_tracks);
      } else if (to_side == RIGHT) {
        to_tracks = get_to_track_list(Fs, (actual_from_track + 1) % num_to_tracks, num_to_tracks);
      }
    }
    /* Finish, we return */
    return to_tracks;
  /* End switch_block_type == WILTON case. */
  default:
    VTR_LOGF_ERROR(__FILE__, __LINE__, 
                   "Invalid switch block pattern !\n");
    exit(1);
  }

  return to_tracks;
}


/************************************************************************
 * Build the track_to_track_map[from_side][0..chan_width-1][to_side][track_indices] 
 * For a group of from_track nodes and to_track nodes
 * For each side of from_tracks, we call a routine to get the list of to_tracks
 * Then, we fill the track2track_map
 ***********************************************************************/
static 
void build_gsb_one_group_track_to_track_map(const RRGraph& rr_graph, 
                                            const RRGSB& rr_gsb, 
                                            const e_switch_block_type& sb_type, 
                                            const int& Fs,
                                            const bool& wire_opposite_side,
                                            const t_track_group& from_tracks, /* [0..gsb_side][track_indices] */
                                            const t_track_group& to_tracks, /* [0..gsb_side][track_indices] */
                                            t_track2track_map& track2track_map) {
  for (size_t side = 0; side < from_tracks.size(); ++side) {
    SideManager side_manager(side);
    e_side from_side = side_manager.get_side();
    /* Find the other sides where the start tracks will locate */
    std::vector<e_side> to_track_sides;
    /* 0. opposite side */
    to_track_sides.push_back(side_manager.get_opposite());
    /* 1. prev side */
    /* Previous side definition: TOP => LEFT; RIGHT=>TOP; BOTTOM=>RIGHT; LEFT=>BOTTOM */
    to_track_sides.push_back(side_manager.get_rotate_counterclockwise()); 
    /* 2. next side */
    /* Next side definition: TOP => RIGHT; RIGHT=>BOTTOM; BOTTOM=>LEFT; LEFT=>TOP */
    to_track_sides.push_back(side_manager.get_rotate_clockwise()); 
    
    for (size_t inode = 0; inode < from_tracks[side].size(); ++inode) {
      for (size_t to_side_id = 0; to_side_id < to_track_sides.size(); ++to_side_id) {
        enum e_side to_side = to_track_sides[to_side_id];
        SideManager to_side_manager(to_side);
        size_t to_side_index = to_side_manager.to_size_t();
        /* Bypass those to_sides have no nodes  */
        if (0 == to_tracks[to_side_index].size()) {
          continue;
        }
        /* Bypass those from_side is same as to_side */
        if (from_side == to_side) {
          continue;
        }
        /* Bypass those from_side is opposite to to_side if required */
        if ( (true == wire_opposite_side) 
          && (to_side_manager.get_opposite() == from_side) ) {
          continue;
        }
        /* Get other track_ids depending on the switch block pattern */
        /* Find the track ids that will start at the other sides */
        std::vector<size_t> to_track_ids = get_switch_block_to_track_id(sb_type, Fs, from_side, inode, 
                                                                        to_side, 
                                                                        to_tracks[to_side_index].size()); 
        /* Update the track2track_map: */
        for (size_t to_track_id = 0; to_track_id < to_track_ids.size(); ++to_track_id) {
          size_t from_side_index = side_manager.to_size_t();
          size_t from_track_index = from_tracks[side][inode];
          /* Check the id is still in the range !*/
          VTR_ASSERT( to_track_ids[to_track_id] < to_tracks[to_side_index].size() );
          size_t to_track_index = to_tracks[to_side_index][to_track_ids[to_track_id]];
          //printf("from_track(size=%lu): %lu , to_track_ids[%lu]:%lu, to_track_index: %lu in a group of %lu tracks\n", 
          //       from_tracks[side].size(), inode, to_track_id, to_track_ids[to_track_id], 
          //       to_track_index, to_tracks[to_side_index].size());
          const RRNodeId& to_track_node = rr_gsb.get_chan_node(to_side, to_track_index);
          VTR_ASSERT(true == rr_graph.valid_node_id(to_track_node));

          /* from_track should be IN_PORT */
          VTR_ASSERT(IN_PORT == rr_gsb.get_chan_node_direction(from_side, from_track_index)); 
          /* to_track should be OUT_PORT */
          VTR_ASSERT(OUT_PORT == rr_gsb.get_chan_node_direction(to_side, to_track_index)); 

          /* Check if the to_track_node is already in the list ! */
          std::vector<RRNodeId>::iterator it = std::find(track2track_map[from_side_index][from_track_index].begin(),
                                                         track2track_map[from_side_index][from_track_index].end(),
                                                         to_track_node);
          if (it != track2track_map[from_side_index][from_track_index].end()) {
             continue; /* the node_id is already in the list, go for the next */
          }
          /* Clear, we should add to the list */
          track2track_map[from_side_index][from_track_index].push_back(to_track_node);
        }
      }
    }
  }
}

/************************************************************************
 * Build the track_to_track_map[from_side][0..chan_width-1][to_side][track_indices] 
 * based on the existing routing resources in the General Switch Block (GSB)
 * The track_indices is the indices of tracks that the node at from_side and [0..chan_width-1] will drive 
 * IMPORTANT: the track_indices are the indicies in the GSB context, but not the rr_graph!!!
 * We separate the connections into two groups:
 * Group 1: the routing tracks start from this GSB 
 *          We will apply switch block patterns (SUBSET, UNIVERSAL, WILTON) 
 * Group 2: the routing tracks do not start from this GSB (bypassing wires)
 *          We will apply switch block patterns (SUBSET, UNIVERSAL, WILTON) 
 *          but we will check the Switch Block (SB) population of these 
 *          routing segments, and determine which requires connections
 *          
 *                         CHANY  CHANY  CHANY CHANY
 *                          [0]    [1]    [2]   [3]
 *                   start  yes    no     yes   no            
 *        end             +-------------------------+           start    Group 1      Group 2
 *         no    CHANX[0] |           TOP           |  CHANX[0]  yes   TOP/BOTTOM   TOP/BOTTOM
 *                        |                         |                  CHANY[0,2]   CHANY[1,3]
 *        yes    CHANX[1] |                         |  CHANX[1]  no
 *                        |  LEFT           RIGHT   |
 *         no    CHANX[2] |                         |  CHANX[2]  yes
 *                        |                         |
 *        yes    CHANX[3] |         BOTTOM          |  CHANX[3]  no
 *                        +-------------------------+           
 *                         CHANY  CHANY  CHANY CHANY
 *                          [0]    [1]    [2]   [3]
 *                   start  yes    no     yes   no            
 *            
 * The mapping is done in the following steps: (For each side of the GSB)
 * 1. Build a list of tracks that will start from this side
 *    if a track starts, its xlow/ylow is the same as the x,y of this gsb
 * 2. Build a list of tracks on the other sides belonging to Group 1.
 *    Take the example of RIGHT side, we will collect 
 *    a. tracks that will end at the LEFT side 
 *    b. tracks that will start at the TOP side
 *    c. tracks that will start at the BOTTOM side
 * 3. Apply switch block patterns to Group 1 (SUBSET, UNIVERSAL, WILTON) 
 * 4. Build a list of tracks on the other sides belonging to Group 1.
 *    Take the example of RIGHT side, we will collect 
 *    a. tracks that will bypass at the TOP side
 *    b. tracks that will bypass at the BOTTOM side
 * 5. Apply switch block patterns to Group 2 (SUBSET, UNIVERSAL, WILTON) 
 ***********************************************************************/
t_track2track_map build_gsb_track_to_track_map(const RRGraph& rr_graph,
                                               const RRGSB& rr_gsb,
                                               const e_switch_block_type& sb_type, 
                                               const int& Fs,
                                               const e_switch_block_type& sb_subtype, 
                                               const int& subFs,
                                               const bool& wire_opposite_side,
                                               const std::vector<t_segment_inf>& segment_inf) {
  t_track2track_map track2track_map; /* [0..gsb_side][0..chan_width][track_indices] */

  /* Categorize tracks into 3 groups: 
   * (1) tracks will start here 
   * (2) tracks will end here 
   * (2) tracks will just pass through the SB */
  t_track_group start_tracks; /* [0..gsb_side][track_indices] */
  t_track_group end_tracks; /* [0..gsb_side][track_indices] */
  t_track_group pass_tracks; /* [0..gsb_side][track_indices] */

  /* resize to the number of sides */
  start_tracks.resize(rr_gsb.get_num_sides());
  end_tracks.resize(rr_gsb.get_num_sides());
  pass_tracks.resize(rr_gsb.get_num_sides());

  /* Walk through each side */
  for (size_t side = 0; side < rr_gsb.get_num_sides(); ++side) {
    SideManager side_manager(side);
    e_side gsb_side = side_manager.get_side();
    /* Build a list of tracks that will start from this side */
    for (size_t inode = 0; inode < rr_gsb.get_chan_width(gsb_side); ++inode) {
      /* We need to check Switch block population of this track 
       * The track node will not be considered if there supposed to be no SB at this position 
       */
      if (false == is_gsb_in_track_sb_population(rr_graph, rr_gsb, gsb_side, inode, segment_inf)) {
        continue; /* skip this node and go to the next */
      }
      /* check if this track will start from here */
      enum e_track_status track_status = determine_track_status_of_gsb(rr_graph, rr_gsb, gsb_side, inode);

      switch (track_status) {
      case TRACK_START:
        /* update starting track list */
        start_tracks[side].push_back(inode);
        break;
      case TRACK_END:
        /* Update end track list */
        end_tracks[side].push_back(inode);
        break;
      case TRACK_PASS:
        /* Update passing track list */
        /* Note that the pass_track should be IN_PORT only !!! */
        if (IN_PORT == rr_gsb.get_chan_node_direction(gsb_side, inode)) {
          pass_tracks[side].push_back(inode);
        }
        break;
      default:
        VTR_LOGF_ERROR(__FILE__, __LINE__,
                       "Invalid track status!\n");
        exit(1);
      }
    }
  }

  /* Allocate track2track map */
  track2track_map.resize(rr_gsb.get_num_sides());
  for (size_t side = 0; side < rr_gsb.get_num_sides(); ++side) {
    SideManager side_manager(side);
    enum e_side gsb_side = side_manager.get_side();
    /* allocate track2track_map[gsb_side] */
    track2track_map[side].resize(rr_gsb.get_chan_width(gsb_side));
    for (size_t inode = 0; inode < rr_gsb.get_chan_width(gsb_side); ++inode) {
      /* allocate track2track_map[gsb_side][inode] */
      track2track_map[side][inode].clear();
    }
  }

  /* For Group 1: we build connections between end_tracks and start_tracks*/
  build_gsb_one_group_track_to_track_map(rr_graph, rr_gsb, 
                                         sb_type, Fs,
                                         true, /* End tracks should always to wired to start tracks */ 
                                         end_tracks, start_tracks,
                                         track2track_map);

  /* For Group 2: we build connections between end_tracks and start_tracks*/
  /* Currently, I use the same Switch Block pattern for the passing tracks and end tracks,
   * TODO: This can be improved with different patterns! 
   */
  build_gsb_one_group_track_to_track_map(rr_graph, rr_gsb, 
                                         sb_subtype, subFs,
                                         wire_opposite_side, /* Pass tracks may not be wired to start tracks */ 
                                         pass_tracks, start_tracks, 
                                         track2track_map);

  return track2track_map;
}

/* Build a RRChan Object with the given channel type and coorindators */
static 
RRChan build_one_tileable_rr_chan(const vtr::Point<size_t>& chan_coordinate, 
                                  const t_rr_type& chan_type, 
                                  const RRGraph& rr_graph, 
                                  const ChanNodeDetails& chan_details) {
  std::vector<RRNodeId> chan_rr_nodes;

  /* Create a rr_chan object and check if it is unique in the graph */
  RRChan rr_chan;

  /* Fill the information */
  rr_chan.set_type(chan_type); 

  /* Collect rr_nodes for this channel */
  chan_rr_nodes = find_rr_graph_chan_nodes(rr_graph,
                                           chan_coordinate.x(), chan_coordinate.y(),
                                           chan_type);

  /* Reserve */
  /* rr_chan.reserve_node(size_t(chan_width)); */

  /* Fill the rr_chan */  
  for (size_t itrack = 0; itrack < chan_rr_nodes.size(); ++itrack) {
    size_t iseg = chan_details.get_track_segment_id(itrack); 
    rr_chan.add_node(rr_graph, chan_rr_nodes[itrack], RRSegmentId(iseg));
  }

  return rr_chan;
}

/***********************************************************************
 * Build a General Switch Block (GSB) 
 * which includes:
 * [I] A Switch Box subckt consists of following ports:
 * 1. Channel Y [x][y] inputs 
 * 2. Channel X [x+1][y] inputs
 * 3. Channel Y [x][y-1] outputs
 * 4. Channel X [x][y] outputs
 * 5. Grid[x][y+1] Right side outputs pins
 * 6. Grid[x+1][y+1] Left side output pins
 * 7. Grid[x+1][y+1] Bottom side output pins
 * 8. Grid[x+1][y] Top side output pins
 * 9. Grid[x+1][y] Left side output pins
 * 10. Grid[x][y] Right side output pins
 * 11. Grid[x][y] Top side output pins
 * 12. Grid[x][y+1] Bottom side output pins
 *
 *    --------------          --------------
 *    |            |   CBY    |            |
 *    |    Grid    |  ChanY   |    Grid    |
 *    |  [x][y+1]  | [x][y+1] | [x+1][y+1] |
 *    |            |          |            |
 *    --------------          --------------
 *                  ----------
 *     ChanX & CBX  | Switch |     ChanX 
 *       [x][y]     |   Box  |    [x+1][y]
 *                  | [x][y] |
 *                  ----------
 *    --------------          --------------
 *    |            |          |            |
 *    |    Grid    |  ChanY   |    Grid    |
 *    |   [x][y]   |  [x][y]  |  [x+1][y]  |
 *    |            |          |            |
 *    --------------          --------------
 * For channels chanY with INC_DIRECTION on the top side, they should be marked as outputs
 * For channels chanY with DEC_DIRECTION on the top side, they should be marked as inputs
 * For channels chanY with INC_DIRECTION on the bottom side, they should be marked as inputs
 * For channels chanY with DEC_DIRECTION on the bottom side, they should be marked as outputs
 * For channels chanX with INC_DIRECTION on the left side, they should be marked as inputs
 * For channels chanX with DEC_DIRECTION on the left side, they should be marked as outputs
 * For channels chanX with INC_DIRECTION on the right side, they should be marked as outputs
 * For channels chanX with DEC_DIRECTION on the right side, they should be marked as inputs
 *
 * [II] A X-direction Connection Block [x][y]
 * The connection block shares the same routing channel[x][y] with the Switch Block
 * We just need to fill the ipin nodes at TOP and BOTTOM sides 
 * as well as properly fill the ipin_grid_side information
 * [III] A Y-direction Connection Block [x][y+1]
 * The connection block shares the same routing channel[x][y+1] with the Switch Block
 * We just need to fill the ipin nodes at LEFT and RIGHT sides 
 * as well as properly fill the ipin_grid_side information
 ***********************************************************************/
RRGSB build_one_tileable_rr_gsb(const DeviceGrid& grids, 
                                const RRGraph& rr_graph,
                                const vtr::Point<size_t>& device_chan_width, 
                                const std::vector<t_segment_inf>& segment_inf,
                                const vtr::Point<size_t>& gsb_coordinate) {
  /* Create an object to return */
  RRGSB rr_gsb;

  /* Check */
  VTR_ASSERT(gsb_coordinate.x() <= grids.width()); 
  VTR_ASSERT(gsb_coordinate.y() <= grids.height()); 

  /* Coordinator initialization */
  rr_gsb.set_coordinate(gsb_coordinate.x(), gsb_coordinate.y());

  /* Basic information*/
  rr_gsb.init_num_sides(4); /* Fixed number of sides */

  /* Find all rr_nodes of channels */
  /* SideManager: TOP => 0, RIGHT => 1, BOTTOM => 2, LEFT => 3 */
  for (size_t side = 0; side < rr_gsb.get_num_sides(); ++side) {
    /* Local variables inside this for loop */
    SideManager side_manager(side);
    vtr::Point<size_t> coordinate = rr_gsb.get_side_block_coordinate(side_manager.get_side());
    RRChan rr_chan;
    std::vector<std::vector<RRNodeId>> temp_opin_rr_nodes(2);
    enum e_side opin_grid_side[2] = {NUM_SIDES, NUM_SIDES};
    enum PORTS chan_dir_to_port_dir_mapping[2] = {OUT_PORT, IN_PORT}; /* 0: INC_DIRECTION => ?; 1: DEC_DIRECTION => ? */

    /* Build a segment details, where we need the segment ids for building rr_chan  
     * We do not care starting and ending points here, so set chan_side as NUM_SIDES 
     */
    ChanNodeDetails chanx_details = build_unidir_chan_node_details(device_chan_width.x(), grids.width() - 1, 
                                                                   false, false, segment_inf); 
    ChanNodeDetails chany_details = build_unidir_chan_node_details(device_chan_width.y(), grids.height() - 1, 
                                                                   false, false, segment_inf); 

    switch (side) {
    case TOP: /* TOP = 0 */
      /* For the bording, we should take special care */
      if (gsb_coordinate.y() == grids.height() - 1) {
        rr_gsb.clear_one_side(side_manager.get_side());
        break;
      }
      /* Routing channels*/
      /* SideManager: TOP => 0, RIGHT => 1, BOTTOM => 2, LEFT => 3 */
      /* Create a rr_chan object and check if it is unique in the graph */
      rr_chan = build_one_tileable_rr_chan(coordinate, CHANY, rr_graph, chany_details);
      chan_dir_to_port_dir_mapping[0] = OUT_PORT; /* INC_DIRECTION => OUT_PORT */
      chan_dir_to_port_dir_mapping[1] =  IN_PORT; /* DEC_DIRECTION => IN_PORT */

      /* Assign grid side of OPIN */
      /* Grid[x][y+1] RIGHT side outputs pins */
      opin_grid_side[0] = RIGHT;
      /* Grid[x+1][y+1] left side outputs pins */
      opin_grid_side[1] = LEFT; 

      /* Build the Switch block: opin and opin_grid_side */
      /* Include Grid[x][y+1] RIGHT side outputs pins */
      temp_opin_rr_nodes[0] = find_rr_graph_grid_nodes(rr_graph, grids, 
                                                       gsb_coordinate.x(), gsb_coordinate.y() + 1,
                                                       OPIN, opin_grid_side[0]);
      /* Include Grid[x+1][y+1] Left side output pins */
      temp_opin_rr_nodes[1] = find_rr_graph_grid_nodes(rr_graph, grids, 
                                                       gsb_coordinate.x() + 1, gsb_coordinate.y() + 1,
                                                       OPIN, opin_grid_side[1]);

      break;
    case RIGHT: /* RIGHT = 1 */
      /* For the bording, we should take special care */
      if (gsb_coordinate.x() == grids.width() - 1) {
        rr_gsb.clear_one_side(side_manager.get_side());
        break;
      }
      /* Routing channels*/
      /* SideManager: TOP => 0, RIGHT => 1, BOTTOM => 2, LEFT => 3 */
      /* Collect rr_nodes for Tracks for top: chany[x][y+1] */
      /* Create a rr_chan object and check if it is unique in the graph */
      rr_chan = build_one_tileable_rr_chan(coordinate, CHANX, rr_graph, chanx_details);
      chan_dir_to_port_dir_mapping[0] = OUT_PORT; /* INC_DIRECTION => OUT_PORT */
      chan_dir_to_port_dir_mapping[1] =  IN_PORT; /* DEC_DIRECTION => IN_PORT */

      /* Assign grid side of OPIN */
      /* Grid[x+1][y+1] BOTTOM side outputs pins */
      opin_grid_side[0] = BOTTOM;
      /* Grid[x+1][y] TOP side outputs pins */
      opin_grid_side[1] = TOP;

      /* Build the Switch block: opin and opin_grid_side */
      /* include Grid[x+1][y+1] Bottom side output pins */
      temp_opin_rr_nodes[0] = find_rr_graph_grid_nodes(rr_graph, grids, 
                                                       gsb_coordinate.x() + 1, gsb_coordinate.y() + 1,
                                                       OPIN, opin_grid_side[0]);
      /* include Grid[x+1][y] Top side output pins */
      temp_opin_rr_nodes[1] = find_rr_graph_grid_nodes(rr_graph, grids, 
                                                       gsb_coordinate.x() + 1, gsb_coordinate.y(),
                                                       OPIN, opin_grid_side[1]);
      break;
    case BOTTOM: /* BOTTOM = 2*/
      /* For the bording, we should take special care */
      if (gsb_coordinate.y() == 0) {
        rr_gsb.clear_one_side(side_manager.get_side());
        break;
      }
      /* Routing channels*/
      /* SideManager: TOP => 0, RIGHT => 1, BOTTOM => 2, LEFT => 3 */
      /* Collect rr_nodes for Tracks for bottom: chany[x][y] */
      /* Create a rr_chan object and check if it is unique in the graph */
      rr_chan = build_one_tileable_rr_chan(coordinate, CHANY, rr_graph, chany_details);
      chan_dir_to_port_dir_mapping[0] =  IN_PORT; /* INC_DIRECTION => IN_PORT */
      chan_dir_to_port_dir_mapping[1] = OUT_PORT; /* DEC_DIRECTION => OUT_PORT */

      /* Assign grid side of OPIN */
      /* Grid[x+1][y] LEFT side outputs pins */
      opin_grid_side[0] = LEFT;
      /* Grid[x][y] RIGHT side outputs pins */
      opin_grid_side[1] = RIGHT;

      /* Build the Switch block: opin and opin_grid_side */
      /* include Grid[x+1][y] Left side output pins */
      temp_opin_rr_nodes[0] = find_rr_graph_grid_nodes(rr_graph, grids, 
                                                       gsb_coordinate.x() + 1, gsb_coordinate.y(),
                                                       OPIN, opin_grid_side[0]);
      /* include Grid[x][y] Right side output pins */
      temp_opin_rr_nodes[1] = find_rr_graph_grid_nodes(rr_graph, grids, 
                                                       gsb_coordinate.x(), gsb_coordinate.y(),
                                                       OPIN, opin_grid_side[1]);
      break;
    case LEFT: /* LEFT = 3 */
      /* For the bording, we should take special care */
      if (gsb_coordinate.x() == 0) {
        rr_gsb.clear_one_side(side_manager.get_side());
        break;
      }
      /* Routing channels*/
      /* SideManager: TOP => 0, RIGHT => 1, BOTTOM => 2, LEFT => 3 */
      /* Collect rr_nodes for Tracks for left: chanx[x][y] */
      /* Create a rr_chan object and check if it is unique in the graph */
      rr_chan = build_one_tileable_rr_chan(coordinate, CHANX, rr_graph, chanx_details);
      chan_dir_to_port_dir_mapping[0] =  IN_PORT; /* INC_DIRECTION => IN_PORT */
      chan_dir_to_port_dir_mapping[1] = OUT_PORT; /* DEC_DIRECTION => OUT_PORT */

      /* Grid[x][y+1] BOTTOM side outputs pins */
      opin_grid_side[0] = BOTTOM;
      /* Grid[x][y] TOP side outputs pins */
      opin_grid_side[1] = TOP;

      /* Build the Switch block: opin and opin_grid_side */
      /* include Grid[x][y+1] Bottom side outputs pins */
      temp_opin_rr_nodes[0] = find_rr_graph_grid_nodes(rr_graph, grids, 
                                                       gsb_coordinate.x(), gsb_coordinate.y() + 1,
                                                       OPIN, opin_grid_side[0]);
      /* include Grid[x][y] Top side output pins */
      temp_opin_rr_nodes[1] = find_rr_graph_grid_nodes(rr_graph, grids, 
                                                       gsb_coordinate.x(), gsb_coordinate.y(),
                                                       OPIN, opin_grid_side[1]);

      break;
    default:
      VTR_LOGF_ERROR(__FILE__, __LINE__,
                     "Invalid side index!\n");
      exit(1);
    }

    /* Organize a vector of port direction */
    if (0 < rr_chan.get_chan_width()) {
      std::vector<enum PORTS> rr_chan_dir;
      rr_chan_dir.resize(rr_chan.get_chan_width());
      for (size_t itrack = 0; itrack < rr_chan.get_chan_width(); ++itrack) {
        /* Identify the directionality, record it in rr_node_direction */
        if (Direction::INC == rr_graph.node_direction(rr_chan.get_node(itrack))) {
          rr_chan_dir[itrack] = chan_dir_to_port_dir_mapping[0];
        } else {
          VTR_ASSERT(Direction::DEC == rr_graph.node_direction(rr_chan.get_node(itrack)));
          rr_chan_dir[itrack] = chan_dir_to_port_dir_mapping[1];
        }
      }
      /* Fill chan_rr_nodes */
      rr_gsb.add_chan_node(side_manager.get_side(), rr_chan, rr_chan_dir);
    }

    /* Fill opin_rr_nodes */
    /* Copy from temp_opin_rr_node to opin_rr_node */
    for (const RRNodeId& inode : temp_opin_rr_nodes[0]) {
      /* Grid[x+1][y+1] Bottom side outputs pins */
      rr_gsb.add_opin_node(inode, side_manager.get_side());
    }
    for (const RRNodeId& inode : temp_opin_rr_nodes[1]) {
      /* Grid[x+1][y] TOP side outputs pins */
      rr_gsb.add_opin_node(inode, side_manager.get_side());
    }

    /* Clean ipin_rr_nodes */
    /* We do not have any IPIN for a Switch Block */
    rr_gsb.clear_ipin_nodes(side_manager.get_side());

    /* Clear the temp data */
    temp_opin_rr_nodes[0].clear();
    temp_opin_rr_nodes[1].clear();
    opin_grid_side[0] = NUM_SIDES;
    opin_grid_side[1] = NUM_SIDES;
  }

  /* Add IPIN nodes from adjacent grids: the 4 grids sitting on the 4 corners of the Switch Block 
   *
   * - The concept of top/bottom side of connection block in GSB domain:
   *
   *     |    Grid[x][y+1]       |
   *     |      BOTTOM side      |
   *     +-----------------------+
   *                |
   *                v
   *     +-----------------------+
   *     |        TOP side       |
   *     |  X- Connection Block  |
   *     |     BOTTOM side       |
   *     +-----------------------+
   *                ^
   *                |
   *     +-----------------------+
   *     |      TOP side         |
   *     |    Grid[x][y]         |
   *
   * - The concept of top/bottom side of connection block in GSB domain:
   *
   *   ---------------+  +---------------------- ... ---------------------+  +----------------
   *     Grid[x][y+1] |->| Y- Connection Block        Y- Connection Block |<-| Grid[x+1][y+1]
   *    RIGHT side    |  | LEFT side             ...  RIGHT side          |  | LEFT side
   *    --------------+  +---------------------- ... ---------------------+  +----------------
   *
   */
  for (size_t side = 0; side < rr_gsb.get_num_sides(); ++side) {
    SideManager side_manager(side);
    size_t ix; 
    size_t iy; 
    enum e_side chan_side;
    std::vector<RRNodeId> temp_ipin_rr_nodes;
    enum e_side ipin_rr_node_grid_side;
   
    switch (side) {
    case TOP:
      /* Consider the routing channel that is connected to the left side of the switch block */
      chan_side = LEFT;
      /* The input pins of the routing channel come from the bottom side of Grid[x][y+1] */
      ix = rr_gsb.get_sb_x(); 
      iy = rr_gsb.get_sb_y() + 1; 
      ipin_rr_node_grid_side = BOTTOM; 
      break;
    case RIGHT:
      /* Consider the routing channel that is connected to the top side of the switch block */
      chan_side = TOP;
      /* The input pins of the routing channel come from the left side of Grid[x+1][y+1] */
      ix = rr_gsb.get_sb_x() + 1; 
      iy = rr_gsb.get_sb_y() + 1; 
      ipin_rr_node_grid_side = LEFT; 
      break;
    case BOTTOM:
      /* Consider the routing channel that is connected to the left side of the switch block */
      chan_side = LEFT;
      /* The input pins of the routing channel come from the top side of Grid[x][y] */
      ix = rr_gsb.get_sb_x(); 
      iy = rr_gsb.get_sb_y(); 
      ipin_rr_node_grid_side = TOP; 
      break;
    case LEFT:
      /* Consider the routing channel that is connected to the top side of the switch block */
      chan_side = TOP;
      /* The input pins of the routing channel come from the right side of Grid[x][y+1] */
      ix = rr_gsb.get_sb_x(); 
      iy = rr_gsb.get_sb_y() + 1; 
      ipin_rr_node_grid_side = RIGHT; 
      break;
    default:
      VTR_LOGF_ERROR(__FILE__, __LINE__,
                     "Invalid side index!\n");
      exit(1);
    }
    
    /* If there is no channel at this side, we skip ipin_node annotation */
    if (0 == rr_gsb.get_chan_width(chan_side)) {
      continue;
    }
    /* Collect IPIN rr_nodes*/ 
    temp_ipin_rr_nodes = find_rr_graph_grid_nodes(rr_graph, grids, 
                                                  ix, iy, IPIN, ipin_rr_node_grid_side);
    /* Fill the ipin nodes of RRGSB */ 
    for (const RRNodeId& inode : temp_ipin_rr_nodes) {
      rr_gsb.add_ipin_node(inode, side_manager.get_side());
    }
    /* Clear the temp data */
    temp_ipin_rr_nodes.clear();
  }

  return rr_gsb;
}

/************************************************************************
 * Create edges for each rr_node of a General Switch Blocks (GSB):
 * 1. create edges between CHANX | CHANY and IPINs (connections inside connection blocks) 
 * 2. create edges between OPINs, CHANX and CHANY (connections inside switch blocks) 
 * 3. create edges between OPINs and IPINs (direct-connections) 
 ***********************************************************************/
void build_edges_for_one_tileable_rr_gsb(RRGraph& rr_graph, 
                                         const RRGSB& rr_gsb,
                                         const t_track2pin_map& track2ipin_map,
                                         const t_pin2track_map& opin2track_map,
                                         const t_track2track_map& track2track_map,
                                         const vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches) {
  
  /* Walk through each sides */ 
  for (size_t side = 0; side < rr_gsb.get_num_sides(); ++side) {
    SideManager side_manager(side);
    enum e_side gsb_side = side_manager.get_side();

    /* Find OPINs */  
    for (size_t inode = 0; inode < rr_gsb.get_num_opin_nodes(gsb_side); ++inode) {
      const RRNodeId& opin_node = rr_gsb.get_opin_node(gsb_side, inode); 

      /* 1. create edges between OPINs and CHANX|CHANY, using opin2track_map */
      /* add edges to the opin_node */
      for (const RRNodeId& track_node : opin2track_map[gsb_side][inode]) {
        rr_graph.create_edge(opin_node, track_node, rr_node_driver_switches[track_node]);
      }
    }

    /* Find  CHANX or CHANY */
    /* For TRACKs to IPINs, we only care LEFT and TOP sides 
     * Skip RIGHT and BOTTOM for the ipin2track_map since they should be handled in other GSBs 
     */
    if ( (side_manager.get_side() == rr_gsb.get_cb_chan_side(CHANX))
      || (side_manager.get_side() == rr_gsb.get_cb_chan_side(CHANY)) ) {
      /* 2. create edges between CHANX|CHANY and IPINs, using ipin2track_map */
      for (size_t inode = 0; inode < rr_gsb.get_chan_width(gsb_side); ++inode) {
        const RRNodeId& chan_node = rr_gsb.get_chan_node(gsb_side, inode); 
        for (const RRNodeId& ipin_node : track2ipin_map[gsb_side][inode]) {
          rr_graph.create_edge(chan_node, ipin_node, rr_node_driver_switches[ipin_node]);
        }
      }
    }

    /* 3. create edges between CHANX|CHANY and CHANX|CHANY, using track2track_map */
    for (size_t inode = 0; inode < rr_gsb.get_chan_width(gsb_side); ++inode) {
      const RRNodeId& chan_node = rr_gsb.get_chan_node(gsb_side, inode); 
      for (const RRNodeId& track_node : track2track_map[gsb_side][inode]) {
        rr_graph.create_edge(chan_node, track_node, rr_node_driver_switches[track_node]);
      }
    }
  }
}

/************************************************************************
 * Build track2ipin_map for an IPIN  
 * 1. build a list of routing tracks which are allowed for connections
 *    We will check the Connection Block (CB) population of each routing track. 
 *    By comparing current chan_y - ylow, we can determine if a CB connection
 *    is required for each routing track
 * 2. Divide the routing tracks by segment types, so that we can balance
 *    the connections between IPINs and different types of routing tracks.
 * 3. Scale the Fc of each pin to the actual number of routing tracks
 *    actual_Fc = (int) Fc * num_tracks / chan_width
 ***********************************************************************/
static 
void build_gsb_one_ipin_track2pin_map(const RRGraph& rr_graph, 
                                      const RRGSB& rr_gsb, 
                                      const enum e_side& ipin_side, 
                                      const size_t& ipin_node_id, 
                                      const std::vector<int>& Fc, 
                                      const size_t& offset, 
                                      const std::vector<t_segment_inf>& segment_inf, 
                                      t_track2pin_map& track2ipin_map) {
  /* Get a list of segment_ids*/
  enum e_side chan_side = rr_gsb.get_cb_chan_side(ipin_side);
  SideManager chan_side_manager(chan_side);
  std::vector<RRSegmentId> seg_list = rr_gsb.get_chan_segment_ids(chan_side);
  size_t chan_width = rr_gsb.get_chan_width(chan_side);
  SideManager ipin_side_manager(ipin_side);
  const RRNodeId& ipin_node = rr_gsb.get_ipin_node(ipin_side, ipin_node_id);

  for (size_t iseg = 0; iseg < seg_list.size(); ++iseg) {
    /* Get a list of node that have the segment id */
    std::vector<size_t> track_list = rr_gsb.get_chan_node_ids_by_segment_ids(chan_side, seg_list[iseg]);
    /* Refine the track_list: keep those will have connection blocks in the GSB */
    std::vector<size_t> actual_track_list;
    for (size_t inode = 0; inode < track_list.size(); ++inode) {
      /* Check if tracks allow connection blocks in the GSB*/
      if (false == is_gsb_in_track_cb_population(rr_graph, rr_gsb, chan_side, track_list[inode], segment_inf)) {
         continue; /* Bypass condition */
      }
      /* Push the node to actual_track_list  */
      actual_track_list.push_back(track_list[inode]);
    }
    /* Check the actual track list */
    VTR_ASSERT(0 == actual_track_list.size() % 2);
   
    /* Scale Fc  */
    int actual_Fc = std::ceil((float)Fc[iseg] * (float)actual_track_list.size() / (float)chan_width); 
    /* Minimum Fc should be 2 : ensure we will connect to a pair of routing tracks */
    actual_Fc = std::max(1, actual_Fc);
    /* Compute the step between two connection from this IPIN to tracks: 
     * step = W' / Fc', W' and Fc' are the adapted W and Fc from actual_track_list and Fc_in 
     */
    size_t track_step = std::floor((float)actual_track_list.size() / (float)actual_Fc);
    /* Make sure step should be at least 2 */
    track_step = std::max(1, (int)track_step);
    /* Adapt offset to the range of actual_track_list */
    size_t actual_offset = offset % actual_track_list.size();
    /* rotate the track list by an offset */
    if (0 < actual_offset) {
      std::rotate(actual_track_list.begin(), actual_track_list.begin() + actual_offset, actual_track_list.end());   
    }

    /* Assign tracks: since we assign 2 track per round, we increment itrack by 2* step  */
    int track_cnt = 0;
    /* Keep assigning until we meet the Fc requirement */
    for (size_t itrack = 0; itrack < actual_track_list.size(); itrack = itrack + 2 * track_step) {
      /* Update pin2track map */ 
      size_t chan_side_index = chan_side_manager.to_size_t();
      /* itrack may exceed the size of actual_track_list, adapt it */
      size_t actual_itrack = itrack % actual_track_list.size();
      /* track_index may exceed the chan_width(), adapt it */
      size_t track_index = actual_track_list[actual_itrack] % chan_width;

      track2ipin_map[chan_side_index][track_index].push_back(ipin_node);

      /* track_index may exceed the chan_width(), adapt it */
      track_index = (actual_track_list[actual_itrack] + 1) % chan_width;

      track2ipin_map[chan_side_index][track_index].push_back(ipin_node);

      track_cnt += 2;
    }

    /* Ensure the number of tracks is similar to Fc */
    /* Give a warning if Fc is < track_cnt */
    /*
    if (actual_Fc != track_cnt) {
      vpr_printf(TIO_MESSAGE_INFO, 
                 "IPIN Node(%lu) will have a different Fc(=%lu) than specified(=%lu)!\n",
                 ipin_node - rr_graph->rr_node, track_cnt, actual_Fc);
    }
    */
  }
}

/************************************************************************
 * Build opin2track_map for an OPIN  
 * 1. build a list of routing tracks which are allowed for connections
 *    We will check the Switch Block (SB) population of each routing track. 
 *    By comparing current chan_y - ylow, we can determine if a SB connection
 *    is required for each routing track
 * 2. Divide the routing tracks by segment types, so that we can balance
 *    the connections between OPINs and different types of routing tracks.
 * 3. Scale the Fc of each pin to the actual number of routing tracks
 *    actual_Fc = (int) Fc * num_tracks / chan_width
 ***********************************************************************/
static 
void build_gsb_one_opin_pin2track_map(const RRGraph& rr_graph, 
                                      const RRGSB& rr_gsb, 
                                      const enum e_side& opin_side, 
                                      const size_t& opin_node_id, 
                                      const std::vector<int>& Fc,
                                      const size_t& offset, 
                                      const std::vector<t_segment_inf>& segment_inf, 
                                      t_pin2track_map& opin2track_map) {
  /* Get a list of segment_ids*/
  std::vector<RRSegmentId> seg_list = rr_gsb.get_chan_segment_ids(opin_side);
  enum e_side chan_side = opin_side;
  size_t chan_width = rr_gsb.get_chan_width(chan_side);
  SideManager opin_side_manager(opin_side);

  for (size_t iseg = 0; iseg < seg_list.size(); ++iseg) {
    /* Get a list of node that have the segment id */
    std::vector<size_t> track_list = rr_gsb.get_chan_node_ids_by_segment_ids(chan_side, seg_list[iseg]);
    /* Refine the track_list: keep those will have connection blocks in the GSB */
    std::vector<size_t> actual_track_list;
    for (size_t inode = 0; inode < track_list.size(); ++inode) {
      /* Check if tracks allow connection blocks in the GSB*/
      if (false == is_gsb_in_track_sb_population(rr_graph, rr_gsb, chan_side, 
                                                 track_list[inode], segment_inf)) {
         continue; /* Bypass condition */
      }
      if (TRACK_START != determine_track_status_of_gsb(rr_graph, rr_gsb, chan_side, track_list[inode])) {
         continue; /* Bypass condition */
      }
      /* Push the node to actual_track_list  */
      actual_track_list.push_back(track_list[inode]);
    }

    /* Go the next segment if offset is zero or actual_track_list is empty */    
    if (0 == actual_track_list.size()) {
      continue;
    }
   
    /* Scale Fc  */
    int actual_Fc = std::ceil((float)Fc[iseg] * (float)actual_track_list.size() / (float)chan_width); 
    /* Minimum Fc should be 1 : ensure we will drive 1 routing track */
    actual_Fc = std::max(1, actual_Fc);
    /* Compute the step between two connection from this IPIN to tracks: 
     * step = W' / Fc', W' and Fc' are the adapted W and Fc from actual_track_list and Fc_in 
    */
    size_t track_step = std::floor((float)actual_track_list.size() / (float)actual_Fc);
    /* Track step mush be a multiple of 2!!!*/
    /* Make sure step should be at least 1 */
    track_step = std::max(1, (int)track_step);
    /* Adapt offset to the range of actual_track_list */
    size_t actual_offset = offset % actual_track_list.size();

    /* No need to rotate if offset is zero */    
    if (0 < actual_offset) {
      /* rotate the track list by an offset */
      std::rotate(actual_track_list.begin(), actual_track_list.begin() + actual_offset, actual_track_list.end());   
    }

    /* Assign tracks  */
    int track_cnt = 0;
    /* Keep assigning until we meet the Fc requirement */
    for (size_t itrack = 0; itrack < actual_track_list.size(); itrack = itrack + track_step) {
      /* Update pin2track map */ 
      size_t opin_side_index = opin_side_manager.to_size_t();
      /* itrack may exceed the size of actual_track_list, adapt it */
      size_t actual_itrack = itrack % actual_track_list.size();
      size_t track_index = actual_track_list[actual_itrack];
      const RRNodeId& track_rr_node_index = rr_gsb.get_chan_node(chan_side, track_index);
      opin2track_map[opin_side_index][opin_node_id].push_back(track_rr_node_index);
      /* update track counter */
      track_cnt++;
      /* Stop when we have enough Fc: this may lead to some tracks have zero drivers. 
       * So I comment it. And we just make sure its track_cnt >= actual_Fc
      if (actual_Fc == track_cnt) {
        break;
      }
      */
    }

    /* Ensure the number of tracks is similar to Fc */
    /* Give a warning if Fc is < track_cnt */
    /*
    if (actual_Fc != track_cnt) {
      vpr_printf(TIO_MESSAGE_INFO, 
                 "OPIN Node(%lu) will have a different Fc(=%lu) than specified(=%lu)!\n",
                 opin_node_id, track_cnt, actual_Fc);
    }
    */
  }
}


/************************************************************************
 * Build the track_to_ipin_map[gsb_side][0..chan_width-1][ipin_indices] 
 * based on the existing routing resources in the General Switch Block (GSB)
 * This function supports both X-directional and Y-directional tracks 
 * The mapping is done in the following steps:
 * 1. Build ipin_to_track_map[gsb_side][0..num_ipin_nodes-1][track_indices]
 *    For each IPIN, we ensure at least one connection to the tracks.
 *    Then, we assign IPINs to tracks evenly while satisfying the actual_Fc 
 * 2. Convert the ipin_to_track_map to track_to_ipin_map
 ***********************************************************************/
t_track2pin_map build_gsb_track_to_ipin_map(const RRGraph& rr_graph,
                                            const RRGSB& rr_gsb, 
                                            const DeviceGrid& grids, 
                                            const std::vector<t_segment_inf>& segment_inf, 
                                            const std::vector<vtr::Matrix<int>>& Fc_in) {
  t_track2pin_map track2ipin_map;
  /* Resize the matrix */ 
  track2ipin_map.resize(rr_gsb.get_num_sides());
  
  /* offset counter: it aims to balance the track-to-IPIN for each connection block */
  size_t offset_size = 0;
  std::vector<size_t> offset;
  for (size_t side = 0; side < rr_gsb.get_num_sides(); ++side) {
    SideManager side_manager(side);
    enum e_side ipin_side = side_manager.get_side();
    /* Get the chan_side */
    enum e_side chan_side = rr_gsb.get_cb_chan_side(ipin_side);
    SideManager chan_side_manager(chan_side);
    /* resize offset to the maximum chan_side*/
    offset_size = std::max(offset_size, chan_side_manager.to_size_t() + 1);
  }
  /* Initial offset */
  offset.resize(offset_size);
  offset.assign(offset.size(), 0);
     
  /* Walk through each side */
  for (size_t side = 0; side < rr_gsb.get_num_sides(); ++side) {
    SideManager side_manager(side);
    enum e_side ipin_side = side_manager.get_side();
    /* Get the chan_side */
    enum e_side chan_side = rr_gsb.get_cb_chan_side(ipin_side);
    SideManager chan_side_manager(chan_side);
    /* This track2pin mapping is for Connection Blocks, so we only care two sides! */
    /* Get channel width and resize the matrix */
    size_t chan_width = rr_gsb.get_chan_width(chan_side);
    track2ipin_map[chan_side_manager.to_size_t()].resize(chan_width); 
    /* Find the ipin/opin nodes */
    for (size_t inode = 0; inode < rr_gsb.get_num_ipin_nodes(ipin_side); ++inode) {
      const RRNodeId& ipin_node = rr_gsb.get_ipin_node(ipin_side, inode);
      /* Skip EMPTY type */
      if (true == is_empty_type(grids[rr_graph.node_xlow(ipin_node)][rr_graph.node_ylow(ipin_node)].type)) {
        continue;
      }

      int grid_type_index = grids[rr_graph.node_xlow(ipin_node)][rr_graph.node_ylow(ipin_node)].type->index; 
      /* Get Fc of the ipin */
      /* skip Fc = 0 or unintialized, those pins are in the <directlist> */
      bool skip_conn2track = true; 
      std::vector<int> ipin_Fc_out;
      for (size_t iseg = 0; iseg < segment_inf.size(); ++iseg) {
        int ipin_Fc = Fc_in[grid_type_index][rr_graph.node_pin_num(ipin_node)][iseg];
        ipin_Fc_out.push_back(ipin_Fc);
        if (0 != ipin_Fc) { 
          skip_conn2track = false;
          continue;
        }
      }

      if (true == skip_conn2track) {
        continue;
      }

      VTR_ASSERT(ipin_Fc_out.size() == segment_inf.size());

      /* Build track2ipin_map for this IPIN */
      build_gsb_one_ipin_track2pin_map(rr_graph, rr_gsb, ipin_side, inode, ipin_Fc_out, 
                                       /* Give an offset for the first track that this ipin will connect to */
                                       offset[chan_side_manager.to_size_t()], 
                                       segment_inf, track2ipin_map);
      /* update offset */
      offset[chan_side_manager.to_size_t()] += 2;
      //printf("offset[%lu]=%lu\n", chan_side_manager.to_size_t(), offset[chan_side_manager.to_size_t()]);
    }
  }

  return track2ipin_map;
}

/************************************************************************
 * Build the opin_to_track_map[gsb_side][0..num_opin_nodes-1][track_indices] 
 * based on the existing routing resources in the General Switch Block (GSB)
 * This function supports both X-directional and Y-directional tracks 
 * The mapping is done in the following steps:
 * 1. Build a list of routing tracks whose starting points locate at this GSB
 *    (xlow - gsb_x == 0)
 * 2. Divide the routing tracks by segment types, so that we can balance
 *    the connections between OPINs and different types of routing tracks.
 * 3. Scale the Fc of each pin to the actual number of routing tracks
 *    actual_Fc = (int) Fc * num_tracks / chan_width
 ***********************************************************************/
t_pin2track_map build_gsb_opin_to_track_map(const RRGraph& rr_graph,
                                            const RRGSB& rr_gsb, 
                                            const DeviceGrid& grids, 
                                            const std::vector<t_segment_inf>& segment_inf, 
                                            const std::vector<vtr::Matrix<int>>& Fc_out) {
  t_pin2track_map opin2track_map;
  /* Resize the matrix */ 
  opin2track_map.resize(rr_gsb.get_num_sides());
  
  /* offset counter: it aims to balance the OPIN-to-track for each switch block */
  std::vector<size_t> offset;
  /* Get the chan_side: which is the same as the opin side  */
  offset.resize(rr_gsb.get_num_sides());
  /* Initial offset */
  offset.assign(offset.size(), 0);
     
  /* Walk through each side */
  for (size_t side = 0; side < rr_gsb.get_num_sides(); ++side) {
    SideManager side_manager(side);
    enum e_side opin_side = side_manager.get_side();
    /* Get the chan_side */
    /* This track2pin mapping is for Connection Blocks, so we only care two sides! */
    /* Get channel width and resize the matrix */
    size_t num_opin_nodes = rr_gsb.get_num_opin_nodes(opin_side);
    opin2track_map[side].resize(num_opin_nodes); 
    /* Find the ipin/opin nodes */
    for (size_t inode = 0; inode < num_opin_nodes; ++inode) {
      const RRNodeId& opin_node = rr_gsb.get_opin_node(opin_side, inode);
      /* Skip EMPTY type */
      if (true == is_empty_type(grids[rr_graph.node_xlow(opin_node)][rr_graph.node_ylow(opin_node)].type)) {
        continue;
      }
      int grid_type_index = grids[rr_graph.node_xlow(opin_node)][rr_graph.node_ylow(opin_node)].type->index; 

      /* Get Fc of the ipin */
      /* skip Fc = 0 or unintialized, those pins are in the <directlist> */
      bool skip_conn2track = true; 
      std::vector<int> opin_Fc_out;
      for (size_t iseg = 0; iseg < segment_inf.size(); ++iseg) {
        int opin_Fc = Fc_out[grid_type_index][rr_graph.node_pin_num(opin_node)][iseg];
        opin_Fc_out.push_back(opin_Fc);
        if (0 != opin_Fc) { 
          skip_conn2track = false;
          continue;
        }
      }

      if (true == skip_conn2track) {
        continue;
      }
      VTR_ASSERT(opin_Fc_out.size() == segment_inf.size());

      /* Build track2ipin_map for this IPIN */
      build_gsb_one_opin_pin2track_map(rr_graph, rr_gsb, opin_side, inode, opin_Fc_out, 
                                       /* Give an offset for the first track that this ipin will connect to */
                                       offset[side_manager.to_size_t()], 
                                       segment_inf, opin2track_map);
      /* update offset: aim to rotate starting tracks by 1*/
      offset[side_manager.to_size_t()] += 1;
    }

    /* Check:
     * 1. We want to ensure that each OPIN will drive at least one track
     * 2. We want to ensure that each track will be driven by at least 1 OPIN */
  }

  return opin2track_map;
}

/************************************************************************
 * Add all direct clb-pin-to-clb-pin edges to given opin  
 ***********************************************************************/
void build_direct_connections_for_one_gsb(RRGraph& rr_graph,
                                          const DeviceGrid& grids,
                                          const vtr::Point<size_t>& from_grid_coordinate,
                                          const RRSwitchId& delayless_switch, 
                                          const std::vector<t_direct_inf>& directs, 
                                          const std::vector<t_clb_to_clb_directs>& clb_to_clb_directs) {
  VTR_ASSERT(directs.size() == clb_to_clb_directs.size());

  const t_grid_tile& from_grid = grids[from_grid_coordinate.x()][from_grid_coordinate.y()];
  t_physical_tile_type_ptr grid_type = from_grid.type;

  /* Iterate through all direct connections */
  for (size_t i = 0; i < directs.size(); ++i) {
    /* Bypass unmatched direct clb-to-clb connections */
    if (grid_type != clb_to_clb_directs[i].from_clb_type) {
      continue;
    }

    /* This opin is specified to connect directly to an ipin, 
     * now compute which ipin to connect to 
     */
    vtr::Point<size_t> to_grid_coordinate(from_grid_coordinate.x() + directs[i].x_offset, 
                                          from_grid_coordinate.y() + directs[i].y_offset);

    /* Bypass unmatched direct clb-to-clb connections */
    t_physical_tile_type_ptr to_grid_type = grids[to_grid_coordinate.x()][to_grid_coordinate.y()].type;
    /* Check if to_grid if the same grid */
    if (to_grid_type != clb_to_clb_directs[i].to_clb_type) {
      continue;
    }

    bool swap;
    int max_index, min_index;
    /* Compute index of opin with regards to given pins */ 
    if ( clb_to_clb_directs[i].from_clb_pin_start_index 
        > clb_to_clb_directs[i].from_clb_pin_end_index) {
      swap = true;
      max_index = clb_to_clb_directs[i].from_clb_pin_start_index;
      min_index = clb_to_clb_directs[i].from_clb_pin_end_index;
    } else {
      swap = false;
      min_index = clb_to_clb_directs[i].from_clb_pin_start_index;
      max_index = clb_to_clb_directs[i].from_clb_pin_end_index;
    }

    /* get every opin in the range */
    for (int opin = min_index; opin <= max_index; ++opin) {
      int offset = opin - min_index;

      if (  (to_grid_coordinate.x() < grids.width() - 1) 
         && (to_grid_coordinate.y() < grids.height() - 1) ) { 
        int ipin = OPEN;
        if ( clb_to_clb_directs[i].to_clb_pin_start_index 
          > clb_to_clb_directs[i].to_clb_pin_end_index) {
          if (true == swap) {
            ipin = clb_to_clb_directs[i].to_clb_pin_end_index + offset;
          } else {
            ipin = clb_to_clb_directs[i].to_clb_pin_start_index - offset;
          }
        } else {
          if(true == swap) {
            ipin = clb_to_clb_directs[i].to_clb_pin_end_index - offset;
          } else {
            ipin = clb_to_clb_directs[i].to_clb_pin_start_index + offset;
          }
        }

        /* Get the pin index in the rr_graph */
        int from_grid_width_ofs = from_grid.width_offset;
        int from_grid_height_ofs = from_grid.height_offset;
        int to_grid_width_ofs = grids[to_grid_coordinate.x()][to_grid_coordinate.y()].width_offset;
        int to_grid_height_ofs = grids[to_grid_coordinate.x()][to_grid_coordinate.y()].height_offset;
 
        /* Find the side of grid pins, the pin location should be unique!
         * Pin location is required by searching a node in rr_graph 
         */
        std::vector<e_side> opin_grid_side = find_grid_pin_sides(from_grid, opin); 
        VTR_ASSERT(1 == opin_grid_side.size());

        std::vector<e_side> ipin_grid_side = find_grid_pin_sides(grids[to_grid_coordinate.x()][to_grid_coordinate.y()], ipin); 
        VTR_ASSERT(1 == ipin_grid_side.size());

        const RRNodeId& opin_node_id = rr_graph.find_node(from_grid_coordinate.x() - from_grid_width_ofs, 
                                                          from_grid_coordinate.y() - from_grid_height_ofs, 
                                                          OPIN, opin, opin_grid_side[0]);
        const RRNodeId& ipin_node_id = rr_graph.find_node(to_grid_coordinate.x() - to_grid_width_ofs, 
                                                          to_grid_coordinate.y() - to_grid_height_ofs, 
                                                          IPIN, ipin, ipin_grid_side[0]);
        /*
        VTR_LOG("Direct connection: from grid[%lu][%lu].pin[%lu] at side %s to grid[%lu][%lu].pin[%lu] at side %s\n",
                from_grid_coordinate.x() - from_grid_width_ofs,
                from_grid_coordinate.y() - from_grid_height_ofs,
                opin, SIDE_STRING[opin_grid_side[0]],
                to_grid_coordinate.x() - to_grid_width_ofs,
                to_grid_coordinate.y() - to_grid_height_ofs,
                ipin, SIDE_STRING[ipin_grid_side[0]]);
         */
     
        /* add edges to the opin_node */
        rr_graph.create_edge(opin_node_id, ipin_node_id,
                             delayless_switch);
      }
    }
  }
}


} /* end namespace openfpga */
