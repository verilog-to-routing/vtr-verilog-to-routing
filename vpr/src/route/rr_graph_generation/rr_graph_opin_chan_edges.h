#pragma once

#include <vector>
#include "rr_types.h"
#include "rr_graph_type.h"
#include "rr_edge.h"

class RRGraphBuilder;
class RRGraphView;
struct t_chan_width;
struct t_bottleneck_link;
struct t_clb_to_clb_directs;

/**
 * @brief Builds all OPIN-->CHAN edges in the RR graph.
 *
 * For each grid tile (all layers and sides), connects OPIN nodes to adjacent
 * CHANX/CHANY wires based on architecture directionality:
 * - **BI_DIRECTIONAL**: uses pin-to-track lookups.
 * - **UNI_DIRECTIONAL**: distributes Fc connections using staggered offsets.
 *
 * Also adds direct CLB-to-CLB pin connections and inter-die (3D) links.
 * Duplicates are removed before committing edges to the RR graph.
 */
void add_opin_chan_edges(RRGraphBuilder& rr_graph_builder,
                         const RRGraphView& rr_graph,
                         size_t num_seg_types,
                         size_t num_seg_types_x,
                         size_t num_seg_types_y,
                         const t_chan_width& chan_width,
                         const t_chan_details& chan_details_x,
                         const t_chan_details& chan_details_y,
                         const t_unified_to_parallel_seg_index& seg_index_map,
                         const t_pin_to_track_lookup& opin_to_track_map,
                         const vtr::NdMatrix<std::vector<t_bottleneck_link>, 2>& interdie_3d_links,
                         const std::vector<vtr::Matrix<int>>& Fc_out,
                         const std::vector<t_direct_inf>& directs,
                         const std::vector<t_clb_to_clb_directs>& clb_to_clb_directs,
                         e_directionality directionality,
                         int& num_edges,
                         int& rr_edges_before_directs,
                         bool* Fc_clipped);
