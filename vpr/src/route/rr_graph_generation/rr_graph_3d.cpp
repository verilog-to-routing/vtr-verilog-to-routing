
#include "rr_graph_3d.h"

#include "rr_graph_builder.h"
#include "rr_rc_data.h"
#include "build_scatter_gathers.h"
#include "globals.h"

void add_inter_die_3d_edges(RRGraphBuilder& rr_graph_builder,
                            int x_coord,
                            int y_coord,
                            const t_chan_details& chan_details_x,
                            const t_chan_details& chan_details_y,
                            const std::vector<t_bottleneck_link>& interdie_3d_links,
                            t_rr_edge_info_set& interdie_3d_rr_edges_to_create) {

    const RRSpatialLookup& node_lookup = rr_graph_builder.node_lookup();
    const int num_tracks = interdie_3d_links.size();

    for (int track = 0; track < num_tracks; track++) {
        const t_bottleneck_link& link = interdie_3d_links[track];
        VTR_ASSERT_SAFE(x_coord == link.gather_loc.x && y_coord == link.gather_loc.y);
        VTR_ASSERT_SAFE(x_coord == link.scatter_loc.x && y_coord == link.scatter_loc.y);

        RRNodeId chanz_node = node_lookup.find_node(0, x_coord, y_coord, e_rr_type::CHANZ, track);

        for (const t_sg_candidate& gather_wire : link.gather_fanin_connections) {
            const t_physical_tile_loc& chan_loc = gather_wire.chan_loc.location;
            e_rr_type chan_type = gather_wire.chan_loc.chan_type;
            RRNodeId gather_node = node_lookup.find_node(chan_loc.layer_num, chan_loc.x, chan_loc.y, chan_type, gather_wire.wire_switchpoint.wire);

            interdie_3d_rr_edges_to_create.emplace_back(gather_node, chanz_node, link.arch_wire_switch, false);
        }

        for (const t_sg_candidate& scatter_wire : link.scatter_fanout_connections) {
            const t_physical_tile_loc& chan_loc = scatter_wire.chan_loc.location;
            e_rr_type chan_type = scatter_wire.chan_loc.chan_type;
            const t_chan_details& chan_details = (chan_type == e_rr_type::CHANX) ? chan_details_x : chan_details_y;
            RRNodeId scatter_node = node_lookup.find_node(chan_loc.layer_num, chan_loc.x, chan_loc.y, chan_type, scatter_wire.wire_switchpoint.wire);

            int switch_index = chan_details[chan_loc.x][chan_loc.y][scatter_wire.wire_switchpoint.wire].arch_wire_switch();
            interdie_3d_rr_edges_to_create.emplace_back(chanz_node, scatter_node, switch_index, false);
        }
    }
}

void build_inter_die_3d_rr_chan(RRGraphBuilder& rr_graph_builder,
                                int x_coord,
                                int y_coord,
                                const std::vector<t_bottleneck_link>& interdie_3d_links,
                                int const_index_offset) {
    auto& mutable_device_ctx = g_vpr_ctx.mutable_device();

    // 3D connections within the switch blocks use some CHANZ nodes to allow a single 3D connection to be driven
    // by multiple tracks in the source layer, and drives multiple tracks in the destination layer.
    // These nodes have already been added to RRGraph builder, this function will go through all added nodes
    // with specific location (x_coord, y_coord) and sets their attributes.

    // These nodes have the following attributes:
    // 1) type: CHANZ
    // 2) ptc_num: [0:num_of_3d_connections - 1]
    // 3) xhigh=xlow, yhigh=ylow

    // Go through allocated nodes until no nodes are found within the RRGraph builder
    for (size_t track_num = 0; track_num < interdie_3d_links.size(); track_num++) {
        // Try to find a node with the current track_num

        const t_bottleneck_link& link = interdie_3d_links[track_num];
        VTR_ASSERT_SAFE(link.chan_type == e_rr_type::CHANZ);
        const char layer_low = std::min(link.gather_loc.layer_num, link.scatter_loc.layer_num);
        const char layer_high = std::max(link.gather_loc.layer_num, link.scatter_loc.layer_num);

        RRNodeId node = rr_graph_builder.node_lookup().find_node(layer_low, x_coord, y_coord, e_rr_type::CHANZ, track_num);
        for (char layer = layer_low; layer <= layer_high; layer++) {
            VTR_ASSERT(node == rr_graph_builder.node_lookup().find_node(layer, x_coord, y_coord, e_rr_type::CHANZ, track_num));
        }

        // If the track can't be found, it means we have already processed all tracks
        if (!node.is_valid()) {
            VTR_ASSERT(interdie_3d_links.size() == (size_t)track_num);
            break;
        }

        rr_graph_builder.set_node_layer(node, layer_low, layer_high);
        rr_graph_builder.set_node_coordinates(node, x_coord, y_coord, x_coord, y_coord);
        rr_graph_builder.set_node_cost_index(node, RRIndexedDataId(const_index_offset + link.parallel_segment_index));
        rr_graph_builder.set_node_capacity(node, 1); // GLOBAL routing handled elsewhere
        float R = 0;
        float C = 0;
        rr_graph_builder.set_node_rc_index(node, NodeRCIndex(find_create_rr_rc_data(R, C, mutable_device_ctx.rr_rc_data)));

        rr_graph_builder.set_node_type(node, e_rr_type::CHANZ);
        rr_graph_builder.set_node_track_num(node, track_num);
        if (link.scatter_loc.layer_num > link.gather_loc.layer_num) {
            rr_graph_builder.set_node_direction(node, Direction::INC);
        } else {
            VTR_ASSERT_SAFE(link.scatter_loc.layer_num < link.gather_loc.layer_num);
            rr_graph_builder.set_node_direction(node, Direction::DEC);
        }
    }
}
