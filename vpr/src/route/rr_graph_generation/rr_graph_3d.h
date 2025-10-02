
#pragma once

#include <vector>
#include "rr_types.h"
#include "rr_edge.h"

class RRGraphBuilder;
struct t_bottleneck_link;

/**
 * @brief builds the extra length-0 CHANX nodes to handle 3D custom switchblocks edges in the RR graph.
 *  @param rr_graph_builder RRGraphBuilder data structure which allows data modification on a routing resource graph
 *  @param x_coord switch block x_coordinate
 *  @param y_coord switch block y-coordinate
 *  @param const_index_offset index to the correct node type for RR node cost initialization
 */
void build_inter_die_3d_rr_chan(RRGraphBuilder& rr_graph_builder,
                                int x_coord,
                                int y_coord,
                                const std::vector<t_bottleneck_link>& interdie_3d_links,
                                int const_index_offset);

void add_inter_die_3d_edges(RRGraphBuilder& rr_graph_builder,
                            int x_coord,
                            int y_coord,
                            const t_chan_details& chan_details_x,
                            const t_chan_details& chan_details_y,
                            const std::vector<t_bottleneck_link>& interdie_3d_links,
                            t_rr_edge_info_set& interdie_3d_rr_edges_to_create);
