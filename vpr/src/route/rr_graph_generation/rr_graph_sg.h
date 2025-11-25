
#pragma once

#include <vector>
#include "rr_types.h"
#include "rr_edge.h"
#include "rr_graph_type.h"

class RRGraphBuilder;
class RRGraphView;
struct t_bottleneck_link;

/**
 * @brief Builds CHANZ nodes to handle 3D switchblock edges in the RR graph.
 *  @param rr_graph_builder RRGraphBuilder data structure which allows data modification on a routing resource graph
 *  @param x_coord switch block x_coordinate
 *  @param y_coord switch block y-coordinate
 *  @param interdie_3d_links 3D wires and their connectivity specification for the given location.
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
                                   const vtr::NdMatrix<std::vector<t_bottleneck_link>, 2>& interdie_3d_links);

void add_edges_opin_chanz_per_block(const RRGraphView& rr_graph,
                                    int layer,
                                    int x,
                                    int y,
                                    const std::vector<vtr::Matrix<int>>& Fc_out,
                                    const t_unified_to_parallel_seg_index& seg_index_map,
                                    int num_seg_types,
                                    t_rr_edge_info_set& rr_edges_to_create,
                                    const std::vector<t_bottleneck_link>& interdie_3d_links);

/**
 * @brief Adds and connects non-3D scatter–gather (SG) links to the RR graph.
 *
 * For each bottleneck link, this function creates a corresponding RR node
 * representing the non-3D SG link, and records edges between the node and
 * gather and scatter wires. The edges are stored in `non_3d_sg_rr_edges_to_create`
 * for deferred creation.
 *
 * @param rr_graph_builder     Reference to the RR graph builder.
 * @param sg_links             List of scatter–gather bottleneck links.
 * @param sg_node_indices      RR node IDs and track numbers for SG links.
 * @param chan_details_x       Channel details for CHANX segments.
 * @param chan_details_y       Channel details for CHANY segments.
 * @param num_seg_types_x      Number of segment types in the X direction.
 * @param num_edges            Total number of edges added to RR graph.
 */
void add_and_connect_non_3d_sg_links(RRGraphBuilder& rr_graph_builder,
                                     const std::vector<t_bottleneck_link>& sg_links,
                                     const std::vector<std::pair<RRNodeId, int>>& sg_node_indices,
                                     const t_chan_details& chan_details_x,
                                     const t_chan_details& chan_details_y,
                                     size_t num_seg_types_x,
                                     int& num_edges);
