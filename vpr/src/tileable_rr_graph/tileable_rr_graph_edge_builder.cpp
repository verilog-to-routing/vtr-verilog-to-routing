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

/* begin namespace openfpga */
namespace openfpga {

/************************************************************************
 * Build the edges for all the SOURCE and SINKs nodes:
 * 1. create edges between SOURCE and OPINs
 ***********************************************************************/
static 
void build_rr_graph_edges_for_source_nodes(RRGraph& rr_graph,
                                           const vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches,
                                           const DeviceGrid& grids) {
  for (const RRNodeId& node : rr_graph.nodes()) {
    /* Bypass all the non OPIN nodes */
    if (OPIN != rr_graph.node_type(node)) {
      continue;
    }
    /* Now, we have an OPIN node, we get the source node index */
    short xlow = rr_graph.node_xlow(node); 
    short ylow = rr_graph.node_ylow(node); 
    short src_node_class_num = get_grid_pin_class_index(grids[xlow][ylow], 
                                                        rr_graph.node_pin_num(node));

    /* Create edges between SOURCE and OPINs */
    const RRNodeId& src_node = rr_graph.find_node(xlow - grids[xlow][ylow].width_offset,
                                                  ylow - grids[xlow][ylow].height_offset,
                                                  SOURCE, src_node_class_num);
    VTR_ASSERT(true == rr_graph.valid_node_id(src_node));

    /* add edges to the src_node */
    rr_graph.create_edge(src_node, node, rr_node_driver_switches[node]);
  }
}

/************************************************************************
 * Build the edges for all the SINKs nodes:
 * 1. create edges between IPINs and SINKs
 ***********************************************************************/
static 
void build_rr_graph_edges_for_sink_nodes(RRGraph& rr_graph,
                                         const vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches,
                                         const DeviceGrid& grids) {
  for (const RRNodeId& node : rr_graph.nodes()) {
    /* Bypass all the non IPIN nodes */
    if (IPIN != rr_graph.node_type(node)) {
      continue;
    }
    /* Now, we have an OPIN node, we get the source node index */
    short xlow = rr_graph.node_xlow(node); 
    short ylow = rr_graph.node_ylow(node); 
    short sink_node_class_num = get_grid_pin_class_index(grids[xlow][ylow], 
                                                         rr_graph.node_pin_num(node));
    /* 1. create edges between IPINs and SINKs */
    const RRNodeId& sink_node = rr_graph.find_node(xlow - grids[xlow][ylow].width_offset,
                                                   ylow - grids[xlow][ylow].height_offset,
                                                   SINK, sink_node_class_num);
    VTR_ASSERT(true == rr_graph.valid_node_id(sink_node));

    /* add edges to connect the IPIN node to SINK nodes */
    rr_graph.create_edge(node, sink_node, rr_node_driver_switches[sink_node]);
  }
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
void build_rr_graph_edges(RRGraph& rr_graph, 
                          const vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches,
                          const DeviceGrid& grids,
                          const vtr::Point<size_t>& device_chan_width, 
                          const std::vector<t_segment_inf>& segment_inf,
                          const std::vector<vtr::Matrix<int>>& Fc_in,
                          const std::vector<vtr::Matrix<int>>& Fc_out,
                          const e_switch_block_type& sb_type, const int& Fs,
                          const e_switch_block_type& sb_subtype, const int& subFs,
                          const bool& wire_opposite_side) {

  /* Create edges for SOURCE and SINK nodes for a tileable rr_graph */
  build_rr_graph_edges_for_source_nodes(rr_graph, rr_node_driver_switches, grids);
  build_rr_graph_edges_for_sink_nodes(rr_graph, rr_node_driver_switches, grids);

  vtr::Point<size_t> gsb_range(grids.width() - 2, grids.height() - 2);

  /* Go Switch Block by Switch Block */
  for (size_t ix = 0; ix <= gsb_range.x(); ++ix) {
    for (size_t iy = 0; iy <= gsb_range.y(); ++iy) { 
      //vpr_printf(TIO_MESSAGE_INFO, "Building edges for GSB[%lu][%lu]\n", ix, iy);

      vtr::Point<size_t> gsb_coord(ix, iy);
      /* Create a GSB object */
      const RRGSB& rr_gsb = build_one_tileable_rr_gsb(grids, rr_graph,
                                                      device_chan_width, segment_inf,
                                                      gsb_coord);

      /* adapt the track_to_ipin_lookup for the GSB nodes */      
      t_track2pin_map track2ipin_map; /* [0..track_gsb_side][0..num_tracks][ipin_indices] */
      track2ipin_map = build_gsb_track_to_ipin_map(rr_graph, rr_gsb, grids, segment_inf, Fc_in);

      /* adapt the opin_to_track_map for the GSB nodes */      
      t_pin2track_map opin2track_map; /* [0..gsb_side][0..num_opin_node][track_indices] */
      opin2track_map = build_gsb_opin_to_track_map(rr_graph, rr_gsb, grids, segment_inf, Fc_out);

      /* adapt the switch_block_conn for the GSB nodes */      
      t_track2track_map sb_conn; /* [0..from_gsb_side][0..chan_width-1][track_indices] */
      sb_conn = build_gsb_track_to_track_map(rr_graph, rr_gsb, 
                                             sb_type, Fs, sb_subtype, subFs, wire_opposite_side, 
                                             segment_inf);

      /* Build edges for a GSB */
      build_edges_for_one_tileable_rr_gsb(rr_graph, rr_gsb,
                                          track2ipin_map, opin2track_map, 
                                          sb_conn, rr_node_driver_switches);
      /* Finish this GSB, go to the next*/
    }
  }
}

/************************************************************************
 * Build direct edges for Grids *
 ***********************************************************************/
void build_rr_graph_direct_connections(RRGraph& rr_graph, 
                                       const DeviceGrid& grids, 
                                       const RRSwitchId& delayless_switch, 
                                       const std::vector<t_direct_inf>& directs, 
                                       const std::vector<t_clb_to_clb_directs>& clb_to_clb_directs) {
  for (size_t ix = 0; ix < grids.width(); ++ix) {
    for (size_t iy = 0; iy < grids.height(); ++iy) { 
      /* Skip EMPTY tiles */
      if (true == is_empty_type(grids[ix][iy].type)) {
        continue;
      }
      /* Skip height > 1 or width > 1 tiles (mostly heterogeneous blocks) */
      if ( (0 < grids[ix][iy].width_offset)
        || (0 < grids[ix][iy].height_offset) ) {
        continue;
      }
      vtr::Point<size_t> from_grid_coordinate(ix, iy);
      build_direct_connections_for_one_gsb(rr_graph,
                                           grids,
                                           from_grid_coordinate,
                                           delayless_switch, 
                                           directs, clb_to_clb_directs);
    }
  }
}
 
} /* end namespace openfpga */
