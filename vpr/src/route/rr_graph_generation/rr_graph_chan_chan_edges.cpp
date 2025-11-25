
#include "rr_graph_chan_chan_edges.h"

#include "rr_graph_builder.h"
#include "rr_types.h"
#include "rr_rc_data.h"
#include "rr_graph_sg.h"
#include "rr_graph2.h"
#include "build_switchblocks.h"
#include "build_scatter_gathers.h"
#include "globals.h"

static void build_rr_chan(RRGraphBuilder& rr_graph_builder,
                          int layer,
                          int x_coord,
                          int y_coord,
                          e_rr_type chan_type,
                          const t_track_to_pin_lookup& track_to_pin_lookup,
                          t_sb_connection_map* sb_conn_map,
                          const vtr::NdMatrix<std::vector<int>, 3>& switch_block_conn,
                          int cost_index_offset,
                          const t_chan_width& nodes_per_chan,
                          int tracks_per_chan,
                          t_sblock_pattern& sblock_pattern,
                          int Fs_per_side,
                          const t_chan_details& chan_details_x,
                          const t_chan_details& chan_details_y,
                          t_rr_edge_info_set& rr_edges_to_create,
                          int wire_to_ipin_switch,
                          e_directionality directionality);

/**
 * @brief Collects the edges fanning-out of the 'from' track which connect to
 *  the 'to' tracks, according to the switch block pattern.
 *
 * It returns the number of connections added, and updates edge_list_ptr to
 * point at the head of the (extended) linked list giving the nodes to which
 * this segment connects and the switch type used to connect to each.
 *
 * An edge is added from this segment to a y-segment if:
 * (1) this segment should have a switch box at that location, or
 * (2) the y-segment to which it would connect has a switch box, and the switch
 *     type of that y-segment is unbuffered (bidirectional pass transistor).
 *
 * For bidirectional:
 * If the switch in each direction is a pass transistor (unbuffered), both
 * switches are marked as being of the types of the larger (lower R) pass
 * transistor.
 */
static int get_track_to_tracks(RRGraphBuilder& rr_graph_builder,
                               int layer,
                               int from_chan,
                               int from_seg,
                               int from_track,
                               e_rr_type from_type,
                               int to_seg,
                               e_rr_type to_type,
                               int chan_len,
                               int max_chan_width,
                               int Fs_per_side,
                               t_sblock_pattern& sblock_pattern,
                               RRNodeId from_rr_node,
                               t_rr_edge_info_set& rr_edges_to_create,
                               const t_chan_seg_details* from_seg_details,
                               const t_chan_seg_details* to_seg_details,
                               const t_chan_details& to_chan_details,
                               e_directionality directionality,
                               const vtr::NdMatrix<std::vector<int>, 3>& switch_block_conn,
                               const t_sb_connection_map* sb_conn_map);

/**
 * @brief Figures out the edges that should connect the given wire segment to the given channel segment,
 *        adds these edges to 'rr_edge_to_create'
 * @return The number of edges added to 'rr_edge_to_create'
 */
static int get_track_to_chan_seg(RRGraphBuilder& rr_graph_builder,
                                 int layer,
                                 int from_track,
                                 int to_chan,
                                 int to_seg,
                                 e_rr_type to_chan_type,
                                 e_side from_side,
                                 e_side to_side,
                                 int swtich_override,
                                 const t_sb_connection_map& sb_conn_map,
                                 RRNodeId from_rr_node,
                                 t_rr_edge_info_set& rr_edges_to_create);

/// @brief creates the RR graph edges corresponding to switch blocks permutation map
static void get_switchblocks_edges(RRGraphBuilder& rr_graph_builder,
                                   int tile_x,
                                   int tile_y,
                                   int layer,
                                   e_side from_side,
                                   int from_wire,
                                   RRNodeId from_rr_node,
                                   e_side to_side,
                                   int to_x,
                                   int to_y,
                                   e_rr_type to_chan_type,
                                   int switch_override,
                                   const t_sb_connection_map& sb_conn_map,
                                   t_rr_edge_info_set& rr_edges_to_create,
                                   int& edge_count);

/// Adds the fan-out edges from wire segment at (chan, seg, track) to adjacent IPINs along the wire's length
static int get_track_to_ipins(RRGraphBuilder& rr_graph_builder,
                              int layer,
                              int seg,
                              int chan,
                              int track,
                              int tracks_per_chan,
                              RRNodeId from_rr_node,
                              t_rr_edge_info_set& rr_edges_to_create,
                              const t_track_to_pin_lookup& track_to_pin_lookup,
                              const t_chan_seg_details* seg_details,
                              e_rr_type chan_type,
                              int chan_length,
                              int wire_to_ipin_switch,
                              e_directionality directionality);

//Returns how the switch type for the switch block at the specified location should be created
//  from_chan_coord: The horizontal or vertical channel index (i.e. x-coord for CHANY, y-coord for CHANX)
//  from_seg_coord: The horizontal or vertical location along the channel (i.e. y-coord for CHANY, x-coord for CHANX)
//  from_chan_type: The from channel type
//  to_chan_type: The to channel type
static int should_create_switchblock(int layer_num,
                                     int from_chan_coord,
                                     int from_seg_coord,
                                     e_rr_type from_chan_type,
                                     e_rr_type to_chan_type);

static int get_unidir_track_to_chan_seg(RRGraphBuilder& rr_graph_builder,
                                        int layer,
                                        int from_track,
                                        int to_chan,
                                        int to_seg,
                                        int to_sb,
                                        e_rr_type to_type,
                                        int max_chan_width,
                                        e_side from_side,
                                        e_side to_side,
                                        int Fs_per_side,
                                        t_sblock_pattern& sblock_pattern,
                                        int switch_override,
                                        const t_chan_seg_details* seg_details,
                                        bool* Fs_clipped,
                                        RRNodeId from_rr_node,
                                        t_rr_edge_info_set& rr_edges_to_create);

static int get_bidir_track_to_chan_seg(RRGraphBuilder& rr_graph_builder,
                                       const std::vector<int>& conn_tracks,
                                       int layer,
                                       int to_chan,
                                       int to_seg,
                                       int to_sb,
                                       e_rr_type to_type,
                                       const t_chan_seg_details* seg_details,
                                       bool from_is_sblock,
                                       int from_switch,
                                       int switch_override,
                                       e_directionality directionality,
                                       RRNodeId from_rr_node,
                                       t_rr_edge_info_set& rr_edges_to_create);

static int vpr_to_phy_track(int itrack,
                            int chan_num,
                            int seg_num,
                            const t_chan_seg_details* seg_details,
                            e_directionality directionality);

static void get_switch_type(bool is_from_sb,
                            bool is_to_sb,
                            short from_node_switch,
                            short to_node_switch,
                            int switch_override,
                            short switch_types[2]);

static bool should_apply_switch_override(int switch_override);

/* Allocates/loads edges for nodes belonging to specified channel segment and initializes
 * node properties such as cost, occupancy and capacity */
static void build_rr_chan(RRGraphBuilder& rr_graph_builder,
                          int layer,
                          int x_coord,
                          int y_coord,
                          e_rr_type chan_type,
                          const t_track_to_pin_lookup& track_to_pin_lookup,
                          t_sb_connection_map* sb_conn_map,
                          const vtr::NdMatrix<std::vector<int>, 3>& switch_block_conn,
                          int cost_index_offset,
                          const t_chan_width& nodes_per_chan,
                          int tracks_per_chan,
                          t_sblock_pattern& sblock_pattern,
                          int Fs_per_side,
                          const t_chan_details& chan_details_x,
                          const t_chan_details& chan_details_y,
                          t_rr_edge_info_set& rr_edges_to_create,
                          int wire_to_ipin_switch,
                          e_directionality directionality) {
    // this function builds both x and y-directed channel segments, so set up our coordinates based on channel type

    const auto& device_ctx = g_vpr_ctx.device();
    auto& mutable_device_ctx = g_vpr_ctx.mutable_device();

    // Initially assumes CHANX
    int seg_coord = x_coord;                           //The absolute coordinate of this segment within the channel
    int chan_coord = y_coord;                          //The absolute coordinate of this channel within the device
    int seg_dimension = device_ctx.grid.width() - 2;   //-2 for no perim channels
    int chan_dimension = device_ctx.grid.height() - 2; //-2 for no perim channels
    const t_chan_details& from_chan_details = (chan_type == e_rr_type::CHANX) ? chan_details_x : chan_details_y;
    const t_chan_details& opposite_chan_details = (chan_type == e_rr_type::CHANX) ? chan_details_y : chan_details_x;
    e_rr_type opposite_chan_type = e_rr_type::CHANY;
    if (chan_type == e_rr_type::CHANY) {
        //Swap values since CHANX was assumed above
        std::swap(seg_coord, chan_coord);
        std::swap(seg_dimension, chan_dimension);
        opposite_chan_type = e_rr_type::CHANX;
    }

    const t_chan_seg_details* seg_details = from_chan_details[x_coord][y_coord].data();

    // figure out if we're generating switch block edges based on a custom switch block description
    bool custom_switch_block = false;
    if (sb_conn_map != nullptr) {
        VTR_ASSERT(sblock_pattern.empty() && switch_block_conn.empty());
        custom_switch_block = true;
    }

    // Loads up all the routing resource nodes in the current channel segment
    for (int track = 0; track < tracks_per_chan; ++track) {
        if (seg_details[track].length() == 0)
            continue;

        // Start and end coordinates of this segment along the length of the channel
        // Note that these values are in the VPR coordinate system (and do not consider
        // wire directionality), so start correspond to left/bottom and end corresponds to right/top
        int start = get_seg_start(seg_details, track, chan_coord, seg_coord);
        int end = get_seg_end(seg_details, track, start, chan_coord, seg_dimension);

        if (seg_coord > start) {
            continue; // Only process segments which start at this location
        }
        VTR_ASSERT(seg_coord == start);

        const t_chan_seg_details* from_seg_details = nullptr;
        if (chan_type == e_rr_type::CHANY) {
            from_seg_details = chan_details_y[x_coord][start].data();
        } else {
            from_seg_details = chan_details_x[start][y_coord].data();
        }

        RRNodeId node = rr_graph_builder.node_lookup().find_node(layer, x_coord, y_coord, chan_type, track);

        if (!node) {
            continue;
        }

        // Add the edges from this track to all it's connected pins into the list
        get_track_to_ipins(rr_graph_builder, layer, start, chan_coord, track, tracks_per_chan, node, rr_edges_to_create,
                           track_to_pin_lookup, seg_details, chan_type, seg_dimension,
                           wire_to_ipin_switch, directionality);

        // Add edges going from the current track into channel segments which are perpendicular to it
        if (chan_coord > 0) {
            const t_chan_seg_details* to_seg_details;
            int max_opposite_chan_width;
            if (chan_type == e_rr_type::CHANX) {
                to_seg_details = chan_details_y[start][y_coord].data();
                max_opposite_chan_width = nodes_per_chan.y_max;
            } else {
                VTR_ASSERT(chan_type == e_rr_type::CHANY);
                to_seg_details = chan_details_x[x_coord][start].data();
                max_opposite_chan_width = nodes_per_chan.x_max;
            }
            if (to_seg_details->length() > 0) {
                get_track_to_tracks(rr_graph_builder, layer, chan_coord, start, track, chan_type, chan_coord,
                                    opposite_chan_type, seg_dimension, max_opposite_chan_width,
                                    Fs_per_side, sblock_pattern, node, rr_edges_to_create,
                                    from_seg_details, to_seg_details, opposite_chan_details,
                                    directionality,
                                    switch_block_conn, sb_conn_map);
            }
        }

        if (chan_coord < chan_dimension) {
            const t_chan_seg_details* to_seg_details;
            int max_opposite_chan_width = 0;
            if (chan_type == e_rr_type::CHANX) {
                to_seg_details = chan_details_y[start][y_coord + 1].data();
                max_opposite_chan_width = nodes_per_chan.y_max;
            } else {
                VTR_ASSERT(chan_type == e_rr_type::CHANY);
                to_seg_details = chan_details_x[x_coord + 1][start].data();
                max_opposite_chan_width = nodes_per_chan.x_max;
            }
            if (to_seg_details->length() > 0) {
                get_track_to_tracks(rr_graph_builder, layer, chan_coord, start, track, chan_type, chan_coord + 1,
                                    opposite_chan_type, seg_dimension, max_opposite_chan_width,
                                    Fs_per_side, sblock_pattern, node, rr_edges_to_create,
                                    from_seg_details, to_seg_details, opposite_chan_details,
                                    directionality, switch_block_conn, sb_conn_map);
            }
        }

        // walk over the switch blocks along the source track and implement edges from this track to other tracks in the same channel (i.e. straight-through connections)
        for (int target_seg = start - 1; target_seg <= end + 1; target_seg++) {
            if (target_seg != start - 1 && target_seg != end + 1) {
                // skip straight-through connections from midpoint if non-custom switch block.
                // currently non-custom switch blocks don't properly describe connections from the mid-point of a wire segment
                // to other segments in the same channel (i.e. straight-through connections)
                if (!custom_switch_block) {
                    continue;
                }
            }
            if (target_seg > 0 && target_seg < seg_dimension + 1) {
                const t_chan_seg_details* to_seg_details;
                // Same channel width for straight through connections assuming uniform width distributions along the axis
                int max_chan_width = 0;
                if (chan_type == e_rr_type::CHANX) {
                    to_seg_details = chan_details_x[target_seg][y_coord].data();
                    max_chan_width = nodes_per_chan.x_max;
                } else {
                    VTR_ASSERT(chan_type == e_rr_type::CHANY);
                    to_seg_details = chan_details_y[x_coord][target_seg].data();
                    max_chan_width = nodes_per_chan.y_max;
                }
                if (to_seg_details->length() > 0) {
                    get_track_to_tracks(rr_graph_builder, layer, chan_coord, start, track, chan_type, target_seg,
                                        chan_type, seg_dimension, max_chan_width,
                                        Fs_per_side, sblock_pattern, node, rr_edges_to_create,
                                        from_seg_details, to_seg_details, from_chan_details,
                                        directionality,
                                        switch_block_conn, sb_conn_map);
                }
            }
        }

        // Edge arrays have now been built up.  Do everything else.
        // The cost_index should be w.r.t the index of the segment to its **parallel** segment_inf vector.
        // Note that when building channels, we use the indices w.r.t segment_inf_x and segment_inf_y as
        // computed earlier in build_rr_graph so it's fine to use .index() for to get the correct index.
        rr_graph_builder.set_node_cost_index(node, RRIndexedDataId(cost_index_offset + seg_details[track].index()));
        rr_graph_builder.set_node_capacity(node, 1); // GLOBAL routing handled elsewhere

        if (chan_type == e_rr_type::CHANX) {
            rr_graph_builder.set_node_coordinates(node, start, y_coord, end, y_coord);
        } else {
            VTR_ASSERT(chan_type == e_rr_type::CHANY);
            rr_graph_builder.set_node_coordinates(node, x_coord, start, x_coord, end);
        }

        rr_graph_builder.set_node_layer(node, layer, layer);

        int length = end - start + 1;
        float R = length * seg_details[track].Rmetal();
        float C = length * seg_details[track].Cmetal();
        rr_graph_builder.set_node_rc_index(node, find_create_rr_rc_data(R, C, mutable_device_ctx.rr_rc_data));

        rr_graph_builder.set_node_type(node, chan_type);
        rr_graph_builder.set_node_track_num(node, track);
        rr_graph_builder.set_node_direction(node, seg_details[track].direction());
    }
}

static int get_track_to_tracks(RRGraphBuilder& rr_graph_builder,
                               int layer,
                               int from_chan,
                               int from_seg,
                               int from_track,
                               e_rr_type from_type,
                               int to_seg,
                               e_rr_type to_type,
                               int chan_len,
                               int max_chan_width,
                               int Fs_per_side,
                               t_sblock_pattern& sblock_pattern,
                               RRNodeId from_rr_node,
                               t_rr_edge_info_set& rr_edges_to_create,
                               const t_chan_seg_details* from_seg_details,
                               const t_chan_seg_details* to_seg_details,
                               const t_chan_details& to_chan_details,
                               e_directionality directionality,
                               const vtr::NdMatrix<std::vector<int>, 3>& switch_block_conn,
                               const t_sb_connection_map* sb_conn_map) {
    int to_chan, to_sb;
    std::vector<int> conn_tracks;
    bool Fs_clipped;
    e_side to_side;

    // check whether a custom switch block will be used
    bool custom_switch_block = false;
    if (sb_conn_map != nullptr) {
        custom_switch_block = true;
        VTR_ASSERT(switch_block_conn.empty());
    }

    VTR_ASSERT_MSG(from_seg == get_seg_start(from_seg_details, from_track, from_chan, from_seg),
                   "From segment location must be a the wire start point");

    int from_switch = from_seg_details[from_track].arch_wire_switch();

    // The absolute coordinate along the channel where the switch block at the
    // beginning of the current wire segment is located
    int start_sb_seg = from_seg - 1;

    // The absolute coordinate along the channel where the switch block at the
    // end of the current wire segment is located
    int end_sb_seg = get_seg_end(from_seg_details, from_track, from_seg, from_chan, chan_len);

    // Figure out the sides of SB the from_wire will use
    e_side from_side_a, from_side_b;
    if (e_rr_type::CHANX == from_type) {
        from_side_a = RIGHT;
        from_side_b = LEFT;
    } else {
        VTR_ASSERT(e_rr_type::CHANY == from_type);
        from_side_a = TOP;
        from_side_b = BOTTOM;
    }

    // Set the loop bounds, so we iterate over the whole wire segment
    int start = start_sb_seg;
    int end = end_sb_seg;

    // If source and destination segments both lie along the same channel
    // we clip the loop bounds to the switch blocks of interest and proceed normally
    if (to_type == from_type) {
        start = to_seg - 1;
        end = to_seg;
    }

    // Walk along the 'from' wire segment identifying if a switchblock is located
    // at each coordinate and add any related fan-out connections to the 'from' wire segment
    int num_conn = 0;
    for (int sb_seg = start; sb_seg <= end; ++sb_seg) {
        if (sb_seg < start_sb_seg || sb_seg > end_sb_seg) {
            continue;
        }

        // Figure out if we are at a sblock
        bool from_is_sblock = is_sblock(from_chan, from_seg, sb_seg, from_track,
                                        from_seg_details, directionality);
        if (sb_seg == end_sb_seg || sb_seg == start_sb_seg) {
            // end of wire must be an sblock
            from_is_sblock = true;
        }

        int switch_override = should_create_switchblock(layer, from_chan, sb_seg, from_type, to_type);
        if (switch_override == NO_SWITCH) {
            continue; //Do not create an SB here
        }

        // Get the coordinates of the current SB from the perspective of the destination channel.
        // i.e. for segments laid in the x-direction, sb_seg corresponds to the x coordinate and from_chan to the y,
        // but for segments in the y-direction, from_chan is the x coordinate and sb_seg is the y. So here we reverse
        // the coordinates if necessary
        if (from_type == to_type) {
            // Same channel
            to_chan = from_chan;
            to_sb = sb_seg;
        } else {
            VTR_ASSERT(from_type != to_type);
            // Different channels
            to_chan = sb_seg;
            to_sb = from_chan;
        }

        // to_chan_details may correspond to an x-directed or y-directed channel, depending on which
        // channel type this function is used; so coordinates are reversed as necessary
        if (to_type == e_rr_type::CHANX) {
            to_seg_details = to_chan_details[to_seg][to_chan].data();
        } else {
            to_seg_details = to_chan_details[to_chan][to_seg].data();
        }

        if (to_seg_details[0].length() == 0)
            continue;

        // Figure out whether the switch block at the current sb_seg coordinate is *behind*
        // the target channel segment (with respect to VPR coordinate system)
        bool is_behind = false;
        if (to_type == from_type) {
            if (sb_seg == start) {
                is_behind = true;
            } else {
                is_behind = false;
            }
        } else {
            VTR_ASSERT((to_seg == from_chan) || (to_seg == (from_chan + 1)));
            if (to_seg > from_chan) {
                is_behind = true;
            }
        }

        // Figure out which side of the SB the destination segment lies on
        if (e_rr_type::CHANX == to_type) {
            to_side = (is_behind ? RIGHT : LEFT);
        } else {
            VTR_ASSERT(e_rr_type::CHANY == to_type);
            to_side = (is_behind ? TOP : BOTTOM);
        }

        // To get to the destination seg/chan, the source track can connect to the SB from
        // one of two directions. If we're in CHANX, we can connect to it from the left or
        // right, provided we're not at a track endpoint. And similarly for a source track
        // in CHANY.
        // Do edges going from the right SB side (if we're in CHANX) or top (if we're in CHANY).
        // However, can't connect to right (top) if already at rightmost (topmost) track end
        if (sb_seg < end_sb_seg) {
            if (custom_switch_block) {
                if (Direction::DEC == from_seg_details[from_track].direction() || BI_DIRECTIONAL == directionality) {
                    num_conn += get_track_to_chan_seg(rr_graph_builder, layer, from_track, to_chan, to_seg,
                                                      to_type, from_side_a, to_side,
                                                      switch_override,
                                                      *sb_conn_map, from_rr_node, rr_edges_to_create);
                }
            } else {
                if (BI_DIRECTIONAL == directionality) {
                    // For bidir, the target segment might have an unbuffered (bidir pass transistor)
                    // switchbox, so we follow through regardless of whether the current segment has an SB
                    conn_tracks = switch_block_conn[from_side_a][to_side][from_track];
                    num_conn += get_bidir_track_to_chan_seg(rr_graph_builder, conn_tracks, layer,
                                                            to_chan, to_seg, to_sb, to_type,
                                                            to_seg_details, from_is_sblock, from_switch,
                                                            switch_override,
                                                            directionality, from_rr_node, rr_edges_to_create);
                }
                if (UNI_DIRECTIONAL == directionality) {
                    // No fanout if no SB.
                    // Also, we are connecting from the top or right of SB so it
                    // makes the most sense to only get there from Direction::DEC wires.
                    if ((from_is_sblock) && (Direction::DEC == from_seg_details[from_track].direction())) {
                        num_conn += get_unidir_track_to_chan_seg(rr_graph_builder, layer, from_track, to_chan,
                                                                 to_seg, to_sb, to_type, max_chan_width,
                                                                 from_side_a, to_side, Fs_per_side,
                                                                 sblock_pattern,
                                                                 switch_override,
                                                                 to_seg_details,
                                                                 &Fs_clipped, from_rr_node, rr_edges_to_create);
                    }
                }
            }
        }

        // Do the edges going from the left SB side (if we're in CHANX) or bottom (if we're in CHANY)
        // However, can't connect to left (bottom) if already at leftmost (bottommost) track end
        if (sb_seg > start_sb_seg) {
            if (custom_switch_block) {
                if (Direction::INC == from_seg_details[from_track].direction() || BI_DIRECTIONAL == directionality) {
                    num_conn += get_track_to_chan_seg(rr_graph_builder, layer, from_track, to_chan, to_seg,
                                                      to_type, from_side_b, to_side,
                                                      switch_override,
                                                      *sb_conn_map, from_rr_node, rr_edges_to_create);
                }
            } else {
                if (BI_DIRECTIONAL == directionality) {
                    // For bidir, the target segment might have an unbuffered (bidir pass transistor)
                    // switchbox, so we follow through regardless of whether the current segment has an SB
                    conn_tracks = switch_block_conn[from_side_b][to_side][from_track];
                    num_conn += get_bidir_track_to_chan_seg(rr_graph_builder, conn_tracks, layer,
                                                            to_chan, to_seg, to_sb, to_type,
                                                            to_seg_details, from_is_sblock, from_switch,
                                                            switch_override,
                                                            directionality, from_rr_node, rr_edges_to_create);
                }
                if (UNI_DIRECTIONAL == directionality) {
                    // No fanout if no SB.
                    // Also, we are connecting from the bottom or left of SB so it
                    // makes the most sense to only get there from Direction::INC wires.
                    if ((from_is_sblock)
                        && (Direction::INC == from_seg_details[from_track].direction())) {
                        num_conn += get_unidir_track_to_chan_seg(rr_graph_builder, layer, from_track, to_chan,
                                                                 to_seg, to_sb, to_type, max_chan_width,
                                                                 from_side_b, to_side, Fs_per_side,
                                                                 sblock_pattern,
                                                                 switch_override,
                                                                 to_seg_details,
                                                                 &Fs_clipped, from_rr_node, rr_edges_to_create);
                    }
                }
            }
        }
    }

    return num_conn;
}

static int get_track_to_chan_seg(RRGraphBuilder& rr_graph_builder,
                                 int layer,
                                 int from_wire,
                                 int to_chan,
                                 int to_seg,
                                 e_rr_type to_chan_type,
                                 e_side from_side,
                                 e_side to_side,
                                 int switch_override,
                                 const t_sb_connection_map& sb_conn_map,
                                 RRNodeId from_rr_node,
                                 t_rr_edge_info_set& rr_edges_to_create) {
    int edge_count = 0;
    int to_x, to_y;
    int tile_x, tile_y;

    // Get x/y coordinates from seg/chan coordinates
    if (e_rr_type::CHANX == to_chan_type) {
        to_x = tile_x = to_seg;
        to_y = tile_y = to_chan;
        if (RIGHT == to_side) {
            tile_x--;
        }
    } else {
        VTR_ASSERT(e_rr_type::CHANY == to_chan_type);
        to_x = tile_x = to_chan;
        to_y = tile_y = to_seg;
        if (TOP == to_side) {
            tile_y--;
        }
    }

    get_switchblocks_edges(rr_graph_builder,
                           tile_x,
                           tile_y,
                           layer,
                           from_side,
                           from_wire,
                           from_rr_node,
                           to_side,
                           to_x,
                           to_y,
                           to_chan_type,
                           switch_override,
                           sb_conn_map,
                           rr_edges_to_create,
                           edge_count);

    return edge_count;
}

static void get_switchblocks_edges(RRGraphBuilder& rr_graph_builder,
                                   int tile_x,
                                   int tile_y,
                                   int layer,
                                   e_side from_side,
                                   int from_wire,
                                   RRNodeId from_rr_node,
                                   e_side to_side,
                                   int to_x,
                                   int to_y,
                                   e_rr_type to_chan_type,
                                   int switch_override,
                                   const t_sb_connection_map& sb_conn_map,
                                   t_rr_edge_info_set& rr_edges_to_create,
                                   int& edge_count) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();

    // Coordinate to index into the SB map
    SwitchblockLookupKey sb_coord(tile_x, tile_y, layer, from_side, to_side);
    if (sb_conn_map.contains(sb_coord)) {
        // Reference to the connections vector which lists all destination wires for a given source wire
        // at a specific coordinate sb_coord
        const std::vector<t_switchblock_edge>& sb_edges = sb_conn_map.at(sb_coord);

        // Go through the connections...
        for (const t_switchblock_edge& sb_edge : sb_edges) {
            if (sb_edge.from_wire != from_wire) continue;

            int to_wire = sb_edge.to_wire;
            // Get the index of the switch connecting the two wires
            int src_switch = sb_edge.switch_ind;

            RRNodeId to_node = rr_graph_builder.node_lookup().find_node(layer, to_x, to_y, to_chan_type, to_wire);

            if (!to_node) {
                continue;
            }

            // Apply any switch overrides
            if (should_apply_switch_override(switch_override)) {
                src_switch = switch_override;
            }

            rr_edges_to_create.emplace_back(from_rr_node, to_node, src_switch, false);
            ++edge_count;

            if (device_ctx.arch_switch_inf[src_switch].directionality() == BI_DIRECTIONAL) {
                // Add reverse edge since bidirectional
                rr_edges_to_create.emplace_back(to_node, from_rr_node, src_switch, false);
                ++edge_count;
            }
        }
    }
}

static int get_track_to_ipins(RRGraphBuilder& rr_graph_builder,
                              int layer,
                              int seg,
                              int chan,
                              int track,
                              int tracks_per_chan,
                              RRNodeId from_rr_node,
                              t_rr_edge_info_set& rr_edges_to_create,
                              const t_track_to_pin_lookup& track_to_pin_lookup,
                              const t_chan_seg_details* seg_details,
                              e_rr_type chan_type,
                              int chan_length,
                              int wire_to_ipin_switch,
                              e_directionality directionality) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();

    // End of this wire
    int end = get_seg_end(seg_details, track, seg, chan, chan_length);

    int num_conn = 0;

    for (int j = seg; j <= end; j++) {
        if (is_cblock(chan, j, track, seg_details)) {
            for (int pass = 0; pass < 2; ++pass) { //pass == 0 => TOP/RIGHT, pass == 1 => BOTTOM/LEFT
                e_side side;
                int x, y;
                if (e_rr_type::CHANX == chan_type) {
                    x = j;
                    y = chan + pass;
                    side = (0 == pass ? TOP : BOTTOM);
                } else {
                    VTR_ASSERT(e_rr_type::CHANY == chan_type);
                    x = chan + pass;
                    y = j;
                    side = (0 == pass ? RIGHT : LEFT);
                }

                // if the pointed to is an EMPTY then shouldn't look for ipins
                t_physical_tile_type_ptr type = device_ctx.grid.get_physical_type({x, y, layer});
                if (type == device_ctx.EMPTY_PHYSICAL_TILE_TYPE)
                    continue;

                /* Move from logical (straight) to physical (twisted) track index
                 * - algorithm assigns ipin connections to same physical track index
                 * so that the logical track gets distributed uniformly */

                int phy_track = vpr_to_phy_track(track, chan, j, seg_details, directionality);
                phy_track %= tracks_per_chan;

                /* We need the type to find the ipin map for this type */

                int width_offset = device_ctx.grid.get_width_offset({x, y, layer});
                int height_offset = device_ctx.grid.get_height_offset({x, y, layer});

                const int max_conn = track_to_pin_lookup[type->index][phy_track][width_offset][height_offset][side].size();
                for (int iconn = 0; iconn < max_conn; iconn++) {
                    const int ipin = track_to_pin_lookup[type->index][phy_track][width_offset][height_offset][side][iconn];

                    // Check there is a connection and Fc map isn't wrong
                    RRNodeId to_node = rr_graph_builder.node_lookup().find_node(layer, x, y, e_rr_type::IPIN, ipin, side);
                    if (to_node) {
                        rr_edges_to_create.emplace_back(from_rr_node, to_node, wire_to_ipin_switch, false);
                        ++num_conn;
                    }
                }
            }
        }
    }

    return num_conn;
}

static int should_create_switchblock(int layer_num, int from_chan_coord, int from_seg_coord, e_rr_type from_chan_type, e_rr_type to_chan_type) {
    const DeviceGrid& grid = g_vpr_ctx.device().grid;

    // Convert the chan/seg indices to real x/y coordinates
    int y_coord;
    int x_coord;
    if (from_chan_type == e_rr_type::CHANX) {
        y_coord = from_chan_coord;
        x_coord = from_seg_coord;
    } else {
        VTR_ASSERT(from_chan_type == e_rr_type::CHANY);
        y_coord = from_seg_coord;
        x_coord = from_chan_coord;
    }

    t_physical_tile_type_ptr blk_type = grid.get_physical_type({x_coord, y_coord, layer_num});
    int width_offset = grid.get_width_offset({x_coord, y_coord, layer_num});
    int height_offset = grid.get_height_offset({x_coord, y_coord, layer_num});

    e_sb_type sb_type = blk_type->switchblock_locations[width_offset][height_offset];
    int switch_override = blk_type->switchblock_switch_overrides[width_offset][height_offset];

    if (sb_type == e_sb_type::FULL) {
        return switch_override;
    } else if (sb_type == e_sb_type::STRAIGHT && from_chan_type == to_chan_type) {
        return switch_override;
    } else if (sb_type == e_sb_type::TURNS && from_chan_type != to_chan_type) {
        return switch_override;
    } else if (sb_type == e_sb_type::HORIZONTAL && from_chan_type == e_rr_type::CHANX && to_chan_type == e_rr_type::CHANX) {
        return switch_override;
    } else if (sb_type == e_sb_type::VERTICAL && from_chan_type == e_rr_type::CHANY && to_chan_type == e_rr_type::CHANY) {
        return switch_override;
    }

    return NO_SWITCH;
}

static int get_unidir_track_to_chan_seg(RRGraphBuilder& rr_graph_builder,
                                        int layer,
                                        int from_track,
                                        int to_chan,
                                        int to_seg,
                                        int to_sb,
                                        e_rr_type to_type,
                                        int max_chan_width,
                                        e_side from_side,
                                        e_side to_side,
                                        int Fs_per_side,
                                        t_sblock_pattern& sblock_pattern,
                                        int switch_override,
                                        const t_chan_seg_details* seg_details,
                                        bool* Fs_clipped,
                                        RRNodeId from_rr_node,
                                        t_rr_edge_info_set& rr_edges_to_create) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const DeviceGrid& grid = device_ctx.grid;

    int num_labels = 0;
    std::vector<int> mux_labels;

    // x, y coords for get_rr_node lookups
    int to_x = (e_rr_type::CHANX == to_type ? to_seg : to_chan);
    int to_y = (e_rr_type::CHANX == to_type ? to_chan : to_seg);
    int sb_x = (e_rr_type::CHANX == to_type ? to_sb : to_chan);
    int sb_y = (e_rr_type::CHANX == to_type ? to_chan : to_sb);
    int max_len = (e_rr_type::CHANX == to_type ? grid.width() : grid.height()) - 2; //-2 for no perimeter channels

    enum Direction to_dir = Direction::DEC;
    if (to_sb < to_seg) {
        to_dir = Direction::INC;
    }

    *Fs_clipped = false;

    /* get list of muxes to which we can connect */
    int dummy;
    label_wire_muxes(to_chan, to_seg, seg_details, UNDEFINED, max_len,
                     to_dir, max_chan_width, false, mux_labels, &num_labels, &dummy);

    /* Can't connect if no muxes. */
    if (num_labels < 1) {
        return 0;
    }

    /* Check if Fs demand was too high. */
    if (Fs_per_side > num_labels) {
        *Fs_clipped = true;
    }

    // Handle Fs > 3 by assigning consecutive muxes.
    int count = 0;
    for (int i = 0; i < Fs_per_side; ++i) {
        // Get the target label
        for (int j = 0; j < 4; j = j + 2) {
            // Use the balanced labeling for passing and fringe wires
            int to_mux = sblock_pattern[sb_x][sb_y][from_side][to_side][from_track][j];
            if (to_mux < 0) {
                continue;
            }

            int to_track = sblock_pattern[sb_x][sb_y][from_side][to_side][from_track][j + 1];
            if (to_track < 0) {
                to_track = mux_labels[(to_mux + i) % num_labels];
                sblock_pattern[sb_x][sb_y][from_side][to_side][from_track][j + 1] = to_track;
            }
            RRNodeId to_node = rr_graph_builder.node_lookup().find_node(layer, to_x, to_y, to_type, to_track);

            if (!to_node) {
                continue;
            }

            // Determine which switch to use
            int iswitch = seg_details[to_track].arch_wire_switch();

            // Apply any switch overrides
            if (should_apply_switch_override(switch_override)) {
                iswitch = switch_override;
            }
            VTR_ASSERT(iswitch != UNDEFINED);

            // Add edge to list.
            rr_edges_to_create.emplace_back(from_rr_node, to_node, iswitch, false);
            ++count;

            if (device_ctx.arch_switch_inf[iswitch].directionality() == BI_DIRECTIONAL) {
                // Add reverse edge since bidirectional
                rr_edges_to_create.emplace_back(to_node, from_rr_node, iswitch, false);
                ++count;
            }
        }
    }

    return count;
}

static int get_bidir_track_to_chan_seg(RRGraphBuilder& rr_graph_builder,
                                       const std::vector<int>& conn_tracks,
                                       int layer,
                                       int to_chan,
                                       int to_seg,
                                       int to_sb,
                                       e_rr_type to_type,
                                       const t_chan_seg_details* seg_details,
                                       bool from_is_sblock,
                                       int from_switch,
                                       int switch_override,
                                       e_directionality directionality,
                                       RRNodeId from_rr_node,
                                       t_rr_edge_info_set& rr_edges_to_create) {
    int to_x, to_y;
    short switch_types[2];

    // x, y coords for get_rr_node lookups
    if (e_rr_type::CHANX == to_type) {
        to_x = to_seg;
        to_y = to_chan;
    } else {
        VTR_ASSERT(e_rr_type::CHANY == to_type);
        to_x = to_chan;
        to_y = to_seg;
    }

    // Go through the list of tracks we can connect to
    int num_conn = 0;
    for (int to_track : conn_tracks) {
        RRNodeId to_node = rr_graph_builder.node_lookup().find_node(layer, to_x, to_y, to_type, to_track);

        if (!to_node) {
            continue;
        }

        // Get the switches for any edges between the two tracks
        int to_switch = seg_details[to_track].arch_wire_switch();

        bool to_is_sblock = is_sblock(to_chan, to_seg, to_sb, to_track, seg_details, directionality);
        get_switch_type(from_is_sblock, to_is_sblock, from_switch, to_switch,
                        switch_override,
                        switch_types);

        // There are up to two switch edges allowed from track to track
        for (int i = 0; i < 2; ++i) {
            // If the switch_type entry is empty, skip it
            if (UNDEFINED == switch_types[i]) {
                continue;
            }

            // Add the edge to the list
            rr_edges_to_create.emplace_back(from_rr_node, to_node, switch_types[i], false);
            ++num_conn;
        }
    }

    return num_conn;
}

static int vpr_to_phy_track(int itrack,
                            int chan_num,
                            int seg_num,
                            const t_chan_seg_details* seg_details,
                            e_directionality directionality) {

    // Assign in pairs if unidir.
    int fac = 1;
    if (UNI_DIRECTIONAL == directionality) {
        fac = 2;
    }

    int group_start = seg_details[itrack].group_start();
    int group_size = seg_details[itrack].group_size();

    int vpr_offset_for_first_phy_track = (chan_num + seg_num - 1) % (group_size / fac);
    int vpr_offset = (itrack - group_start) / fac;
    int phy_offset = (vpr_offset_for_first_phy_track + vpr_offset) % (group_size / fac);
    int phy_track = group_start + (fac * phy_offset) + (itrack - group_start) % fac;

    return phy_track;
}

static void get_switch_type(bool is_from_sblock,
                            bool is_to_sblock,
                            short from_node_switch,
                            short to_node_switch,
                            int switch_override,
                            short switch_types[2]) {
    /* This routine looks at whether the from_node and to_node want a switch,  *
     * and what type of switch is used to connect *to* each type of node       *
     * (from_node_switch and to_node_switch).  It decides what type of switch, *
     * if any, should be used to go from from_node to to_node.  If no switch   *
     * should be inserted (i.e. no connection), it returns NO_SWITCH.  Its returned *
     * values are in the switch_types array.  It needs to return an array      *
     * because some topologies (e.g. bi-dir pass gates) result in two switches.*/

    auto& device_ctx = g_vpr_ctx.device();

    switch_types[0] = NO_SWITCH;
    switch_types[1] = NO_SWITCH;

    if (switch_override == NO_SWITCH) {
        return; //No switches
    }

    if (should_apply_switch_override(switch_override)) {
        //Use the override switches instead
        from_node_switch = switch_override;
        to_node_switch = switch_override;
    }

    int used = 0;
    bool forward_switch = false;
    bool backward_switch = false;

    /* Connect forward if we are a sblock */
    if (is_from_sblock) {
        switch_types[used] = to_node_switch;
        ++used;

        forward_switch = true;
    }

    /* Check for reverse switch */
    if (is_to_sblock) {
        if (device_ctx.arch_switch_inf[from_node_switch].directionality() == e_directionality::BI_DIRECTIONAL) {
            switch_types[used] = from_node_switch;
            ++used;

            backward_switch = true;
        }
    }

    /* Take the larger switch if there are two of the same type */
    if (forward_switch
        && backward_switch
        && (device_ctx.arch_switch_inf[from_node_switch].type() == device_ctx.arch_switch_inf[to_node_switch].type())) {
        //Sanity checks
        VTR_ASSERT_SAFE_MSG(device_ctx.arch_switch_inf[from_node_switch].type() == device_ctx.arch_switch_inf[to_node_switch].type(), "Same switch type");
        VTR_ASSERT_MSG(device_ctx.arch_switch_inf[to_node_switch].directionality() == e_directionality::BI_DIRECTIONAL, "Bi-dir to switch");
        VTR_ASSERT_MSG(device_ctx.arch_switch_inf[from_node_switch].directionality() == e_directionality::BI_DIRECTIONAL, "Bi-dir from switch");

        /* Take the smaller index unless the other
         * switch is bigger (smaller R). */

        int first_switch = std::min(to_node_switch, from_node_switch);
        int second_switch = std::max(to_node_switch, from_node_switch);

        if (used < 2) {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "Expected 2 switches (forward and back) between RR nodes (found %d switches, min switch index: %d max switch index: %d)",
                            used, first_switch, second_switch);
        }

        int switch_to_use = first_switch;
        if (device_ctx.arch_switch_inf[second_switch].R < device_ctx.arch_switch_inf[first_switch].R) {
            switch_to_use = second_switch;
        }

        for (int i = 0; i < used; ++i) {
            switch_types[i] = switch_to_use;
        }
    }
}

static bool should_apply_switch_override(int switch_override) {
    if (switch_override != NO_SWITCH && switch_override != DEFAULT_SWITCH) {
        VTR_ASSERT(switch_override >= 0);
        return true;
    }
    return false;
}

void add_chan_chan_edges(RRGraphBuilder& rr_graph_builder,
                         size_t num_seg_types_x,
                         size_t num_seg_types_y,
                         const t_track_to_pin_lookup& track_to_pin_lookup_x,
                         const t_track_to_pin_lookup& track_to_pin_lookup_y,
                         const t_chan_width& chan_width,
                         const t_chan_details& chan_details_x,
                         const t_chan_details& chan_details_y,
                         t_sb_connection_map* sb_conn_map,
                         const vtr::NdMatrix<std::vector<int>, 3>& switch_block_conn,
                         const vtr::NdMatrix<std::vector<t_bottleneck_link>, 2>& interdie_3d_links,
                         t_sblock_pattern& sblock_pattern,
                         int Fs,
                         int wire_to_ipin_switch,
                         e_directionality directionality,
                         bool is_global_graph,
                         int& num_edges) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const DeviceGrid& grid = device_ctx.grid;

    VTR_ASSERT(Fs % 3 == 0);
    num_edges = 0;

    t_rr_edge_info_set rr_edges_to_create;
    t_rr_edge_info_set interdie_3d_rr_edges_to_create;

    for (size_t i = 0; i < grid.width() - 1; ++i) {
        for (size_t j = 0; j < grid.height() - 1; ++j) {

            // In multi-die FPGAs with track-to-track connections between layers, we need to load CHANZ nodes
            // These extra nodes can be driven from many tracks in the source layer and can drive multiple tracks in the destination layer,
            // since these die-crossing connections have more delays.
            if (grid.get_num_layers() > 1) {
                build_inter_die_3d_rr_chan(rr_graph_builder, i, j, interdie_3d_links[i][j],
                                           CHANX_COST_INDEX_START + num_seg_types_x + num_seg_types_y);
            }

            for (int layer = 0; layer < (int)grid.get_num_layers(); ++layer) {

                // Skip the current die if architecture file specifies that it doesn't require inter-cluster programmable resource routing
                if (!device_ctx.inter_cluster_prog_routing_resources.at(layer)) {
                    continue;
                }

                if (i > 0) {
                    int tracks_per_chan = ((is_global_graph) ? 1 : chan_width.x_list[j]);
                    build_rr_chan(rr_graph_builder, layer, i, j, e_rr_type::CHANX, track_to_pin_lookup_x, sb_conn_map,
                                  switch_block_conn,
                                  CHANX_COST_INDEX_START,
                                  chan_width, tracks_per_chan,
                                  sblock_pattern, Fs / 3, chan_details_x, chan_details_y,
                                  rr_edges_to_create,
                                  wire_to_ipin_switch,
                                  directionality);

                    // Create the actual CHAN->CHAN edges
                    uniquify_edges(rr_edges_to_create);
                    rr_graph_builder.alloc_and_load_edges(&rr_edges_to_create);
                    num_edges += rr_edges_to_create.size();

                    rr_edges_to_create.clear();
                }
                if (j > 0) {
                    int tracks_per_chan = ((is_global_graph) ? 1 : chan_width.y_list[i]);
                    build_rr_chan(rr_graph_builder, layer, i, j, e_rr_type::CHANY, track_to_pin_lookup_y, sb_conn_map,
                                  switch_block_conn,
                                  CHANX_COST_INDEX_START + num_seg_types_x,
                                  chan_width, tracks_per_chan,
                                  sblock_pattern, Fs / 3, chan_details_x, chan_details_y,
                                  rr_edges_to_create,
                                  wire_to_ipin_switch,
                                  directionality);

                    // Create the actual CHAN->CHAN edges
                    uniquify_edges(rr_edges_to_create);
                    rr_graph_builder.alloc_and_load_edges(&rr_edges_to_create);
                    num_edges += rr_edges_to_create.size();
                    rr_edges_to_create.clear();
                }
            }

            if (grid.get_num_layers() > 1) {
                add_inter_die_3d_edges(rr_graph_builder, i, j,
                                       chan_details_x, chan_details_y,
                                       interdie_3d_links[i][j], interdie_3d_rr_edges_to_create);
                uniquify_edges(interdie_3d_rr_edges_to_create);
                rr_graph_builder.alloc_and_load_edges(&interdie_3d_rr_edges_to_create);
                num_edges += interdie_3d_rr_edges_to_create.size();
                interdie_3d_rr_edges_to_create.clear();
            }
        }
    }
}
