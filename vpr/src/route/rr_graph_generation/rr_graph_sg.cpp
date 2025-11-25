
#include "rr_graph_sg.h"

#include "rr_graph_builder.h"
#include "rr_rc_data.h"
#include "build_scatter_gathers.h"
#include "globals.h"
#include "get_parallel_segs.h"

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
        const NodeRCIndex rc_index = find_create_rr_rc_data(link.R_metal, link.C_metal, mutable_device_ctx.rr_rc_data);
        rr_graph_builder.set_node_rc_index(node, rc_index);

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

void add_edges_opin_chanz_per_side(const RRGraphView& rr_graph,
                                   int layer,
                                   int x,
                                   int y,
                                   e_side side,
                                   const std::vector<vtr::Matrix<int>>& Fc_out,
                                   const t_unified_to_parallel_seg_index& seg_index_map,
                                   int num_seg_types,
                                   vtr::NdMatrix<int, 3>& Fc_zofs,
                                   t_rr_edge_info_set& rr_edges_to_create,
                                   const vtr::NdMatrix<std::vector<t_bottleneck_link>, 2>& interdie_3d_links) {
    const RRSpatialLookup& node_lookup = rr_graph.node_lookup();
    const DeviceGrid& grid = g_vpr_ctx.device().grid;

    t_physical_tile_type_ptr type = grid.get_physical_type({x, y, layer});

    std::vector<RRNodeId> opin_nodes = node_lookup.find_pin_nodes_at_side(layer, x, y, e_rr_type::OPIN, side);

    // Two switch-block location adjacent to this channel segment
    std::array<t_physical_tile_loc, 2> adjacent_sb_loc;

    switch (side) {
        case TOP:
            adjacent_sb_loc[0] = {x, y, layer};
            adjacent_sb_loc[1] = {x - 1, y, layer};
            break;

        case BOTTOM:
            adjacent_sb_loc[0] = {x, y - 1, layer};
            adjacent_sb_loc[1] = {x - 1, y - 1, layer};
            break;

        case RIGHT:
            adjacent_sb_loc[0] = {x, y, layer};
            adjacent_sb_loc[1] = {x, y - 1, layer};
            break;

        case LEFT:
            adjacent_sb_loc[0] = {x - 1, y, layer};
            adjacent_sb_loc[1] = {x - 1, y - 1, layer};
            break;

        default:
            VTR_ASSERT_SAFE(false);
    }

    const int grid_width = grid.width();
    const int grid_height = grid.height();

    adjacent_sb_loc[0].x = std::clamp(adjacent_sb_loc[0].x, 0, grid_width - 1);
    adjacent_sb_loc[0].y = std::clamp(adjacent_sb_loc[0].y, 0, grid_height - 1);

    adjacent_sb_loc[1].x = std::clamp(adjacent_sb_loc[1].x, 0, grid_width - 1);
    adjacent_sb_loc[1].y = std::clamp(adjacent_sb_loc[1].y, 0, grid_height - 1);

    std::vector<std::pair<RRNodeId, short>> selected_chanz_nodes0;
    std::vector<std::pair<RRNodeId, short>> selected_chanz_nodes1;

    for (int iseg = 0; iseg < num_seg_types; iseg++) {
        int seg_index = get_parallel_seg_index(iseg, seg_index_map, e_parallel_axis::Z_AXIS);
        if (seg_index < 0) {
            continue;
        }

        selected_chanz_nodes0.clear();
        for (size_t track_num = 0; track_num < interdie_3d_links[adjacent_sb_loc[0].x][adjacent_sb_loc[0].y].size(); track_num++) {
            const t_bottleneck_link& bottleneck_link = interdie_3d_links[adjacent_sb_loc[0].x][adjacent_sb_loc[0].y][track_num];
            if (bottleneck_link.parallel_segment_index == seg_index && bottleneck_link.gather_loc.layer_num == layer) {
                RRNodeId node_id = node_lookup.find_node(adjacent_sb_loc[0].layer_num, adjacent_sb_loc[0].x, adjacent_sb_loc[0].y, e_rr_type::CHANZ, track_num);
                selected_chanz_nodes0.emplace_back(node_id, bottleneck_link.arch_wire_switch);
            }
        }

        selected_chanz_nodes1.clear();
        for (size_t track_num = 0; track_num < interdie_3d_links[adjacent_sb_loc[1].x][adjacent_sb_loc[1].y].size(); track_num++) {
            const t_bottleneck_link& bottleneck_link = interdie_3d_links[adjacent_sb_loc[1].x][adjacent_sb_loc[1].y][track_num];
            if (bottleneck_link.parallel_segment_index == seg_index && bottleneck_link.gather_loc.layer_num == layer) {
                RRNodeId node_id = node_lookup.find_node(adjacent_sb_loc[1].layer_num, adjacent_sb_loc[1].x, adjacent_sb_loc[1].y, e_rr_type::CHANZ, track_num);
                selected_chanz_nodes1.emplace_back(node_id, bottleneck_link.arch_wire_switch);
            }
        }

        for (RRNodeId opin_node_id : opin_nodes) {
            int pin_number = rr_graph.node_pin_num(opin_node_id);
            int fc = Fc_out[type->index][pin_number][iseg];

            for (int i = 0; i < fc; i++) {

                RRNodeId chanz_node_id;
                short switch_id;
                if (Fc_zofs[adjacent_sb_loc[0].x][adjacent_sb_loc[0].y][seg_index] < Fc_zofs[adjacent_sb_loc[1].x][adjacent_sb_loc[1].y][seg_index]) {
                    int chanz_idx = Fc_zofs[adjacent_sb_loc[0].x][adjacent_sb_loc[0].y][seg_index];
                    chanz_node_id = selected_chanz_nodes0[chanz_idx % selected_chanz_nodes0.size()].first;
                    switch_id = selected_chanz_nodes0[chanz_idx % selected_chanz_nodes0.size()].second;
                    Fc_zofs[adjacent_sb_loc[0].x][adjacent_sb_loc[0].y][seg_index]++;
                } else {
                    int chanz_idx = Fc_zofs[adjacent_sb_loc[1].x][adjacent_sb_loc[1].y][seg_index];
                    chanz_node_id = selected_chanz_nodes1[chanz_idx % selected_chanz_nodes1.size()].first;
                    switch_id = selected_chanz_nodes1[chanz_idx % selected_chanz_nodes1.size()].second;
                    Fc_zofs[adjacent_sb_loc[1].x][adjacent_sb_loc[1].y][seg_index]++;
                }

                rr_edges_to_create.emplace_back(opin_node_id, chanz_node_id, switch_id, false);
            }
        }
    }
}

void add_edges_opin_chanz_per_block(const RRGraphView& rr_graph,
                                    int layer,
                                    int x,
                                    int y,
                                    const std::vector<vtr::Matrix<int>>& Fc_out,
                                    const t_unified_to_parallel_seg_index& seg_index_map,
                                    int num_seg_types,
                                    t_rr_edge_info_set& rr_edges_to_create,
                                    const std::vector<t_bottleneck_link>& interdie_3d_links) {
    const RRSpatialLookup& node_lookup = rr_graph.node_lookup();
    const DeviceGrid& grid = g_vpr_ctx.device().grid;

    t_physical_tile_type_ptr type = grid.get_physical_type({x, y, layer});

    std::vector<RRNodeId> opin_nodes = node_lookup.find_grid_nodes_at_all_sides(layer, x, y, e_rr_type::OPIN);
    std::ranges::stable_sort(opin_nodes, std::less<>{}, [](RRNodeId id) noexcept { return size_t(id); });
    // Remove adjacent duplicates
    auto [unique_end, _] = std::ranges::unique(opin_nodes);
    opin_nodes.erase(unique_end, opin_nodes.end());

    std::vector<std::pair<RRNodeId, short>> selected_chanz_nodes;

    for (int iseg = 0; iseg < num_seg_types; iseg++) {
        int seg_index = get_parallel_seg_index(iseg, seg_index_map, e_parallel_axis::Z_AXIS);
        if (seg_index < 0) {
            continue;
        }

        selected_chanz_nodes.clear();
        for (size_t track_num = 0; track_num < interdie_3d_links.size(); track_num++) {
            const t_bottleneck_link& bottleneck_link = interdie_3d_links[track_num];
            if (bottleneck_link.parallel_segment_index == seg_index && bottleneck_link.gather_loc.layer_num == layer) {
                RRNodeId node_id = node_lookup.find_node(layer, x, y, e_rr_type::CHANZ, track_num);
                selected_chanz_nodes.emplace_back(node_id, bottleneck_link.arch_wire_switch);
            }
        }

        int chanz_idx = 0;
        for (RRNodeId opin_node_id : opin_nodes) {
            int pin_number = rr_graph.node_pin_num(opin_node_id);
            int fc = Fc_out[type->index][pin_number][iseg];

            for (int i = 0; i < fc; i++) {
                RRNodeId chanz_node_id = selected_chanz_nodes[chanz_idx % selected_chanz_nodes.size()].first;
                short switch_id = selected_chanz_nodes[chanz_idx % selected_chanz_nodes.size()].second;
                chanz_idx++;
                rr_edges_to_create.emplace_back(opin_node_id, chanz_node_id, switch_id, false);
            }
        }
    }
}

void add_and_connect_non_3d_sg_links(RRGraphBuilder& rr_graph_builder,
                                     const std::vector<t_bottleneck_link>& sg_links,
                                     const std::vector<std::pair<RRNodeId, int>>& sg_node_indices,
                                     const t_chan_details& chan_details_x,
                                     const t_chan_details& chan_details_y,
                                     size_t num_seg_types_x,
                                     int& num_edges) {
    // Each SG link should have a corresponding RR node index
    VTR_ASSERT(sg_links.size() == sg_node_indices.size());
    const size_t num_links = sg_links.size();

    t_rr_edge_info_set rr_edges_to_create;
    num_edges = 0;

    for (size_t i = 0; i < num_links; i++) {

        const t_bottleneck_link& link = sg_links[i];

        int xlow, xhigh, ylow, yhigh;
        Direction direction;
        e_rr_type chan_type;
        const t_physical_tile_loc& src_loc = link.gather_loc;
        const t_physical_tile_loc& dst_loc = link.scatter_loc;

        // Step 1: Determine the link’s direction and its spatial span.
        // SG links are confined to one layer (non-3D), but can run in X or Y.
        VTR_ASSERT_SAFE(src_loc.layer_num == dst_loc.layer_num);
        const int layer = src_loc.layer_num;
        compute_non_3d_sg_link_geometry(src_loc, dst_loc, chan_type, xlow, xhigh, ylow, yhigh, direction);

        // Retrieve the node ID and track number allocated earlier
        const RRNodeId node_id = sg_node_indices[i].first;
        const int track_num = sg_node_indices[i].second;

        // Step 2: Assign coordinates
        rr_graph_builder.set_node_layer(node_id, layer, layer);
        rr_graph_builder.set_node_coordinates(node_id, xlow, ylow, xhigh, yhigh);
        rr_graph_builder.set_node_capacity(node_id, 1);

        // Step 3: Set cost index based on segment type and orientation
        const size_t cons_index = link.chan_type == e_rr_type::CHANX ? CHANX_COST_INDEX_START + link.parallel_segment_index
                                                                     : CHANX_COST_INDEX_START + num_seg_types_x + link.parallel_segment_index;
        rr_graph_builder.set_node_cost_index(node_id, RRIndexedDataId(cons_index));

        // Step 4: Assign electrical characteristics
        const NodeRCIndex rc_index = find_create_rr_rc_data(link.R_metal, link.C_metal, g_vpr_ctx.mutable_device().rr_rc_data);
        rr_graph_builder.set_node_rc_index(node_id, rc_index);
        // Step 5: Set node type, track number, and direction
        rr_graph_builder.set_node_type(node_id, link.chan_type);
        rr_graph_builder.set_node_track_num(node_id, track_num);
        rr_graph_builder.set_node_direction(node_id, direction);

        // Step 6: Add incoming edges from gather (fanin) channel wires
        // Each gather wire connects to this SG link node using the SG wire switch.
        for (const t_sg_candidate& gather_wire : link.gather_fanin_connections) {
            const t_physical_tile_loc& chan_loc = gather_wire.chan_loc.location;
            e_rr_type gather_chan_type = gather_wire.chan_loc.chan_type;

            // Locate the source RR node for this gather wire
            RRNodeId gather_node = rr_graph_builder.node_lookup().find_node(chan_loc.layer_num,
                                                                            chan_loc.x,
                                                                            chan_loc.y,
                                                                            gather_chan_type,
                                                                            gather_wire.wire_switchpoint.wire);
            // Record deferred edge creation (gather_node --> sg_node)
            rr_edges_to_create.emplace_back(gather_node, node_id, link.arch_wire_switch, false);
        }

        // Step 7: Add outgoing edges to scatter (fanout) channel wires
        // Each scatter wire connects from this SG link node outward.
        for (const t_sg_candidate& scatter_wire : link.scatter_fanout_connections) {
            const t_physical_tile_loc& chan_loc = scatter_wire.chan_loc.location;
            e_rr_type scatter_chan_type = scatter_wire.chan_loc.chan_type;
            const t_chan_details& chan_details = (scatter_chan_type == e_rr_type::CHANX) ? chan_details_x : chan_details_y;

            // Locate the destination RR node for this scatter wire
            RRNodeId scatter_node = rr_graph_builder.node_lookup().find_node(chan_loc.layer_num,
                                                                             chan_loc.x,
                                                                             chan_loc.y,
                                                                             scatter_chan_type,
                                                                             scatter_wire.wire_switchpoint.wire);
            // Determine which architecture switch this edge should use
            int switch_index = chan_details[chan_loc.x][chan_loc.y][scatter_wire.wire_switchpoint.wire].arch_wire_switch();
            // Record deferred edge creation (sg_node --> scatter_node)
            rr_edges_to_create.emplace_back(node_id, scatter_node, switch_index, false);
        }
    }

    uniquify_edges(rr_edges_to_create);
    rr_graph_builder.alloc_and_load_edges(&rr_edges_to_create);
    num_edges += rr_edges_to_create.size();
}
