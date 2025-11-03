/************************************************************************
 *  This file contains functions that are used to build edges
 *  between nodes of a tileable routing resource graph
 ***********************************************************************/
#include <algorithm>

/* Headers from vtrutil library */
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

#include "vpr_utils.h"

#include "rr_graph_builder_utils.h"
#include "tileable_rr_graph_gsb.h"
#include "tileable_rr_graph_edge_builder.h"

#include "crr_generator.h"
#include "crr_edge_builder.h"
#include "xml_handler.h"

/************************************************************************
 * Build the edges for all the SOURCE and SINKs nodes:
 * 1. create edges between SOURCE and OPINs
 ***********************************************************************/
void build_rr_graph_edges_for_source_nodes(const RRGraphView& rr_graph,
                                           RRGraphBuilder& rr_graph_builder,
                                           const vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches,
                                           const DeviceGrid& grids,
                                           const size_t& layer,
                                           size_t& num_edges_to_create) {
    size_t edge_count = 0;
    for (const RRNodeId& node : rr_graph.nodes()) {
        /* Bypass all the non OPIN nodes */
        if (e_rr_type::OPIN != rr_graph.node_type(node)) {
            continue;
        }
        /* Now, we have an OPIN node, we get the source node index */
        short xlow = rr_graph.node_xlow(node);
        short ylow = rr_graph.node_ylow(node);
        short src_node_class_num = get_grid_pin_class_index(grids, layer, xlow, ylow,
                                                            rr_graph.node_pin_num(node));
        /* Create edges between SOURCE and OPINs */
        t_physical_tile_loc tile_loc(xlow, ylow, layer);
        RRNodeId src_node = rr_graph.node_lookup().find_node(layer,
                                                             xlow - grids.get_width_offset(tile_loc),
                                                             ylow - grids.get_height_offset(tile_loc),
                                                             e_rr_type::SOURCE, src_node_class_num);
        VTR_ASSERT(true == rr_graph.valid_node(src_node));

        /* add edges to the src_node */
        rr_graph_builder.create_edge_in_cache(src_node, node, rr_node_driver_switches[node], false);
        edge_count++;
    }
    /* Allocate edges for all the source nodes */
    rr_graph_builder.build_edges(true);
    num_edges_to_create += edge_count;
}

/************************************************************************
 * Build the edges for all the SINKs nodes:
 * 1. create edges between IPINs and SINKs
 ***********************************************************************/
void build_rr_graph_edges_for_sink_nodes(const RRGraphView& rr_graph,
                                         RRGraphBuilder& rr_graph_builder,
                                         const vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches,
                                         const DeviceGrid& grids,
                                         const size_t& layer,
                                         size_t& num_edges_to_create) {
    size_t edge_count = 0;
    for (const RRNodeId& node : rr_graph.nodes()) {
        /* Bypass all the non IPIN nodes */
        if (e_rr_type::IPIN != rr_graph.node_type(node)) {
            continue;
        }
        /* Now, we have an OPIN node, we get the source node index */
        short xlow = rr_graph.node_xlow(node);
        short ylow = rr_graph.node_ylow(node);
        short sink_node_class_num = get_grid_pin_class_index(grids, layer, xlow, ylow,
                                                             rr_graph.node_pin_num(node));
        /* 1. create edges between IPINs and SINKs */
        t_physical_tile_loc tile_loc(xlow, ylow, 0);
        const RRNodeId& sink_node = rr_graph.node_lookup().find_node(layer,
                                                                     xlow - grids.get_width_offset(tile_loc),
                                                                     ylow - grids.get_height_offset(tile_loc),
                                                                     e_rr_type::SINK, sink_node_class_num, TOTAL_2D_SIDES[0]);
        VTR_ASSERT(true == rr_graph.valid_node(sink_node));

        /* add edges to connect the IPIN node to SINK nodes */
        rr_graph_builder.create_edge_in_cache(node, sink_node, rr_node_driver_switches[sink_node], false);
        edge_count++;
    }
    /* Allocate edges for all the source nodes */
    rr_graph_builder.build_edges(true);
    num_edges_to_create += edge_count;
}

/************************************************************************
 * Build the edges of each rr_node tile by tile:
 * We classify rr_nodes into a general switch block (GSB) data structure
 * where we create edges to each rr_nodes in the GSB with respect to
 * Fc_in and Fc_out, switch block patterns
 * For each GSB:
 * 1. create edges between CHANX | CHANY and IPINs (connections inside connection blocks)
 * 2. create edges between OPINs, CHANX and CHANY (connections inside switch blocks)
 * 3. create edges between OPINs and IPINs (direct-connections)
 ***********************************************************************/
void build_rr_graph_edges(const RRGraphView& rr_graph,
                          RRGraphBuilder& rr_graph_builder,
                          vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches,
                          const t_crr_opts& crr_opts,
                          const DeviceGrid& grids,
                          const VibDeviceGrid& vib_grid,
                          const size_t& layer,
                          const vtr::Point<size_t>& device_chan_width,
                          const std::vector<t_segment_inf>& segment_inf,
                          const std::vector<t_segment_inf>& segment_inf_x,
                          const std::vector<t_segment_inf>& segment_inf_y,
                          const std::vector<vtr::Matrix<int>>& Fc_in,
                          const std::vector<vtr::Matrix<int>>& Fc_out,
                          const e_switch_block_type& sb_type,
                          const int& Fs,
                          const e_switch_block_type& sb_subtype,
                          const int& sub_fs,
                          const bool& perimeter_cb,
                          const bool& opin2all_sides,
                          const bool& concat_wire,
                          const bool& wire_opposite_side,
                          const RRSwitchId& delayless_switch) {

    if (!vib_grid.is_empty()) {
        build_rr_graph_vib_edges(rr_graph,
                                 rr_graph_builder,
                                 rr_node_driver_switches,
                                 grids,
                                 vib_grid,
                                 layer,
                                 device_chan_width,
                                 segment_inf,
                                 segment_inf_x,
                                 segment_inf_y,
                                 perimeter_cb,
                                 delayless_switch);
    } else {
        build_rr_graph_regular_edges(rr_graph,
                                     rr_graph_builder,
                                     rr_node_driver_switches,
                                     crr_opts,
                                     grids,
                                     layer,
                                     device_chan_width,
                                     segment_inf,
                                     segment_inf_x,
                                     segment_inf_y,
                                     Fc_in,
                                     Fc_out,
                                     sb_type,
                                     Fs,
                                     sb_subtype,
                                     sub_fs,
                                     perimeter_cb,
                                     opin2all_sides,
                                     concat_wire,
                                     wire_opposite_side);
    }
}

/************************************************************************
 * Build direct edges for Grids *
 ***********************************************************************/
void build_rr_graph_direct_connections(const RRGraphView& rr_graph,
                                       RRGraphBuilder& rr_graph_builder,
                                       const DeviceGrid& grids,
                                       const size_t& layer,
                                       const std::vector<t_direct_inf>& directs,
                                       const std::vector<t_clb_to_clb_directs>& clb_to_clb_directs) {
    for (size_t ix = 0; ix < grids.width(); ++ix) {
        for (size_t iy = 0; iy < grids.height(); ++iy) {
            t_physical_tile_loc tile_loc(ix, iy, layer);
            /* Skip EMPTY tiles */
            if (true == is_empty_type(grids.get_physical_type(tile_loc))) {
                continue;
            }
            /* Skip height > 1 or width > 1 tiles (mostly heterogeneous blocks) */
            if ((0 < grids.get_width_offset(tile_loc))
                || (0 < grids.get_height_offset(tile_loc))) {
                continue;
            }
            vtr::Point<size_t> from_grid_coordinate(ix, iy);
            build_direct_connections_for_one_gsb(rr_graph,
                                                 rr_graph_builder,
                                                 grids, layer,
                                                 from_grid_coordinate,
                                                 directs, clb_to_clb_directs);
        }
    }
}

void build_rr_graph_vib_edges(const RRGraphView& rr_graph,
                              RRGraphBuilder& rr_graph_builder,
                              vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches,
                              const DeviceGrid& grids,
                              const VibDeviceGrid& vib_grid,
                              const size_t& layer,
                              const vtr::Point<size_t>& device_chan_width,
                              const std::vector<t_segment_inf>& segment_inf,
                              const std::vector<t_segment_inf>& segment_inf_x,
                              const std::vector<t_segment_inf>& segment_inf_y,
                              const bool& perimeter_cb,
                              const RRSwitchId& delayless_switch) {
    /* Create map from mux name to index */

    size_t num_edges_to_create = 0;
    /* Create edges for SOURCE and SINK nodes for a tileable rr_graph */
    build_rr_graph_edges_for_source_nodes(rr_graph, rr_graph_builder, rr_node_driver_switches, grids, layer, num_edges_to_create);
    build_rr_graph_edges_for_sink_nodes(rr_graph, rr_graph_builder, rr_node_driver_switches, grids, layer, num_edges_to_create);

    vtr::Point<size_t> gsb_range(grids.width() - 2, grids.height() - 2);

    /* Go Switch Block by Switch Block */
    for (size_t ix = 0; ix <= gsb_range.x(); ++ix) {
        for (size_t iy = 0; iy <= gsb_range.y(); ++iy) {
            //vpr_printf(TIO_MESSAGE_INFO, "Building edges for GSB[%lu][%lu]\n", ix, iy);

            vtr::Point<size_t> gsb_coord(ix, iy);

            /* adapt the bend_conn */
            t_bend_track2track_map sb_bend_conn; /* [0..from_gsb_side][0..chan_width-1][track_indices] */
            sb_bend_conn = build_bend_track_to_track_map(grids, rr_graph_builder, rr_graph,
                                                         segment_inf,
                                                         layer, gsb_coord, delayless_switch, rr_node_driver_switches);

            /* Create a GSB object */
            const RRGSB& rr_gsb = build_one_tileable_rr_gsb(grids, rr_graph,
                                                            device_chan_width, segment_inf_x, segment_inf_y,
                                                            layer, gsb_coord, perimeter_cb);

            t_vib_map vib_map;
            vib_map = build_vib_map(rr_graph, grids, vib_grid, rr_gsb, segment_inf, layer, gsb_coord, gsb_coord);
            build_edges_for_one_tileable_vib(rr_graph_builder, vib_map, sb_bend_conn, rr_node_driver_switches, num_edges_to_create);

            rr_graph_builder.build_edges(true);
        }
    }

    /* Process boundary */

    size_t ix, iy;
    // process top boundary
    iy = gsb_range.y() + 1; // == grids.height() - 1
    for (ix = 0; ix < gsb_range.x() + 1; ++ix) {
        vtr::Point<size_t> actual_coord(ix, iy);
        vtr::Point<size_t> gsb_coord(ix, iy - 1);

        /* Create a GSB object */
        const RRGSB& rr_gsb = build_one_tileable_rr_gsb(grids, rr_graph,
                                                        device_chan_width, segment_inf_x, segment_inf_y,
                                                        layer, gsb_coord, perimeter_cb);

        t_vib_map vib_map;
        vib_map = build_vib_map(rr_graph, grids, vib_grid, rr_gsb, segment_inf, layer, gsb_coord, actual_coord);
        //build_edges_for_one_tileable_vib(rr_graph_builder, vib_map, sb_bend_conn, rr_node_driver_switches, num_edges_to_create);
        size_t edge_count = 0;
        for (auto iter = vib_map.begin(); iter != vib_map.end(); ++iter) {
            for (auto to_node : iter->second) {
                rr_graph_builder.create_edge_in_cache(iter->first, to_node, rr_node_driver_switches[to_node], false);
                edge_count++;
            }
        }
        num_edges_to_create += edge_count;
        //rr_graph_builder.build_edges(true);
    }

    // process right boundary
    ix = gsb_range.x() + 1;
    for (iy = 0; iy < gsb_range.y() + 1; ++iy) {
        vtr::Point<size_t> actual_coord(ix, iy);
        vtr::Point<size_t> gsb_coord(ix - 1, iy);

        /* Create a GSB object */
        const RRGSB& rr_gsb = build_one_tileable_rr_gsb(grids, rr_graph,
                                                        device_chan_width, segment_inf_x, segment_inf_y,
                                                        layer, gsb_coord, perimeter_cb);

        t_vib_map vib_map;
        vib_map = build_vib_map(rr_graph, grids, vib_grid, rr_gsb, segment_inf, layer, gsb_coord, actual_coord);
        //build_edges_for_one_tileable_vib(rr_graph_builder, vib_map, sb_bend_conn, rr_node_driver_switches, num_edges_to_create);
        size_t edge_count = 0;
        for (auto iter = vib_map.begin(); iter != vib_map.end(); ++iter) {
            for (auto to_node : iter->second) {
                rr_graph_builder.create_edge_in_cache(iter->first, to_node, rr_node_driver_switches[to_node], false);
                edge_count++;
            }
        }
        num_edges_to_create += edge_count;
        //rr_graph_builder.build_edges(true);
    }
    rr_graph_builder.build_edges(true);
}

void build_rr_graph_regular_edges(const RRGraphView& rr_graph,
                                  RRGraphBuilder& rr_graph_builder,
                                  const vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches,
                                  const t_crr_opts& crr_opts,
                                  const DeviceGrid& grids,
                                  const size_t& layer,
                                  const vtr::Point<size_t>& device_chan_width,
                                  const std::vector<t_segment_inf>& segment_inf,
                                  const std::vector<t_segment_inf>& segment_inf_x,
                                  const std::vector<t_segment_inf>& segment_inf_y,
                                  const std::vector<vtr::Matrix<int>>& Fc_in,
                                  const std::vector<vtr::Matrix<int>>& Fc_out,
                                  const e_switch_block_type& sb_type,
                                  const int& Fs,
                                  const e_switch_block_type& sb_subtype,
                                  const int& sub_fs,
                                  const bool& perimeter_cb,
                                  const bool& opin2all_sides,
                                  const bool& concat_wire,
                                  const bool& wire_opposite_side) {
    bool build_crr_edges = !crr_opts.sb_templates.empty();
    size_t num_edges_to_create = 0;
    /* Create edges for SOURCE and SINK nodes for a tileable rr_graph */
    build_rr_graph_edges_for_source_nodes(rr_graph, rr_graph_builder, rr_node_driver_switches, grids, layer, num_edges_to_create);
    build_rr_graph_edges_for_sink_nodes(rr_graph, rr_graph_builder, rr_node_driver_switches, grids, layer, num_edges_to_create);

    vtr::Point<size_t> gsb_range(grids.width() - 1, grids.height() - 1);

    // Building CRR Graph
    const crrgenerator::CRRConnectionBuilder* crr_connection_builder = nullptr;
    crrgenerator::SwitchBlockManager sb_manager;
    crrgenerator::NodeLookupManager node_lookup(rr_graph, grids.width(), grids.height());
    crrgenerator::CRRGraphGenerator parser(crr_opts, rr_graph, node_lookup, sb_manager, "vpr_custom_rr_graph.xml");
    if (build_crr_edges) {
        sb_manager.initialize(crr_opts.sb_maps, crr_opts.sb_templates, crr_opts.annotated_rr_graph);
        node_lookup.initialize();
        parser.run();
        crr_connection_builder = parser.get_connection_builder();
    }

    /* Go Switch Block by Switch Block */
    for (size_t ix = 0; ix <= gsb_range.x(); ++ix) {
        for (size_t iy = 0; iy <= gsb_range.y(); ++iy) {
            //vpr_printf(TIO_MESSAGE_INFO, "Building edges for GSB[%lu][%lu]\n", ix, iy);

            vtr::Point<size_t> gsb_coord(ix, iy);
            /* Create a GSB object */
            const RRGSB& rr_gsb = build_one_tileable_rr_gsb(grids, rr_graph,
                                                            device_chan_width, segment_inf_x, segment_inf_y,
                                                            layer, gsb_coord, perimeter_cb);

            t_track2pin_map track2ipin_map; /* [0..track_gsb_side][0..num_tracks][ipin_indices] */
            t_pin2track_map opin2track_map; /* [0..gsb_side][0..num_opin_node][track_indices] */
            if (build_crr_edges && !crr_opts.preserve_pin_connections) {
                VTR_ASSERT_MSG(false, "Not implemented");
            } else {
                /* adapt the track_to_ipin_lookup for the GSB nodes */
                track2ipin_map = build_gsb_track_to_ipin_map(rr_graph, rr_gsb, grids, segment_inf, Fc_in);

                /* adapt the opin_to_track_map for the GSB nodes */
                opin2track_map = build_gsb_opin_to_track_map(rr_graph, rr_gsb, grids, segment_inf, Fc_out, opin2all_sides);
            }

            /* adapt the switch_block_conn for the GSB nodes */
            t_track2track_map sb_conn; /* [0..from_gsb_side][0..chan_width-1][track_indices] */
            if (build_crr_edges) {
                build_crr_gsb_track_to_track_edges(rr_graph_builder, rr_gsb, *crr_connection_builder);
            } else {
                sb_conn = build_gsb_track_to_track_map(rr_graph,
                                                       rr_gsb,
                                                       sb_type,
                                                       Fs,
                                                       sb_subtype,
                                                       sub_fs,
                                                       concat_wire,
                                                       wire_opposite_side,
                                                       segment_inf);
            }

            /* Build edges for a GSB */
            build_edges_for_one_tileable_rr_gsb(rr_graph_builder,
                                                rr_gsb,
                                                track2ipin_map,
                                                opin2track_map,
                                                sb_conn,
                                                rr_node_driver_switches,
                                                num_edges_to_create);

            /* Finish this GSB, go to the next*/
            rr_graph_builder.build_edges(true);
        }
    }
}
