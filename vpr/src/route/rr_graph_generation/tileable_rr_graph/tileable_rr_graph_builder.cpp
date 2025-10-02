/************************************************************************
 *  This file contains a builder for the complex rr_graph data structure
 *  Different from VPR rr_graph builders, this builder aims to create a
 *  highly regular rr_graph, where each Connection Block (CB), Switch
 *  Block (SB) is the same (except for those on the borders). Thus, the
 *  rr_graph is called tileable, which brings significant advantage in
 *  producing large FPGA fabrics.
 ***********************************************************************/
/* Headers from vtrutil library */
#include "vtr_assert.h"
#include "vtr_time.h"
#include "vtr_log.h"
#include "vtr_memory.h"

#include "vpr_error.h"
#include "vpr_utils.h"

#include "rr_graph_switch_utils.h"
#include "check_rr_graph.h"
#include "get_parallel_segs.h"
#include "device_grid_annotation.h"

#include "rr_graph_builder_utils.h"
#include "tileable_chan_details_builder.h"
#include "tileable_rr_graph_node_builder.h"
#include "tileable_rr_graph_edge_builder.h"
#include "tileable_rr_graph_builder.h"

#include "globals.h"

/************************************************************************
 * Main function of this file
 * Builder for a detailed uni-directional tileable rr_graph
 * Global graph is not supported here, the VPR rr_graph generator can be used
 * It follows the procedures to complete the rr_graph generation
 * 1. Assign the segments for each routing channel,
 *    To be specific, for each routing track, we assign a routing segment.
 *    The assignment is subject to users' specifications, such as
 *    a. length of each type of segment
 *    b. frequency of each type of segment.
 *    c. routing channel width
 * 2. Estimate the number of nodes in the rr_graph
 *    This will estimate the number of
 *    a. IPINs, input pins of each grid
 *    b. OPINs, output pins of each grid
 *    c. SOURCE, virtual node which drives OPINs
 *    d. SINK, virtual node which is connected to IPINs
 *    e. CHANX and CHANY, routing segments of each channel
 * 3. Create the connectivity of OPINs
 *    a. Evenly assign connections to OPINs to routing tracks
 *    b. the connection pattern should be same across the fabric
 * 4. Create the connectivity of IPINs
 *    a. Evenly assign connections from routing tracks to IPINs
 *    b. the connection pattern should be same across the fabric
 * 5. Create the switch block patterns,
 *    It is based on the type of switch block, the supported patterns are
 *    a. Disjoint, which connects routing track (i)th from (i)th and (i)th routing segments
 *    b. Universal, which connects routing track (i)th from (i)th and (M-i)th routing segments
 *    c. Wilton, which rotates the connection of Disjoint by 1 track
 * 6. Allocate rr_graph, fill the node information
 *    For each node, fill
 *    a. basic information: coordinate(xlow, xhigh, ylow, yhigh), ptc_num
 *    b. edges (both incoming and outcoming)
 *    c. handle direct-connections
 * 7. Build fast look-up for the rr_graph
 * 8. Allocate external data structures
 *    a. cost_index
 *    b. RC tree
 ***********************************************************************/

void build_tileable_unidir_rr_graph(const std::vector<t_physical_tile_type>& types,
                                    const DeviceGrid& grids,
                                    const t_chan_width& chan_width,
                                    const e_switch_block_type& sb_type,
                                    const int& Fs,
                                    const e_switch_block_type& sb_subtype,
                                    const int& sub_fs,
                                    const std::vector<t_segment_inf>& segment_inf,
                                    const int& delayless_switch,
                                    const int& wire_to_arch_ipin_switch,
                                    const float R_minW_nmos,
                                    const float R_minW_pmos,
                                    const e_base_cost_type& base_cost_type,
                                    const std::vector<t_direct_inf>& directs,
                                    RRSwitchId& wire_to_rr_ipin_switch,
                                    const bool& shrink_boundary,
                                    const bool& perimeter_cb,
                                    const bool& through_channel,
                                    const bool& opin2all_sides,
                                    const bool& concat_wire,
                                    const bool& wire_opposite_side,
                                    int* Warnings) {
    vtr::ScopedStartFinishTimer timer("Build tileable routing resource graph");

    // Reset warning flag
    *Warnings = RR_GRAPH_NO_WARN;

    // Create a matrix of grid
    // Create a vector of channel width, we support X-direction and Y-direction has different W
    vtr::Point<size_t> device_chan_width(chan_width.x_max, chan_width.y_max);

    VTR_LOG("X-direction routing channel width is %lu\n", device_chan_width.x());
    VTR_LOG("Y-direction routing channel width is %lu\n", device_chan_width.y());

    // Get a mutable device ctx so that we have a mutable rr_graph
    DeviceContext& device_ctx = g_vpr_ctx.mutable_device();

    // Set the tileable flag to true
    device_ctx.rr_graph_builder.set_tileable(true);

    // Annotate the device grid on the boundry
    DeviceGridAnnotation device_grid_annotation(device_ctx.grid, perimeter_cb);

    // The number of segments are in general small, reserve segments may not bring
    // significant memory efficiency
    device_ctx.rr_graph_builder.reserve_segments(segment_inf.size());
    // Create the segments
    for (size_t iseg = 0; iseg < segment_inf.size(); ++iseg) {
        device_ctx.rr_graph_builder.add_rr_segment(segment_inf[iseg]);
    }

    // TODO: Load architecture switch to rr_graph switches
    // Draft the switches as internal data of RRGraph object
    // These are temporary switches copied from arch switches
    // We use them to build the edges
    // We will reset all the switches in the function
    //   alloc_and_load_rr_switch_inf()
    // TODO: Spot the switch id in the architecture switch list
    RRSwitchId wire_to_ipin_rr_switch = RRSwitchId::INVALID();
    RRSwitchId delayless_rr_switch = RRSwitchId::INVALID();

    device_ctx.rr_graph_builder.reserve_switches(device_ctx.arch_switch_inf.size());
    // Create the switches
    for (size_t iswitch = 0; iswitch < device_ctx.arch_switch_inf.size(); ++iswitch) {
        const t_rr_switch_inf& temp_rr_switch = create_rr_switch_from_arch_switch(device_ctx.arch_switch_inf[iswitch], R_minW_nmos, R_minW_pmos);
        RRSwitchId rr_switch = device_ctx.rr_graph_builder.add_rr_switch(temp_rr_switch);
        if ((int)iswitch == wire_to_arch_ipin_switch) {
            wire_to_ipin_rr_switch = rr_switch;
        }
        if ((int)iswitch == delayless_switch) {
            delayless_rr_switch = rr_switch;
        }
    }

    // Validate the special switches
    VTR_ASSERT(true == device_ctx.rr_graph.valid_switch(wire_to_ipin_rr_switch));
    VTR_ASSERT(true == device_ctx.rr_graph.valid_switch(delayless_rr_switch));

    // A temp data about the driver switch ids for each rr_node
    vtr::vector<RRNodeId, RRSwitchId> rr_node_driver_switches;

    // A temp data about the track ids for each CHANX and CHANY rr_node
    std::map<RRNodeId, std::vector<size_t>> rr_node_track_ids;

    // Get the routing segments on X-axis and Y-axis separately
    t_unified_to_parallel_seg_index segment_index_map;
    std::vector<t_segment_inf> segment_inf_x = get_parallel_segs(segment_inf, segment_index_map, e_parallel_axis::X_AXIS, true);
    std::vector<t_segment_inf> segment_inf_y = get_parallel_segs(segment_inf, segment_index_map, e_parallel_axis::Y_AXIS, true);
    std::vector<t_segment_inf> segment_inf_z = get_parallel_segs(segment_inf, segment_index_map, e_parallel_axis::Z_AXIS, true);

    // Get vib grid
    const auto& vib_grid = device_ctx.vib_grid;

    // Allocate the rr_nodes
    alloc_tileable_rr_graph_nodes(device_ctx.rr_graph_builder,
                                  rr_node_driver_switches,
                                  grids, vib_grid, 0,
                                  device_chan_width,
                                  segment_inf_x, segment_inf_y,
                                  device_grid_annotation,
                                  shrink_boundary,
                                  perimeter_cb,
                                  through_channel);

    // Create all the rr_nodes
    create_tileable_rr_graph_nodes(device_ctx.rr_graph,
                                   device_ctx.rr_graph_builder,
                                   rr_node_driver_switches,
                                   rr_node_track_ids,
                                   device_ctx.rr_rc_data,
                                   grids, vib_grid, 0,
                                   device_chan_width,
                                   segment_inf_x, segment_inf_y,
                                   segment_index_map,
                                   wire_to_ipin_rr_switch,
                                   delayless_rr_switch,
                                   device_grid_annotation,
                                   shrink_boundary,
                                   perimeter_cb,
                                   through_channel);

    // Create the connectivity of OPINs
    //   a. Evenly assign connections to OPINs to routing tracks
    //   b. the connection pattern should be same across the fabric
    // Create the connectivity of IPINs
    //   a. Evenly assign connections from routing tracks to IPINs
    //   b. the connection pattern should be same across the fabric
    // Global routing uses a single longwire track
    int max_chan_width = find_unidir_routing_channel_width(chan_width.max);
    VTR_ASSERT(max_chan_width > 0);

    // get maximum number of pins across all blocks
    int max_pins = types[0].num_pins;
    for (const auto& type : types) {
        if (is_empty_type(&type)) {
            continue;
        }

        if (type.num_pins > max_pins) {
            max_pins = type.num_pins;
        }
    }

    // Fc assignment still uses the old function from VPR.
    // Should use tileable version so that we have can have full control
    std::vector<size_t> num_tracks = get_num_tracks_per_seg_type(max_chan_width / 2, segment_inf, false);
    std::vector<int> sets_per_seg_type(segment_inf.size());
    VTR_ASSERT(num_tracks.size() == segment_inf.size());
    for (size_t iseg = 0; iseg < num_tracks.size(); ++iseg) {
        sets_per_seg_type[iseg] = num_tracks[iseg];
    }

    bool Fc_clipped = false;
    // [0..num_types-1][0..num_pins-1]
    std::vector<vtr::Matrix<int>> Fc_in;
    Fc_in = alloc_and_load_actual_fc(types, max_pins, segment_inf, sets_per_seg_type, (const t_chan_width*)&chan_width,
                                     e_fc_type::IN, UNI_DIRECTIONAL, &Fc_clipped, false);
    if (Fc_clipped) {
        *Warnings |= RR_GRAPH_WARN_FC_CLIPPED;
    }

    Fc_clipped = false;
    // [0..num_types-1][0..num_pins-1]
    std::vector<vtr::Matrix<int>> Fc_out;
    Fc_out = alloc_and_load_actual_fc(types, max_pins, segment_inf, sets_per_seg_type, (const t_chan_width*)&chan_width,
                                      e_fc_type::OUT, UNI_DIRECTIONAL, &Fc_clipped, false);

    if (Fc_clipped) {
        *Warnings |= RR_GRAPH_WARN_FC_CLIPPED;
    }

    // Build the connections tile by tile:
    // We classify rr_nodes into a general switch block (GSB) data structure
    // where we create edges to each rr_nodes in the GSB with respect to
    // Fc_in and Fc_out, switch block patterns
    // In addition, we will also handle direct-connections:
    // Add edges that bridge OPINs and IPINs to the rr_graph
    // Create edges for a tileable rr_graph
    build_rr_graph_edges(device_ctx.rr_graph,
                         device_ctx.rr_graph_builder,
                         rr_node_driver_switches,
                         grids, vib_grid, 0,
                         device_chan_width,
                         segment_inf, segment_inf_x, segment_inf_y,
                         Fc_in, Fc_out,
                         sb_type, Fs, sb_subtype, sub_fs,
                         perimeter_cb,
                         opin2all_sides, concat_wire,
                         wire_opposite_side,
                         delayless_rr_switch);

    // Build direction connection lists
    // TODO: use tile direct builder
    // Create data structure of direct-connections
    std::vector<t_clb_to_clb_directs> clb_to_clb_directs = alloc_and_load_clb_to_clb_directs(directs, delayless_switch);
    std::vector<t_clb_to_clb_directs> clb2clb_directs;
    for (size_t idirect = 0; idirect < directs.size(); ++idirect) {
        // Sanity checks on rr switch id
        VTR_ASSERT(device_ctx.rr_graph.valid_switch(RRSwitchId(clb_to_clb_directs[idirect].switch_index)));
        clb2clb_directs.push_back(clb_to_clb_directs[idirect]);
    }

    build_rr_graph_direct_connections(device_ctx.rr_graph, device_ctx.rr_graph_builder, device_ctx.grid, 0,
                                      directs, clb2clb_directs);

    // Allocate and load routing resource switches, which are derived from the switches from the architecture file,
    // based on their fanin in the rr graph. This routine also adjusts the rr nodes to point to these new rr switches
    device_ctx.rr_graph_builder.init_fan_in();
    alloc_and_load_rr_switch_inf(device_ctx.rr_graph_builder, device_ctx.switch_fanin_remap, device_ctx.all_sw_inf, R_minW_nmos, R_minW_pmos, wire_to_arch_ipin_switch, wire_to_rr_ipin_switch);

    // Save the channel widths for the newly constructed graph
    device_ctx.chan_width = chan_width;

    // Save the track ids for tileable routing resource graph
    device_ctx.rr_node_track_ids = rr_node_track_ids;

    // Build incoming edges
    device_ctx.rr_graph_builder.partition_edges();
    device_ctx.rr_graph_builder.build_in_edges();

    // Allocate external data structures
    //  a. cost_index
    //  b. RC tree
    rr_graph_externals(segment_inf, segment_inf_x, segment_inf_y, segment_inf_z, wire_to_rr_ipin_switch, base_cost_type);

    // Sanitizer for the rr_graph, check connectivities of rr_nodes
    // Essential check for rr_graph, build look-up and
    if (!device_ctx.rr_graph_builder.validate()) {
        // Error out if built-in validator of rr_graph fails
        vpr_throw(VPR_ERROR_ROUTE,
                  __FILE__,
                  __LINE__,
                  "Fundamental errors occurred when validating rr_graph object!\n");
    }

    // No clock network support yet; Does not support flatten rr_graph yet

    check_rr_graph(device_ctx.rr_graph, types, device_ctx.rr_indexed_data, grids, vib_grid, device_ctx.chan_width, e_graph_type::UNIDIR_TILEABLE, false);
}
