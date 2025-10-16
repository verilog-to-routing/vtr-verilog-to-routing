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
#include "side_manager.h"

#include "vpr_utils.h"
#include "physical_types_util.h"
#include "rr_graph_view_util.h"
#include "tileable_rr_graph_utils.h"
#include "rr_graph_builder_utils.h"
#include "tileable_chan_details_builder.h"
#include "tileable_rr_graph_gsb.h"
#include "rr_graph_view.h"
#include "rr_graph_builder.h"
#include "vtr_geometry.h"

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
static enum e_track_status determine_track_status_of_gsb(const RRGraphView& rr_graph,
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
    if ((track_start.x() == side_coordinate.x())
        && (track_start.y() == side_coordinate.y())
        && (OUT_PORT == rr_gsb.get_chan_node_direction(gsb_side, track_id))) {
        /* Double check: start track should be an OUTPUT PORT of the GSB */
        track_status = TRACK_START;
    }

    /* Get the coordinate of where the track ends */
    vtr::Point<size_t> track_end = get_track_rr_node_end_coordinate(rr_graph, track_node);

    /* INC_DIRECTION end_track: (xhigh, yhigh) should be same as the GSB side coordinate */
    /* DEC_DIRECTION end_track: (xlow, ylow) should be same as the GSB side coordinate */
    if ((track_end.x() == side_coordinate.x())
        && (track_end.y() == side_coordinate.y())
        && (IN_PORT == rr_gsb.get_chan_node_direction(gsb_side, track_id))) {
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
static bool is_gsb_in_track_cb_population(const RRGraphView& rr_graph,
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
static bool is_gsb_in_track_sb_population(const RRGraphView& rr_graph,
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
static std::vector<size_t> get_to_track_list(const int& Fs, const int& to_track, const int& num_to_tracks) {
    std::vector<size_t> to_tracks;

    for (int i = 0; i < Fs; i = i + 3) {
        /* TODO: currently, for Fs > 3, I always search the next from_track until Fs is satisfied
         * The optimal track selection should be done in a more scientific way!!!
         */
        int to_track_i = to_track + i;
        /* make sure the track id is still in range */
        if (to_track_i > num_to_tracks - 1) {
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
static std::vector<size_t> get_switch_block_to_track_id(const e_switch_block_type& switch_block_type,
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
        case e_switch_block_type::SUBSET: /* NB:  Global routing uses SUBSET too */
            to_tracks = get_to_track_list(Fs, actual_from_track, num_to_tracks);
            /* Finish, we return */
            return to_tracks;
        case e_switch_block_type::UNIVERSAL:
            if ((from_side == LEFT)
                || (from_side == RIGHT)) {
                /* For the prev_side, to_track is from_track
                 * For the next_side, to_track is num_to_tracks - 1 - from_track
                 * For the opposite_side, to_track is always from_track
                 */
                SideManager side_manager(from_side);
                if ((to_side == side_manager.get_opposite())
                    || (to_side == side_manager.get_rotate_counterclockwise())) {
                    to_tracks = get_to_track_list(Fs, actual_from_track, num_to_tracks);
                } else if (to_side == side_manager.get_rotate_clockwise()) {
                    to_tracks = get_to_track_list(Fs, num_to_tracks - 1 - actual_from_track, num_to_tracks);
                }
            }

            if ((from_side == TOP)
                || (from_side == BOTTOM)) {
                /* For the next_side, to_track is from_track
                 * For the prev_side, to_track is num_to_tracks - 1 - from_track
                 * For the opposite_side, to_track is always from_track
                 */
                SideManager side_manager(from_side);
                if ((to_side == side_manager.get_opposite())
                    || (to_side == side_manager.get_rotate_clockwise())) {
                    to_tracks = get_to_track_list(Fs, actual_from_track, num_to_tracks);
                } else if (to_side == side_manager.get_rotate_counterclockwise()) {
                    to_tracks = get_to_track_list(Fs, num_to_tracks - 1 - actual_from_track, num_to_tracks);
                }
            }
            /* Finish, we return */
            return to_tracks;
            /* End switch_block_type == UNIVERSAL case. */
        case e_switch_block_type::WILTON:
            /* See S. Wilton Phd thesis, U of T, 1996 p. 103 for details on following. */
            if (from_side == LEFT) {
                if (to_side == RIGHT) { /* CHANX to CHANX */
                    to_tracks = get_to_track_list(Fs, actual_from_track, num_to_tracks);
                } else if (to_side == TOP) { /* from CHANX to CHANY */
                    to_tracks = get_to_track_list(Fs, (num_to_tracks - actual_from_track) % num_to_tracks, num_to_tracks);
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
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "Invalid switch block pattern !\n");
    }

    return to_tracks;
}

/************************************************************************
 * Build the track_to_track_map[from_side][0..chan_width-1][to_side][track_indices]
 * For a group of from_track nodes and to_track nodes
 * For each side of from_tracks, we call a routine to get the list of to_tracks
 * Then, we fill the track2track_map
 ***********************************************************************/
static void build_gsb_one_group_track_to_track_map(const RRGraphView& rr_graph,
                                                   const RRGSB& rr_gsb,
                                                   const e_switch_block_type& sb_type,
                                                   const int& Fs,
                                                   const bool& wire_opposite_side,
                                                   const t_track_group& from_tracks, /* [0..gsb_side][track_indices] */
                                                   const t_track_group& to_tracks,   /* [0..gsb_side][track_indices] */
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
                if (!wire_opposite_side
                    && (to_side_manager.get_opposite() == from_side)) {
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
                    VTR_ASSERT(to_track_ids[to_track_id] < to_tracks[to_side_index].size());
                    size_t to_track_index = to_tracks[to_side_index][to_track_ids[to_track_id]];
                    //printf("from_track(size=%lu): %lu , to_track_ids[%lu]:%lu, to_track_index: %lu in a group of %lu tracks\n",
                    //       from_tracks[side].size(), inode, to_track_id, to_track_ids[to_track_id],
                    //       to_track_index, to_tracks[to_side_index].size());
                    const RRNodeId& to_track_node = rr_gsb.get_chan_node(to_side, to_track_index);
                    VTR_ASSERT(true == rr_graph.valid_node(to_track_node));

                    /* from_track should be IN_PORT */
                    VTR_ASSERT(IN_PORT == rr_gsb.get_chan_node_direction(from_side, from_track_index));
                    /* to_track should be OUT_PORT */
                    VTR_ASSERT(OUT_PORT == rr_gsb.get_chan_node_direction(to_side, to_track_index));

                    //VTR_LOG("Consider a connection from pass tracks %d on side %s to track node %ld on side %s\n", from_track_index, SIDE_STRING[from_side], size_t(to_track_node), SIDE_STRING[to_side]);

                    /* Check if the to_track_node is already in the list ! */
                    std::vector<RRNodeId>::iterator it = std::find(track2track_map[from_side_index][from_track_index].begin(),
                                                                   track2track_map[from_side_index][from_track_index].end(),
                                                                   to_track_node);
                    if (it != track2track_map[from_side_index][from_track_index].end()) {
                        continue; /* the node_id is already in the list, go for the next */
                    }
                    /* Clear, we should add to the list */
                    track2track_map[from_side_index][from_track_index].push_back(to_track_node);
                    //VTR_LOG("Built a connection from pass tracks %d on side %s to track node %ld on side %s\n", from_track_index, SIDE_STRING[from_side], size_t(to_track_node), SIDE_STRING[to_side]);
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
t_track2track_map build_gsb_track_to_track_map(const RRGraphView& rr_graph,
                                               const RRGSB& rr_gsb,
                                               const e_switch_block_type& sb_type,
                                               const int& Fs,
                                               const e_switch_block_type& sb_subtype,
                                               const int& sub_fs,
                                               const bool& concat_wire,
                                               const bool& wire_opposite_side,
                                               const std::vector<t_segment_inf>& segment_inf) {
    t_track2track_map track2track_map; /* [0..gsb_side][0..chan_width][track_indices] */

    /* Categorize tracks into 3 groups:
     * (1) tracks will start here
     * (2) tracks will end here
     * (2) tracks will just pass through the SB */
    t_track_group start_tracks; /* [0..gsb_side][track_indices] */
    t_track_group end_tracks;   /* [0..gsb_side][track_indices] */
    t_track_group pass_tracks;  /* [0..gsb_side][track_indices] */

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
                    VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                    "Invalid track status!\n");
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
                                           concat_wire, /* End tracks should always to wired to start tracks */
                                           end_tracks, start_tracks,
                                           track2track_map);

    /* For Group 2: we build connections between end_tracks and start_tracks*/
    /* Currently, I use the same Switch Block pattern for the passing tracks and end tracks,
     * TODO: This can be improved with different patterns!
     */
    //for (e_side curr_side : SIDES) {
    //    VTR_LOG("Number of pass tracks %d on side %s\n", pass_tracks[size_t(curr_side)].size(), SIDE_STRING[curr_side]);
    //}
    build_gsb_one_group_track_to_track_map(rr_graph, rr_gsb,
                                           sb_subtype, sub_fs,
                                           wire_opposite_side, /* Pass tracks may not be wired to start tracks */
                                           pass_tracks, start_tracks,
                                           track2track_map);

    return track2track_map;
}

t_bend_track2track_map build_bend_track_to_track_map(const DeviceGrid& grids,
                                                     RRGraphBuilder& rr_graph_builder,
                                                     const RRGraphView& rr_graph,
                                                     const std::vector<t_segment_inf>& segment_inf,
                                                     const size_t& layer,
                                                     const vtr::Point<size_t>& gsb_coordinate,
                                                     const RRSwitchId& delayless_switch,
                                                     vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches) {

    std::vector<std::vector<std::vector<std::vector<RRNodeId>>>> chan_rr_nodes_all_sides; //[side][bend_num][start/end][node]
    chan_rr_nodes_all_sides.resize(4);

    int bend_seg_num = 0;
    std::vector<int> bend_seg_type; //bend type:  1: U; 2: D
    for (size_t iseg = 0; iseg < segment_inf.size(); iseg++) {
        if (segment_inf[iseg].is_bend) {
            bend_seg_num++;

            for (size_t i = 0; i < segment_inf[iseg].bend.size(); i++) {
                if (segment_inf[iseg].bend[i] != 0) {
                    bend_seg_type.push_back(segment_inf[iseg].bend[i]);
                    break;
                }
            }
        }
    }
    VTR_ASSERT(bend_seg_num == int(bend_seg_type.size()));
    for (size_t side = 0; side < 4; ++side) {
        std::vector<RRNodeId> rr_nodes;
        switch (side) {
            case TOP: /* TOP = 0 */
                /* For the bording, we should take special care */
                if (gsb_coordinate.y() == grids.height() - 2) {

                    break;
                }

                chan_rr_nodes_all_sides[0].resize(bend_seg_num);
                for (int i = 0; i < bend_seg_num; i++) {
                    chan_rr_nodes_all_sides[0][i].resize(2); //start/end track for bend
                }

                rr_nodes = find_rr_graph_chan_nodes(rr_graph,
                                                    layer, gsb_coordinate.x(), gsb_coordinate.y() + 1,
                                                    e_rr_type::CHANY);

                for (auto inode : rr_nodes) {
                    VTR_ASSERT(rr_graph.node_type(inode) == e_rr_type::CHANY);
                    Direction direction = rr_graph.node_direction(inode);
                    size_t xlow = rr_graph.node_xlow(inode);
                    size_t ylow = rr_graph.node_ylow(inode);
                    int bend_start = rr_graph.node_bend_start(inode);
                    int bend_end = rr_graph.node_bend_end(inode);

                    VTR_ASSERT((bend_start <= bend_seg_num) && (bend_end <= bend_seg_num));
                    if (direction == Direction::INC && bend_start != 0 && xlow == gsb_coordinate.x() && (ylow == gsb_coordinate.y() + 1)) {
                        VTR_ASSERT(bend_end == 0);
                        chan_rr_nodes_all_sides[0][bend_start - 1][0].push_back(inode);
                    }
                    if (direction == Direction::DEC && bend_end != 0 && xlow == gsb_coordinate.x() && (ylow == gsb_coordinate.y() + 1)) {
                        VTR_ASSERT(bend_start == 0);
                        chan_rr_nodes_all_sides[0][bend_end - 1][1].push_back(inode);
                    }
                }

                break;
            case RIGHT: /* RIGHT = 1 */
                /* For the bording, we should take special care */
                if (gsb_coordinate.x() == grids.width() - 2) {

                    break;
                }

                chan_rr_nodes_all_sides[1].resize(bend_seg_num);
                for (int i = 0; i < bend_seg_num; i++) {
                    chan_rr_nodes_all_sides[1][i].resize(2); //start/end track for bend
                }

                rr_nodes = find_rr_graph_chan_nodes(rr_graph,
                                                    layer, gsb_coordinate.x() + 1, gsb_coordinate.y(),
                                                    e_rr_type::CHANX);

                for (auto inode : rr_nodes) {
                    VTR_ASSERT(rr_graph.node_type(inode) == e_rr_type::CHANX);
                    Direction direction = rr_graph.node_direction(inode);
                    size_t xlow = rr_graph.node_xlow(inode);
                    size_t ylow = rr_graph.node_ylow(inode);
                    int bend_start = rr_graph.node_bend_start(inode);
                    int bend_end = rr_graph.node_bend_end(inode);

                    VTR_ASSERT((bend_start <= bend_seg_num) && (bend_end <= bend_seg_num));
                    if (direction == Direction::INC && bend_start != 0 && (xlow == gsb_coordinate.x() + 1) && ylow == gsb_coordinate.y()) {
                        VTR_ASSERT(bend_end == 0);
                        chan_rr_nodes_all_sides[1][bend_start - 1][0].push_back(inode);
                    }
                    if (direction == Direction::DEC && bend_end != 0 && (xlow == gsb_coordinate.x() + 1) && ylow == gsb_coordinate.y()) {
                        VTR_ASSERT(bend_start == 0);
                        chan_rr_nodes_all_sides[1][bend_end - 1][1].push_back(inode);
                    }
                }
                break;
            case BOTTOM: /* BOTTOM = 2 */
                /* For the bording, we should take special care */
                if (gsb_coordinate.y() == 0) {

                    break;
                }

                chan_rr_nodes_all_sides[2].resize(bend_seg_num);
                for (int i = 0; i < bend_seg_num; i++) {
                    chan_rr_nodes_all_sides[2][i].resize(2); //start/end track for bend
                }

                rr_nodes = find_rr_graph_chan_nodes(rr_graph,
                                                    layer, gsb_coordinate.x(), gsb_coordinate.y(),
                                                    e_rr_type::CHANY);

                for (auto inode : rr_nodes) {
                    VTR_ASSERT(rr_graph.node_type(inode) == e_rr_type::CHANY);
                    Direction direction = rr_graph.node_direction(inode);
                    size_t xhigh = rr_graph.node_xhigh(inode);
                    size_t yhigh = rr_graph.node_yhigh(inode);
                    int bend_start = rr_graph.node_bend_start(inode);
                    int bend_end = rr_graph.node_bend_end(inode);

                    VTR_ASSERT((bend_start <= bend_seg_num) && (bend_end <= bend_seg_num));
                    if (direction == Direction::INC && bend_end != 0 && xhigh == gsb_coordinate.x() && yhigh == gsb_coordinate.y()) {
                        VTR_ASSERT(bend_start == 0);
                        chan_rr_nodes_all_sides[2][bend_end - 1][1].push_back(inode);
                    }
                    if (direction == Direction::DEC && bend_start != 0 && xhigh == gsb_coordinate.x() && yhigh == gsb_coordinate.y()) {
                        VTR_ASSERT(bend_end == 0);
                        chan_rr_nodes_all_sides[2][bend_start - 1][0].push_back(inode);
                    }
                }
                break;
            case LEFT: /* BOTTOM = 2 */
                /* For the bording, we should take special care */
                if (gsb_coordinate.x() == 0) {

                    break;
                }

                chan_rr_nodes_all_sides[3].resize(bend_seg_num);
                for (int i = 0; i < bend_seg_num; i++) {
                    chan_rr_nodes_all_sides[3][i].resize(2); //start/end track for bend
                }

                rr_nodes = find_rr_graph_chan_nodes(rr_graph,
                                                    layer, gsb_coordinate.x(), gsb_coordinate.y(),
                                                    e_rr_type::CHANX);

                for (auto inode : rr_nodes) {
                    VTR_ASSERT(rr_graph.node_type(inode) == e_rr_type::CHANX);
                    Direction direction = rr_graph.node_direction(inode);
                    size_t xhigh = rr_graph.node_xhigh(inode);
                    size_t yhigh = rr_graph.node_yhigh(inode);
                    int bend_start = rr_graph.node_bend_start(inode);
                    int bend_end = rr_graph.node_bend_end(inode);

                    VTR_ASSERT((bend_start <= bend_seg_num) && (bend_end <= bend_seg_num));
                    if (direction == Direction::INC && bend_end != 0 && xhigh == gsb_coordinate.x() && yhigh == gsb_coordinate.y()) {
                        VTR_ASSERT(bend_start == 0);
                        chan_rr_nodes_all_sides[3][bend_end - 1][1].push_back(inode);
                    }
                    if (direction == Direction::DEC && bend_start != 0 && xhigh == gsb_coordinate.x() && yhigh == gsb_coordinate.y()) {
                        VTR_ASSERT(bend_end == 0);
                        chan_rr_nodes_all_sides[3][bend_start - 1][0].push_back(inode);
                    }
                }
                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "Invalid side index!\n");
        }
    }
    std::map<RRNodeId, RRNodeId> bend_seg_head2bend_seg_end_map;
    for (size_t ibend_seg = 0; ibend_seg < (size_t)bend_seg_num; ibend_seg++) {
        int bend_type = bend_seg_type[ibend_seg]; //bend_type  1:U  2:D
        VTR_ASSERT(bend_type == 1 || bend_type == 2);

        if (bend_type == 1) { //bend type U
            for (size_t side = 0; side < 4; side++) {
                size_t to_side = (side + 1) % 4;
                if (chan_rr_nodes_all_sides[side].size() > 0)
                    for (size_t inode = 0; inode < chan_rr_nodes_all_sides[side][ibend_seg][1].size(); inode++) {

                        if (chan_rr_nodes_all_sides[to_side].size() > 0) {
                            VTR_ASSERT(chan_rr_nodes_all_sides[side][ibend_seg][1].size() == chan_rr_nodes_all_sides[to_side][ibend_seg][0].size());
                            bend_seg_head2bend_seg_end_map.emplace(std::make_pair(chan_rr_nodes_all_sides[side][ibend_seg][1][inode], chan_rr_nodes_all_sides[to_side][ibend_seg][0][inode]));
                            rr_node_driver_switches[chan_rr_nodes_all_sides[to_side][ibend_seg][0][inode]] = delayless_switch;
                        } else {
                            rr_graph_builder.set_node_bend_end(chan_rr_nodes_all_sides[side][ibend_seg][1][inode], 0);
                        }
                    }
                else {
                    if (chan_rr_nodes_all_sides[to_side].size() > 0) {
                        for (size_t inode = 0; inode < chan_rr_nodes_all_sides[to_side][ibend_seg][0].size(); inode++) {
                            rr_graph_builder.set_node_bend_start(chan_rr_nodes_all_sides[to_side][ibend_seg][0][inode], 0);
                        }
                    }
                }
            }

        } else if (bend_type == 2) { //bend type D
            for (size_t side = 0; side < 4; side++) {
                size_t to_side = (side + 3) % 4;
                if (chan_rr_nodes_all_sides[side].size() > 0)
                    for (size_t inode = 0; inode < chan_rr_nodes_all_sides[side][ibend_seg][1].size(); inode++) {

                        if (chan_rr_nodes_all_sides[to_side].size() > 0) {
                            VTR_ASSERT(chan_rr_nodes_all_sides[side][ibend_seg][1].size() == chan_rr_nodes_all_sides[to_side][ibend_seg][0].size());
                            bend_seg_head2bend_seg_end_map.emplace(std::make_pair(chan_rr_nodes_all_sides[side][ibend_seg][1][inode], chan_rr_nodes_all_sides[to_side][ibend_seg][0][inode]));
                            rr_node_driver_switches[chan_rr_nodes_all_sides[to_side][ibend_seg][0][inode]] = delayless_switch;
                        } else {
                            rr_graph_builder.set_node_bend_end(chan_rr_nodes_all_sides[side][ibend_seg][1][inode], 0);
                        }
                    }
                else {
                    if (chan_rr_nodes_all_sides[to_side].size() > 0) {
                        for (size_t inode = 0; inode < chan_rr_nodes_all_sides[to_side][ibend_seg][0].size(); inode++) {
                            rr_graph_builder.set_node_bend_start(chan_rr_nodes_all_sides[to_side][ibend_seg][0][inode], 0);
                        }
                    }
                }
            }
        }
    }

    return bend_seg_head2bend_seg_end_map;
}

/* Build a RRChan Object with the given channel type and coorindators */
static RRChan build_one_tileable_rr_chan(const size_t& layer,
                                         const vtr::Point<size_t>& chan_coordinate,
                                         const e_rr_type& chan_type,
                                         const RRGraphView& rr_graph,
                                         const ChanNodeDetails& chan_details) {
    std::vector<RRNodeId> chan_rr_nodes;

    /* Create a rr_chan object and check if it is unique in the graph */
    RRChan rr_chan;

    /* Fill the information */
    rr_chan.set_type(chan_type);

    /* Collect rr_nodes for this channel */
    chan_rr_nodes = find_rr_graph_chan_nodes(rr_graph,
                                             layer, chan_coordinate.x(), chan_coordinate.y(),
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
                                const RRGraphView& rr_graph,
                                const vtr::Point<size_t>& device_chan_width,
                                const std::vector<t_segment_inf>& segment_inf_x,
                                const std::vector<t_segment_inf>& segment_inf_y,
                                const size_t& layer,
                                const vtr::Point<size_t>& gsb_coordinate,
                                const bool& perimeter_cb) {
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
        enum e_side opin_grid_side[2] = {NUM_2D_SIDES, NUM_2D_SIDES};
        enum PORTS chan_dir_to_port_dir_mapping[2] = {OUT_PORT, IN_PORT}; /* 0: INC_DIRECTION => ?; 1: DEC_DIRECTION => ? */

        /* Build a segment details, where we need the segment ids for building rr_chan
         * We do not care starting and ending points here, so set chan_side as NUM_SIDES
         */
        ChanNodeDetails chanx_details = build_unidir_chan_node_details(device_chan_width.x(), grids.width() - 1,
                                                                       false, false, segment_inf_x);
        ChanNodeDetails chany_details = build_unidir_chan_node_details(device_chan_width.y(), grids.height() - 1,
                                                                       false, false, segment_inf_y);

        switch (side) {
            case TOP: /* TOP = 0 */
                /* For the border, we should take special care. */
                if (gsb_coordinate.y() == grids.height() - 1) {
                    rr_gsb.clear_one_side(side_manager.get_side());
                    break;
                }
                /* Routing channels*/
                /* SideManager: TOP => 0, RIGHT => 1, BOTTOM => 2, LEFT => 3 */
                /* Create a rr_chan object and check if it is unique in the graph */
                rr_chan = build_one_tileable_rr_chan(layer, coordinate, e_rr_type::CHANY, rr_graph, chany_details);
                chan_dir_to_port_dir_mapping[0] = OUT_PORT; /* INC_DIRECTION => OUT_PORT */
                chan_dir_to_port_dir_mapping[1] = IN_PORT;  /* DEC_DIRECTION => IN_PORT */

                /* Assign grid side of OPIN */
                /* Grid[x][y+1] RIGHT side outputs pins */
                opin_grid_side[0] = RIGHT;
                /* Grid[x+1][y+1] left side outputs pins */
                opin_grid_side[1] = LEFT;

                /* Build the Switch block: opin and opin_grid_side */
                /* Include Grid[x][y+1] RIGHT side outputs pins */
                temp_opin_rr_nodes[0] = find_rr_graph_grid_nodes(rr_graph, grids,
                                                                 layer, gsb_coordinate.x(), gsb_coordinate.y() + 1,
                                                                 e_rr_type::OPIN, opin_grid_side[0]);
                /* Include Grid[x+1][y+1] Left side output pins */
                temp_opin_rr_nodes[1] = find_rr_graph_grid_nodes(rr_graph, grids,
                                                                 layer, gsb_coordinate.x() + 1, gsb_coordinate.y() + 1,
                                                                 e_rr_type::OPIN, opin_grid_side[1]);

                break;
            case RIGHT: /* RIGHT = 1 */
                /* For the border, we should take special care. The rightmost column (W-1) does not have any right side routing channel. If perimeter connection block is not enabled, even the last second rightmost column (W-2) does not have any right side routing channel  */
                if (gsb_coordinate.x() == grids.width() - 1) {
                    rr_gsb.clear_one_side(side_manager.get_side());
                    break;
                }
                /* Routing channels*/
                /* SideManager: TOP => 0, RIGHT => 1, BOTTOM => 2, LEFT => 3 */
                /* Collect rr_nodes for Tracks for top: chany[x][y+1] */
                /* Create a rr_chan object and check if it is unique in the graph */
                rr_chan = build_one_tileable_rr_chan(layer, coordinate, e_rr_type::CHANX, rr_graph, chanx_details);
                chan_dir_to_port_dir_mapping[0] = OUT_PORT; /* INC_DIRECTION => OUT_PORT */
                chan_dir_to_port_dir_mapping[1] = IN_PORT;  /* DEC_DIRECTION => IN_PORT */

                /* Assign grid side of OPIN */
                /* Grid[x+1][y+1] BOTTOM side outputs pins */
                opin_grid_side[0] = BOTTOM;
                /* Grid[x+1][y] TOP side outputs pins */
                opin_grid_side[1] = TOP;

                /* Build the Switch block: opin and opin_grid_side */
                /* include Grid[x+1][y+1] Bottom side output pins */
                temp_opin_rr_nodes[0] = find_rr_graph_grid_nodes(rr_graph, grids,
                                                                 layer, gsb_coordinate.x() + 1, gsb_coordinate.y() + 1,
                                                                 e_rr_type::OPIN, opin_grid_side[0]);
                /* include Grid[x+1][y] Top side output pins */
                temp_opin_rr_nodes[1] = find_rr_graph_grid_nodes(rr_graph, grids,
                                                                 layer, gsb_coordinate.x() + 1, gsb_coordinate.y(),
                                                                 e_rr_type::OPIN, opin_grid_side[1]);
                break;
            case BOTTOM: /* BOTTOM = 2*/
                if (!perimeter_cb && gsb_coordinate.y() == 0) {
                    rr_gsb.clear_one_side(side_manager.get_side());
                    break;
                }
                /* Routing channels*/
                /* SideManager: TOP => 0, RIGHT => 1, BOTTOM => 2, LEFT => 3 */
                /* Collect rr_nodes for Tracks for bottom: chany[x][y] */
                /* Create a rr_chan object and check if it is unique in the graph */
                rr_chan = build_one_tileable_rr_chan(layer, coordinate, e_rr_type::CHANY, rr_graph, chany_details);
                chan_dir_to_port_dir_mapping[0] = IN_PORT;  /* INC_DIRECTION => IN_PORT */
                chan_dir_to_port_dir_mapping[1] = OUT_PORT; /* DEC_DIRECTION => OUT_PORT */

                /* Assign grid side of OPIN */
                /* Grid[x+1][y] LEFT side outputs pins */
                opin_grid_side[0] = LEFT;
                /* Grid[x][y] RIGHT side outputs pins */
                opin_grid_side[1] = RIGHT;

                /* Build the Switch block: opin and opin_grid_side */
                /* include Grid[x+1][y] Left side output pins */
                temp_opin_rr_nodes[0] = find_rr_graph_grid_nodes(rr_graph, grids,
                                                                 layer, gsb_coordinate.x() + 1, gsb_coordinate.y(),
                                                                 e_rr_type::OPIN, opin_grid_side[0]);
                /* include Grid[x][y] Right side output pins */
                temp_opin_rr_nodes[1] = find_rr_graph_grid_nodes(rr_graph, grids,
                                                                 layer, gsb_coordinate.x(), gsb_coordinate.y(),
                                                                 e_rr_type::OPIN, opin_grid_side[1]);
                break;
            case LEFT: /* LEFT = 3 */
                if (!perimeter_cb && gsb_coordinate.x() == 0) {
                    rr_gsb.clear_one_side(side_manager.get_side());
                    break;
                }
                /* Routing channels*/
                /* SideManager: TOP => 0, RIGHT => 1, BOTTOM => 2, LEFT => 3 */
                /* Collect rr_nodes for Tracks for left: chanx[x][y] */
                /* Create a rr_chan object and check if it is unique in the graph */
                rr_chan = build_one_tileable_rr_chan(layer, coordinate, e_rr_type::CHANX, rr_graph, chanx_details);
                chan_dir_to_port_dir_mapping[0] = IN_PORT;  /* INC_DIRECTION => IN_PORT */
                chan_dir_to_port_dir_mapping[1] = OUT_PORT; /* DEC_DIRECTION => OUT_PORT */

                /* Grid[x][y+1] BOTTOM side outputs pins */
                opin_grid_side[0] = BOTTOM;
                /* Grid[x][y] TOP side outputs pins */
                opin_grid_side[1] = TOP;

                /* Build the Switch block: opin and opin_grid_side */
                /* include Grid[x][y+1] Bottom side outputs pins */
                temp_opin_rr_nodes[0] = find_rr_graph_grid_nodes(rr_graph, grids,
                                                                 layer, gsb_coordinate.x(), gsb_coordinate.y() + 1,
                                                                 e_rr_type::OPIN, opin_grid_side[0]);
                /* include Grid[x][y] Top side output pins */
                temp_opin_rr_nodes[1] = find_rr_graph_grid_nodes(rr_graph, grids,
                                                                 layer, gsb_coordinate.x(), gsb_coordinate.y(),
                                                                 e_rr_type::OPIN, opin_grid_side[1]);

                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "Invalid side index!\n");
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
        opin_grid_side[0] = NUM_2D_SIDES;
        opin_grid_side[1] = NUM_2D_SIDES;
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
     *     Grid[x][y]   |->| Y- Connection Block        Y- Connection Block |<-| Grid[x+1][y]
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
                chan_side = BOTTOM;
                /* The input pins of the routing channel come from the left side of Grid[x+1][y+1] */
                ix = rr_gsb.get_sb_x() + 1;
                iy = rr_gsb.get_sb_y();
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
                chan_side = BOTTOM;
                /* The input pins of the routing channel come from the right side of Grid[x][y+1] */
                ix = rr_gsb.get_sb_x();
                iy = rr_gsb.get_sb_y();
                ipin_rr_node_grid_side = RIGHT;
                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "Invalid side index!\n");
        }

        /* If there is no channel at this side, we skip ipin_node annotation */
        if (0 == rr_gsb.get_chan_width(chan_side)) {
            continue;
        }
        /* Collect IPIN rr_nodes*/
        temp_ipin_rr_nodes = find_rr_graph_grid_nodes(rr_graph, grids,
                                                      layer, ix, iy, e_rr_type::IPIN, ipin_rr_node_grid_side);
        /* Fill the ipin nodes of RRGSB */
        for (const RRNodeId& inode : temp_ipin_rr_nodes) {
            rr_gsb.add_ipin_node(inode, side_manager.get_side());
        }
        /* Clear the temp data */
        temp_ipin_rr_nodes.clear();
    }

    /* Find all MUX rr_nodes */
    std::vector<RRNodeId> mux_rr_nodes = rr_graph.node_lookup().find_grid_nodes_at_all_sides(layer, gsb_coordinate.x(), gsb_coordinate.y(), e_rr_type::MUX);
    for (auto mux_rr_node : mux_rr_nodes) {
        rr_gsb.add_mux_node(mux_rr_node);
    }
    /* For TOP and RIGHT borders, we need to add extra mux nodes. */
    if (gsb_coordinate.y() == grids.height() - 2) {
        std::vector<RRNodeId> extra_mux_rr_nodes = rr_graph.node_lookup().find_grid_nodes_at_all_sides(layer, gsb_coordinate.x(), gsb_coordinate.y() + 1, e_rr_type::MUX);
        for (auto mux_rr_node : extra_mux_rr_nodes) {
            rr_gsb.add_mux_node(mux_rr_node);
        }
    }

    if (gsb_coordinate.x() == grids.width() - 2) {
        std::vector<RRNodeId> extra_mux_rr_nodes = rr_graph.node_lookup().find_grid_nodes_at_all_sides(layer, gsb_coordinate.x() + 1, gsb_coordinate.y(), e_rr_type::MUX);
        for (auto mux_rr_node : extra_mux_rr_nodes) {
            rr_gsb.add_mux_node(mux_rr_node);
        }
    }

    if ((gsb_coordinate.x() == grids.width() - 2) && (gsb_coordinate.y() == grids.height() - 2)) {
        std::vector<RRNodeId> extra_mux_rr_nodes = rr_graph.node_lookup().find_grid_nodes_at_all_sides(layer, gsb_coordinate.x() + 1, gsb_coordinate.y() + 1, e_rr_type::MUX);
        for (auto mux_rr_node : extra_mux_rr_nodes) {
            rr_gsb.add_mux_node(mux_rr_node);
        }
    }

    return rr_gsb;
}

/************************************************************************
 * Create edges for each rr_node of a General Switch Blocks (GSB):
 * 1. create edges between CHANX | CHANY and IPINs (connections inside connection blocks)
 * 2. create edges between OPINs, CHANX and CHANY (connections inside switch blocks)
 * 3. create edges between OPINs and IPINs (direct-connections)
 ***********************************************************************/
void build_edges_for_one_tileable_rr_gsb(RRGraphBuilder& rr_graph_builder,
                                         const RRGSB& rr_gsb,
                                         const t_track2pin_map& track2ipin_map,
                                         const t_pin2track_map& opin2track_map,
                                         const t_track2track_map& track2track_map,
                                         const vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches,
                                         size_t& num_edges_to_create) {
    size_t edge_count = 0;
    /* Walk through each sides */
    for (size_t side = 0; side < rr_gsb.get_num_sides(); ++side) {
        SideManager side_manager(side);
        enum e_side gsb_side = side_manager.get_side();

        /* Find OPINs */
        for (size_t inode = 0; inode < rr_gsb.get_num_opin_nodes(gsb_side); ++inode) {
            const RRNodeId& opin_node = rr_gsb.get_opin_node(gsb_side, inode);

            for (size_t to_side = 0; to_side < opin2track_map[gsb_side][inode].size(); ++to_side) {
                /* 1. create edges between OPINs and CHANX|CHANY, using opin2track_map */
                /* add edges to the opin_node */
                for (const RRNodeId& track_node : opin2track_map[gsb_side][inode][to_side]) {
                    rr_graph_builder.create_edge_in_cache(opin_node, track_node, rr_node_driver_switches[track_node], false);
                    edge_count++;
                }
            }
        }

        /* Find  CHANX or CHANY */
        /* For TRACKs to IPINs, we only care LEFT and TOP sides
         * Skip RIGHT and BOTTOM for the ipin2track_map since they should be handled in other GSBs
         */
        if ((side_manager.get_side() == rr_gsb.get_cb_chan_side(e_rr_type::CHANX))
            || (side_manager.get_side() == rr_gsb.get_cb_chan_side(e_rr_type::CHANY))) {
            /* 2. create edges between CHANX|CHANY and IPINs, using ipin2track_map */
            for (size_t inode = 0; inode < rr_gsb.get_chan_width(gsb_side); ++inode) {
                const RRNodeId& chan_node = rr_gsb.get_chan_node(gsb_side, inode);
                for (const RRNodeId& ipin_node : track2ipin_map[gsb_side][inode]) {
                    rr_graph_builder.create_edge_in_cache(chan_node, ipin_node, rr_node_driver_switches[ipin_node], false);
                    edge_count++;
                }
            }
        }

        /* 3. create edges between CHANX|CHANY and CHANX|CHANY, using track2track_map */
        for (size_t inode = 0; inode < rr_gsb.get_chan_width(gsb_side); ++inode) {
            const RRNodeId& chan_node = rr_gsb.get_chan_node(gsb_side, inode);
            for (const RRNodeId& track_node : track2track_map[gsb_side][inode]) {
                rr_graph_builder.create_edge_in_cache(chan_node, track_node, rr_node_driver_switches[track_node], false);
                edge_count++;
            }
        }
    }

    num_edges_to_create += edge_count;
}

void build_edges_for_one_tileable_rr_gsb_vib(RRGraphBuilder& rr_graph_builder,
                                             const RRGSB& rr_gsb,
                                             const t_bend_track2track_map& sb_bend_conn,
                                             const t_track2pin_map& track2ipin_map,
                                             const t_pin2track_map& opin2track_map,
                                             const t_track2track_map& track2track_map,
                                             const vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches,
                                             size_t& num_edges_to_create) {
    size_t edge_count = 0;
    /* Walk through each sides */
    for (size_t side = 0; side < rr_gsb.get_num_sides(); ++side) {
        SideManager side_manager(side);
        enum e_side gsb_side = side_manager.get_side();

        /* Find OPINs */
        for (size_t inode = 0; inode < rr_gsb.get_num_opin_nodes(gsb_side); ++inode) {
            const RRNodeId& opin_node = rr_gsb.get_opin_node(gsb_side, inode);

            for (size_t to_side = 0; to_side < opin2track_map[gsb_side][inode].size(); ++to_side) {
                /* 1. create edges between OPINs and CHANX|CHANY, using opin2track_map */
                /* add edges to the opin_node */
                for (const RRNodeId& track_node : opin2track_map[gsb_side][inode][to_side]) {
                    rr_graph_builder.create_edge_in_cache(opin_node, track_node, rr_node_driver_switches[track_node], false);
                    edge_count++;
                }
            }
        }

        /* Find  CHANX or CHANY */
        /* For TRACKs to IPINs, we only care LEFT and TOP sides
         * Skip RIGHT and BOTTOM for the ipin2track_map since they should be handled in other GSBs
         */
        if ((side_manager.get_side() == rr_gsb.get_cb_chan_side(e_rr_type::CHANX))
            || (side_manager.get_side() == rr_gsb.get_cb_chan_side(e_rr_type::CHANY))) {
            /* 2. create edges between CHANX|CHANY and IPINs, using ipin2track_map */
            for (size_t inode = 0; inode < rr_gsb.get_chan_width(gsb_side); ++inode) {
                const RRNodeId& chan_node = rr_gsb.get_chan_node(gsb_side, inode);
                for (const RRNodeId& ipin_node : track2ipin_map[gsb_side][inode]) {
                    rr_graph_builder.create_edge_in_cache(chan_node, ipin_node, rr_node_driver_switches[ipin_node], false);
                    edge_count++;
                }
            }
        }

        /* 3. create edges between CHANX|CHANY and CHANX|CHANY, using track2track_map */
        for (size_t inode = 0; inode < rr_gsb.get_chan_width(gsb_side); ++inode) {
            const RRNodeId& chan_node = rr_gsb.get_chan_node(gsb_side, inode);
            for (const RRNodeId& track_node : track2track_map[gsb_side][inode]) {
                rr_graph_builder.create_edge_in_cache(chan_node, track_node, rr_node_driver_switches[track_node], false);
                edge_count++;
            }
        }
    }
    /* Create edges between bend nodes */
    for (auto iter = sb_bend_conn.begin(); iter != sb_bend_conn.end(); ++iter) {
        rr_graph_builder.create_edge_in_cache(iter->first, iter->second, rr_node_driver_switches[iter->second], false);
        edge_count++;
    }
    num_edges_to_create += edge_count;
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
static void build_gsb_one_ipin_track2pin_map(const RRGraphView& rr_graph,
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
        //int track_cnt = 0;
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

            //track_cnt += 2;
        }

        /* Ensure the number of tracks is similar to Fc */
        /* Give a warning if Fc is < track_cnt */
        /*
         * if (actual_Fc != track_cnt) {
         * vpr_printf(TIO_MESSAGE_INFO,
         * "IPIN Node(%lu) will have a different Fc(=%lu) than specified(=%lu)!\n",
         * ipin_node - rr_graph->rr_node, track_cnt, actual_Fc);
         * }
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
static void build_gsb_one_opin_pin2track_map(const RRGraphView& rr_graph,
                                             const RRGSB& rr_gsb,
                                             const enum e_side& opin_side,
                                             const size_t& opin_node_id,
                                             const enum e_side& chan_side,
                                             const std::vector<int>& Fc,
                                             const size_t& offset,
                                             const std::vector<t_segment_inf>& segment_inf,
                                             t_pin2track_map& opin2track_map) {
    /* Get a list of segment_ids*/
    std::vector<RRSegmentId> seg_list = rr_gsb.get_chan_segment_ids(opin_side);
    size_t chan_width = rr_gsb.get_chan_width(chan_side);
    SideManager opin_side_manager(opin_side);
    SideManager chan_side_manager(chan_side);

    for (size_t iseg = 0; iseg < seg_list.size(); ++iseg) {
        /* Get a list of node that have the segment id */
        std::vector<size_t> track_list = rr_gsb.get_chan_node_ids_by_segment_ids(chan_side, seg_list[iseg]);
        /* Refine the track_list: keep those will have connection blocks in the GSB */
        std::vector<size_t> actual_track_list;
        for (size_t inode = 0; inode < track_list.size(); ++inode) {
            /* Check if tracks allow connection blocks in the GSB*/
            if (false == is_gsb_in_track_sb_population(rr_graph, rr_gsb, chan_side, track_list[inode], segment_inf)) {
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
            opin2track_map[opin_side_index][opin_node_id][chan_side_manager.to_size_t()].push_back(track_rr_node_index);
            /* update track counter */
            track_cnt++;
            /* Stop when we have enough Fc: this may lead to some tracks have zero drivers.
             * So I comment it. And we just make sure its track_cnt >= actual_Fc
             * if (actual_Fc == track_cnt) {
             * break;
             * }
             */
        }

        /* Ensure the number of tracks is similar to Fc */
        /* Give a warning if Fc is < track_cnt */
        /*
         * if (actual_Fc != track_cnt) {
         * vpr_printf(TIO_MESSAGE_INFO,
         * "OPIN Node(%lu) will have a different Fc(=%lu) than specified(=%lu)!\n",
         * opin_node_id, track_cnt, actual_Fc);
         * }
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
t_track2pin_map build_gsb_track_to_ipin_map(const RRGraphView& rr_graph,
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
            t_physical_tile_loc ipin_node_phy_tile_loc(rr_graph.node_xlow(ipin_node), rr_graph.node_ylow(ipin_node), 0);
            /* Skip EMPTY type */
            if (true == is_empty_type(grids.get_physical_type(ipin_node_phy_tile_loc))) {
                continue;
            }

            int grid_type_index = grids.get_physical_type(ipin_node_phy_tile_loc)->index;
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

            //VTR_ASSERT(ipin_Fc_out.size() == segment_inf.size());

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
t_pin2track_map build_gsb_opin_to_track_map(const RRGraphView& rr_graph,
                                            const RRGSB& rr_gsb,
                                            const DeviceGrid& grids,
                                            const std::vector<t_segment_inf>& segment_inf,
                                            const std::vector<vtr::Matrix<int>>& Fc_out,
                                            const bool& opin2all_sides) {
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
            t_physical_tile_loc opin_node_phy_tile_loc(rr_graph.node_xlow(opin_node), rr_graph.node_ylow(opin_node), 0);
            /* Skip EMPTY type */
            if (true == is_empty_type(grids.get_physical_type(opin_node_phy_tile_loc))) {
                continue;
            }
            int grid_type_index = grids.get_physical_type(opin_node_phy_tile_loc)->index;

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
            if (rr_gsb.get_sb_x() == grids.width() - 1 || rr_gsb.get_sb_y() == grids.height() - 1) {
                skip_conn2track = true;
            }

            if (true == skip_conn2track) {
                continue;
            }
            VTR_ASSERT(opin_Fc_out.size() == segment_inf.size());

            /* Build track2ipin_map for this IPIN */
            opin2track_map[side][inode].resize(rr_gsb.get_num_sides());
            if (opin2all_sides) {
                for (size_t track_side = 0; track_side < rr_gsb.get_num_sides(); ++track_side) {
                    SideManager track_side_mgr(track_side);
                    build_gsb_one_opin_pin2track_map(rr_graph, rr_gsb, opin_side, inode, track_side_mgr.get_side(), opin_Fc_out,
                                                     /* Give an offset for the first track that this ipin will connect to */
                                                     offset[side_manager.to_size_t()],
                                                     segment_inf, opin2track_map);
                }
            } else {
                build_gsb_one_opin_pin2track_map(rr_graph, rr_gsb, opin_side, inode, opin_side, opin_Fc_out,
                                                 /* Give an offset for the first track that this ipin will connect to */
                                                 offset[side_manager.to_size_t()],
                                                 segment_inf, opin2track_map);
            }
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
void build_direct_connections_for_one_gsb(const RRGraphView& rr_graph,
                                          RRGraphBuilder& rr_graph_builder,
                                          const DeviceGrid& grids,
                                          const size_t& layer,
                                          const vtr::Point<size_t>& from_grid_coordinate,
                                          const std::vector<t_direct_inf>& directs,
                                          const std::vector<t_clb_to_clb_directs>& clb_to_clb_directs) {
    VTR_ASSERT(directs.size() == clb_to_clb_directs.size());

    t_physical_tile_type_ptr grid_type = grids.get_physical_type(t_physical_tile_loc(from_grid_coordinate.x(), from_grid_coordinate.y(), layer));

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
        t_physical_tile_type_ptr to_grid_type = grids.get_physical_type(t_physical_tile_loc(to_grid_coordinate.x(), to_grid_coordinate.y(), layer));
        /* Check if to_grid if the same grid */
        if (to_grid_type != clb_to_clb_directs[i].to_clb_type) {
            continue;
        }

        bool swap;
        int max_index, min_index;
        /* Compute index of opin with regards to given pins */
        if (clb_to_clb_directs[i].from_clb_pin_start_index
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
            //Capacity location determined by pin number relative to pins per capacity instance
            auto [z, relative_opin] = get_capacity_location_from_physical_pin(grid_type, opin);
            VTR_ASSERT(z >= 0 && z < grid_type->capacity);

            if ((to_grid_coordinate.x() < grids.width() - 1)
                && (to_grid_coordinate.y() < grids.height() - 1)) {
                int relative_ipin = UNDEFINED;
                if (clb_to_clb_directs[i].to_clb_pin_start_index
                    > clb_to_clb_directs[i].to_clb_pin_end_index) {
                    if (true == swap) {
                        relative_ipin = clb_to_clb_directs[i].to_clb_pin_end_index + offset;
                    } else {
                        relative_ipin = clb_to_clb_directs[i].to_clb_pin_start_index - offset;
                    }
                } else {
                    if (true == swap) {
                        relative_ipin = clb_to_clb_directs[i].to_clb_pin_end_index - offset;
                    } else {
                        relative_ipin = clb_to_clb_directs[i].to_clb_pin_start_index + offset;
                    }
                }

                /* Get the pin index in the rr_graph */
                t_physical_tile_loc from_tile_loc(from_grid_coordinate.x(), from_grid_coordinate.y(), layer);
                t_physical_tile_loc to_tile_loc(to_grid_coordinate.x(), to_grid_coordinate.y(), layer);

                /* Find the side of grid pins, the pin location should be unique!
                 * Pin location is required by searching a node in rr_graph
                 */
                std::vector<e_side> opin_grid_side = find_grid_pin_sides(grids, layer, from_grid_coordinate.x() + grid_type->pin_width_offset[opin], from_grid_coordinate.y() + grid_type->pin_height_offset[opin], opin);
                if (1 != opin_grid_side.size()) {
                  VPR_FATAL_ERROR(VPR_ERROR_ARCH, "[Arch LINE %d] From pin (index=%d) of direct connection '%s' does not exist on any side of the programmable block '%s'.\n", directs[i].line, opin, directs[i].from_pin.c_str());
                }

                /* directs[i].sub_tile_offset is added to from_capacity(z) to get the target_capacity */
                int to_subtile_cap = z + directs[i].sub_tile_offset;
                /* Iterate over all sub_tiles to get the sub_tile which the target_cap belongs to. */
                const t_sub_tile* to_sub_tile = nullptr;
                for (const t_sub_tile& sub_tile : to_grid_type->sub_tiles) {
                    if (sub_tile.capacity.is_in_range(to_subtile_cap)) {
                        to_sub_tile = &sub_tile;
                        break;
                    }
                }
                VTR_ASSERT(to_sub_tile != nullptr);
                if (relative_ipin >= to_sub_tile->num_phy_pins) continue;
                // If this block has capacity > 1 then the pins of z position > 0 are offset
                // by the number of pins per capacity instance
                int ipin = get_physical_pin_from_capacity_location(to_grid_type, relative_ipin, to_subtile_cap);
                std::vector<e_side> ipin_grid_side = find_grid_pin_sides(grids, layer, to_grid_coordinate.x() + to_grid_type->pin_width_offset[ipin], to_grid_coordinate.y() + to_grid_type->pin_height_offset[ipin], ipin);
                if (1 != ipin_grid_side.size()) {
                  VPR_FATAL_ERROR(VPR_ERROR_ARCH, "[Arch LINE %d] To pin (index=%d) of direct connection '%s' does not exist on any side of the programmable block '%s'.\n", directs[i].line, relative_ipin, directs[i].to_pin.c_str());
                }

                RRNodeId opin_node_id = rr_graph.node_lookup().find_node(layer,
                                                                         from_grid_coordinate.x() + grid_type->pin_width_offset[opin],
                                                                         from_grid_coordinate.y() + grid_type->pin_height_offset[opin],
                                                                         e_rr_type::OPIN, opin, opin_grid_side[0]);
                RRNodeId ipin_node_id = rr_graph.node_lookup().find_node(layer,
                                                                         to_grid_coordinate.x() + to_grid_type->pin_width_offset[ipin],
                                                                         to_grid_coordinate.y() + to_grid_type->pin_height_offset[ipin],
                                                                         e_rr_type::IPIN, ipin, ipin_grid_side[0]);

                /* add edges to the opin_node */
                VTR_ASSERT(opin_node_id && ipin_node_id);
                rr_graph_builder.create_edge_in_cache(opin_node_id, ipin_node_id, RRSwitchId(clb_to_clb_directs[i].switch_index), false);
            }
        }
    }
    /* Build actual edges */
    rr_graph_builder.build_edges(true);
}

/* Vib edge builder */
t_vib_map build_vib_map(const RRGraphView& rr_graph,
                        const DeviceGrid& grids,
                        const VibDeviceGrid& vib_grid,
                        const RRGSB& rr_gsb,
                        const std::vector<t_segment_inf>& segment_inf,
                        const size_t& layer,
                        const vtr::Point<size_t>& gsb_coordinate,
                        const vtr::Point<size_t>& actual_coordinate) {
    VTR_ASSERT(rr_gsb.get_x() == gsb_coordinate.x() && rr_gsb.get_y() == gsb_coordinate.y());

    t_vib_map vib_map;

    const VibInf* vib = vib_grid.get_vib(layer, actual_coordinate.x(), actual_coordinate.y());
    auto phy_type = grids.get_physical_type({(int)actual_coordinate.x(), (int)actual_coordinate.y(), (int)layer});
    VTR_ASSERT(vib->get_pbtype_name() == phy_type->name);
    const std::vector<t_first_stage_mux_inf> first_stages = vib->get_first_stages();
    for (size_t i_first_stage = 0; i_first_stage < first_stages.size(); i_first_stage++) {
        std::vector<t_from_or_to_inf> froms = first_stages[i_first_stage].froms;
        RRNodeId to_node = rr_graph.node_lookup().find_node(layer, actual_coordinate.x(), actual_coordinate.y(), e_rr_type::MUX, i_first_stage);
        VTR_ASSERT(to_node.is_valid());
        VTR_ASSERT(rr_gsb.is_mux_node(to_node));
        for (auto from : froms) {
            RRNodeId from_node;
            if (from.from_type == e_multistage_mux_from_or_to_type::PB) {

                if (from.type_name != vib->get_pbtype_name()) {
                    VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                    "Wrong from type name!\n");
                }

                for (e_side side : TOTAL_2D_SIDES) {
                    from_node = rr_graph.node_lookup().find_node(layer, actual_coordinate.x(), actual_coordinate.y(), e_rr_type::OPIN, from.phy_pin_index, side);
                    if (from_node.is_valid())
                        break;
                }
                if (!from_node.is_valid()) {
                    VTR_LOGF_WARN(__FILE__, __LINE__,
                                  "Can not find from node %s:%d!\n", from.type_name.c_str(), from.phy_pin_index);
                    continue;
                }
                if (!rr_gsb.is_opin_node(from_node)) {
                    VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                    "Opin node %d is not in the GSB (%d, %d)\n", from_node, rr_gsb.get_x(), rr_gsb.get_y());
                }
            } else if (from.from_type == e_multistage_mux_from_or_to_type::SEGMENT) {
                char from_dir = from.seg_dir;
                //int from_index = from.seg_index;
                t_segment_inf segment = segment_inf[from.type_index];
                VTR_ASSERT(segment.name == from.type_name);
                t_seg_group seg_group;
                for (auto seg : vib->get_seg_groups()) {
                    if (seg.name == segment.name) {
                        seg_group = seg;
                        break;
                    }
                }
                VTR_ASSERT(from.seg_index < seg_group.track_num * segment.length);
                e_side side;
                if (from_dir == 'W')
                    side = RIGHT;
                else if (from_dir == 'E')
                    side = LEFT;
                else if (from_dir == 'N')
                    side = BOTTOM;
                else if (from_dir == 'S')
                    side = TOP;
                else {
                    VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                    "Wrong segment from direction!\n");
                }

                std::vector<size_t> track_list = rr_gsb.get_chan_node_ids_by_segment_ids(side, RRSegmentId(segment.seg_index));
                if (track_list.size() == 0)
                    continue;
                else {
                    VTR_ASSERT((int)track_list.size() >= (from.seg_index + 1) * 2);
                    size_t seg_id;
                    if (side == LEFT || side == BOTTOM) { //INC
                        seg_id = from.seg_index * 2;
                    } else { //DEC
                        VTR_ASSERT(side == RIGHT || side == TOP);
                        seg_id = from.seg_index * 2 + 1;
                    }
                    from_node = rr_gsb.get_chan_node(side, track_list[seg_id]);
                    VTR_ASSERT(IN_PORT == rr_gsb.get_chan_node_direction(side, track_list[seg_id]));
                    if (!rr_gsb.is_chan_node(from_node)) {
                        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                        "Wire node %d is not in the GSB (%d, %d)\n", from_node, rr_gsb.get_x(), rr_gsb.get_y());
                    }
                }

            } else if (from.from_type == e_multistage_mux_from_or_to_type::MUX) {
                size_t from_mux_index = vib->mux_index_by_name(from.type_name);
                from_node = rr_graph.node_lookup().find_node(layer, actual_coordinate.x(), actual_coordinate.y(), e_rr_type::MUX, from_mux_index);
                if (!rr_gsb.is_mux_node(from_node)) {
                    VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                    "MUX node %d is not in the GSB (%d, %d)\n", from_node, rr_gsb.get_x(), rr_gsb.get_y());
                }
            } else {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "Wrong from type!\n");
            }
            VTR_ASSERT(from_node.is_valid());
            auto iter = vib_map.begin();
            for (; iter != vib_map.end(); ++iter) {
                if (iter->first == from_node) {
                    vib_map[from_node].push_back(to_node);
                }
            }
            if (iter == vib_map.end()) {
                std::vector<RRNodeId> to_nodes;
                to_nodes.push_back(to_node);
                vib_map.emplace(std::make_pair(from_node, to_nodes));
            }
        }
    }
    /* Second stages*/
    const std::vector<t_second_stage_mux_inf> second_stages = vib->get_second_stages();
    for (size_t i_second_stage = 0; i_second_stage < second_stages.size(); i_second_stage++) {
        std::vector<t_from_or_to_inf> froms = second_stages[i_second_stage].froms;
        std::vector<t_from_or_to_inf> tos = second_stages[i_second_stage].to;

        std::vector<RRNodeId> to_nodes;
        for (auto to : tos) {
            RRNodeId to_node;
            if (to.from_type == e_multistage_mux_from_or_to_type::PB) {

                if (to.type_name != vib->get_pbtype_name()) {
                    VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                    "Wrong to type name!\n");
                }

                for (e_side side : TOTAL_2D_SIDES) {
                    to_node = rr_graph.node_lookup().find_node(layer, actual_coordinate.x(), actual_coordinate.y(), e_rr_type::IPIN, to.phy_pin_index, side);
                    if (to_node.is_valid())
                        break;
                }
                if (!to_node.is_valid()) {
                    VTR_LOGF_WARN(__FILE__, __LINE__,
                                  "Can not find from node %s:%d!\n", to.type_name.c_str(), to.phy_pin_index);
                    continue;
                }
                if (!rr_gsb.is_ipin_node(to_node)) {
                    VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                    "MUX node %d is not in the GSB (%d, %d)\n", to_node, rr_gsb.get_x(), rr_gsb.get_y());
                }
            } else if (to.from_type == e_multistage_mux_from_or_to_type::SEGMENT) {
                char to_dir = to.seg_dir;
                //int from_index = from.seg_index;
                t_segment_inf segment = segment_inf[to.type_index];
                VTR_ASSERT(segment.name == to.type_name);
                t_seg_group seg_group;
                for (auto seg : vib->get_seg_groups()) {
                    if (seg.name == segment.name) {
                        seg_group = seg;
                        break;
                    }
                }
                VTR_ASSERT(to.seg_index < seg_group.track_num * segment.length);
                e_side side;
                if (to_dir == 'W')
                    side = LEFT;
                else if (to_dir == 'E')
                    side = RIGHT;
                else if (to_dir == 'N')
                    side = TOP;
                else if (to_dir == 'S')
                    side = BOTTOM;
                else {
                    VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                    "Wrong segment from direction!\n");
                }

                std::vector<size_t> track_list = rr_gsb.get_chan_node_ids_by_segment_ids(side, RRSegmentId(segment.seg_index));
                if (track_list.size() == 0)
                    continue;
                else {
                    //enum e_track_status track_status = determine_track_status_of_gsb
                    VTR_ASSERT((int)track_list.size() >= (to.seg_index + 1) * 2);
                    size_t seg_id;
                    if (side == LEFT || side == BOTTOM) { //DEC
                        seg_id = to.seg_index * 2 + 1;
                    } else { //INC
                        VTR_ASSERT(side == RIGHT || side == TOP);
                        seg_id = to.seg_index * 2;
                    }
                    enum e_track_status track_status = determine_track_status_of_gsb(rr_graph, rr_gsb, side, track_list[seg_id]);
                    VTR_ASSERT(track_status == TRACK_START);
                    to_node = rr_gsb.get_chan_node(side, track_list[seg_id]);
                    VTR_ASSERT(OUT_PORT == rr_gsb.get_chan_node_direction(side, track_list[seg_id]));
                    if (!rr_gsb.is_chan_node(to_node)) {
                        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                        "MUX node %d is not in the GSB (%d, %d)\n", to_node, rr_gsb.get_x(), rr_gsb.get_y());
                    }
                }

            } else {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "Wrong from type!\n");
            }
            VTR_ASSERT(to_node.is_valid());
            to_nodes.push_back(to_node);
        }

        std::vector<RRNodeId> from_nodes;
        for (auto from : froms) {
            RRNodeId from_node;
            if (from.from_type == e_multistage_mux_from_or_to_type::PB) {

                if (from.type_name != vib->get_pbtype_name()) {
                    VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                    "Wrong from type name!\n");
                }

                for (e_side side : TOTAL_2D_SIDES) {
                    from_node = rr_graph.node_lookup().find_node(layer, actual_coordinate.x(), actual_coordinate.y(), e_rr_type::OPIN, from.phy_pin_index, side);
                    if (from_node.is_valid())
                        break;
                }
                if (!from_node.is_valid()) {
                    VTR_LOGF_WARN(__FILE__, __LINE__,
                                  "Can not find from node %s:%d!\n", from.type_name.c_str(), from.phy_pin_index);
                    continue;
                }
                if (!rr_gsb.is_opin_node(from_node)) {
                    VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                    "MUX node %d is not in the GSB (%d, %d)\n", from_node, rr_gsb.get_x(), rr_gsb.get_y());
                }
            } else if (from.from_type == e_multistage_mux_from_or_to_type::SEGMENT) {
                char from_dir = from.seg_dir;
                //int from_index = from.seg_index;
                t_segment_inf segment = segment_inf[from.type_index];
                VTR_ASSERT(segment.name == from.type_name);
                t_seg_group seg_group;
                for (auto seg : vib->get_seg_groups()) {
                    if (seg.name == segment.name) {
                        seg_group = seg;
                        break;
                    }
                }
                VTR_ASSERT(from.seg_index < seg_group.track_num * segment.length);
                e_side side;
                if (from_dir == 'W')
                    side = RIGHT;
                else if (from_dir == 'E')
                    side = LEFT;
                else if (from_dir == 'N')
                    side = BOTTOM;
                else if (from_dir == 'S')
                    side = TOP;
                else {
                    VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                    "Wrong segment from direction!\n");
                }

                std::vector<size_t> track_list = rr_gsb.get_chan_node_ids_by_segment_ids(side, RRSegmentId(segment.seg_index));
                if (track_list.size() == 0)
                    continue;
                else {
                    VTR_ASSERT((int)track_list.size() >= (from.seg_index + 1) * 2);
                    size_t seg_id;
                    if (side == LEFT || side == BOTTOM) { //INC
                        seg_id = from.seg_index * 2;
                    } else { //DEC
                        VTR_ASSERT(side == RIGHT || side == TOP);
                        seg_id = from.seg_index * 2 + 1;
                    }
                    from_node = rr_gsb.get_chan_node(side, track_list[seg_id]);
                    VTR_ASSERT(IN_PORT == rr_gsb.get_chan_node_direction(side, track_list[seg_id]));
                    if (!rr_gsb.is_chan_node(from_node)) {
                        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                        "MUX node %d is not in the GSB (%d, %d)\n", from_node, rr_gsb.get_x(), rr_gsb.get_y());
                    }
                }

            } else if (from.from_type == e_multistage_mux_from_or_to_type::MUX) {
                size_t from_mux_index = vib->mux_index_by_name(from.type_name);
                from_node = rr_graph.node_lookup().find_node(layer, actual_coordinate.x(), actual_coordinate.y(), e_rr_type::MUX, from_mux_index);
                if (!rr_gsb.is_mux_node(from_node)) {
                    VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                    "MUX node %d is not in the GSB (%d, %d)\n", from_node, rr_gsb.get_x(), rr_gsb.get_y());
                }
            } else {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "Wrong from type!\n");
            }
            VTR_ASSERT(from_node.is_valid());
            from_nodes.push_back(from_node);
        }

        if (to_nodes.size() > 0 && from_nodes.size() > 0) {
            for (auto from_node : from_nodes) {
                auto iter = vib_map.begin();
                for (; iter != vib_map.end(); ++iter) {
                    if (iter->first == from_node) {
                        for (auto to_node : to_nodes) {
                            vib_map[from_node].push_back(to_node);
                        }
                    }
                }
                if (iter == vib_map.end()) {
                    vib_map.emplace(std::make_pair(from_node, to_nodes));
                }
            }
        }
    }
    return vib_map;
}

void build_edges_for_one_tileable_vib(RRGraphBuilder& rr_graph_builder,
                                      const t_vib_map& vib_map,
                                      const t_bend_track2track_map& sb_bend_conn,
                                      const vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches,
                                      size_t& num_edges_to_create) {

    size_t edge_count = 0;
    for (auto iter = vib_map.begin(); iter != vib_map.end(); ++iter) {
        for (auto to_node : iter->second) {
            rr_graph_builder.create_edge_in_cache(iter->first, to_node, rr_node_driver_switches[to_node], false);
            edge_count++;
        }
    }
    for (auto iter = sb_bend_conn.begin(); iter != sb_bend_conn.end(); ++iter) {
        rr_graph_builder.create_edge_in_cache(iter->first, iter->second, rr_node_driver_switches[iter->second], false);
        edge_count++;
    }
    num_edges_to_create += edge_count;
}
