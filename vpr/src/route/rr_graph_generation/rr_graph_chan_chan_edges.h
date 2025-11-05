#pragma once

#include "rr_types.h"
#include "build_switchblocks.h"

class RRGraphBuilder;
struct t_chan_width;
struct t_bottleneck_link;

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
                         int& num_edges);
