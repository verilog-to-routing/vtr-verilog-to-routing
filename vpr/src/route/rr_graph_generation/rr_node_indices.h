#pragma once

#include "rr_graph_builder.h"
#include "rr_graph_view.h"
#include "device_grid.h"
#include "rr_types.h"
#include "rr_graph_type.h"
#include "rr_graph_utils.h"
#include "clustered_netlist_fwd.h"

struct t_bottleneck_link;

/**
 * @brief Allocates and populates data structures for efficient rr_node index lookups.
 *
 * This function sets up the `rr_node_indices` structure, which maps a physical location
 * and type to the index of the first corresponding rr_node.
 *
 * @param rr_graph_builder Reference to the RRGraphBuilder used to construct and populate RR node spatial lookups.
 * @param nodes_per_chan Specifies the maximum number of routing tracks per channel in the x and y directions.
 * @param grid The device grid representing the physical layout of tiles in the FPGA fabric.
 * @param index Pointer to the global RR node index counter; incremented as new RR nodes are assigned.
 * @param chan_details_x Channel details describing segment and track properties for CHANX (horizontal) routing tracks.
 * @param chan_details_y Channel details describing segment and track properties for CHANY (vertical) routing tracks.
 */
void alloc_and_load_rr_node_indices(RRGraphBuilder& rr_graph_builder,
                                    const t_chan_width& nodes_per_chan,
                                    const DeviceGrid& grid,
                                    int* index,
                                    const t_chan_details& chan_details_x,
                                    const t_chan_details& chan_details_y);

/**
 * @brief Allocates extra nodes within the RR graph to support 3D custom switch blocks for multi-die FPGAs
 *
 * @param rr_graph_builder RRGraphBuilder data structure which allows data modification on a routing resource graph
 * @param interdie_3d_links Specifies the 3-d inter-die wires that are to be added at each switch-block location.
 * @param index Pointer to the global RR node index counter; incremented as new RR nodes are assigned.
 */
void alloc_and_load_inter_die_rr_node_indices(RRGraphBuilder& rr_graph_builder,
                                              const vtr::NdMatrix<std::vector<t_bottleneck_link>, 2>& interdie_3d_links,
                                              int* index);

/**
 * @brief Allocates and loads RR node indices for a specific tile.
 *
 * This function assigns RR node IDs to all classes (SOURCE/SINK) and pins (IPIN/OPIN)
 * associated with a given physical tile at a specific grid location and layer.
 * It is primarily used in scenarios where a standalone tile's routing resources
 * need to be initialized independently.
 *
 * @param rr_graph_builder Reference to the RR graph builder with spatial lookup.
 * @param physical_tile Pointer to the physical tile type being processed.
 * @param root_loc Tile's root position in the grid.
 * @param num_rr_nodes Pointer to the global RR node index counter (will be incremented).
 */
void alloc_and_load_tile_rr_node_indices(RRGraphBuilder& rr_graph_builder,
                                         t_physical_tile_type_ptr physical_tile,
                                         const t_physical_tile_loc& root_loc,
                                         int* num_rr_nodes);

void alloc_and_load_intra_cluster_rr_node_indices(RRGraphBuilder& rr_graph_builder,
                                                  const DeviceGrid& grid,
                                                  const vtr::vector<ClusterBlockId, t_cluster_pin_chain>& pin_chains,
                                                  const vtr::vector<ClusterBlockId, std::unordered_set<int>>& pin_chains_num,
                                                  int* index);

/**
 * Validate the node look-up matches all the node-level information
 * in the storage of a routing resource graph
 * This function will check the following aspects:
 * - The type of each node matches its type that is indexed in the node look-up
 * - For bounding box (xlow, ylow, xhigh, yhigh) of each node is indexable in the node look-up
 * - The number of unique indexable nodes in the node look up matches the number of nodes in the storage
 *   This ensures that every node in the storage is indexable and there are no hidden nodes in the look-up
 */
bool verify_rr_node_indices(const DeviceGrid& grid,
                            const RRGraphView& rr_graph,
                            const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                            const t_rr_graph_storage& rr_nodes,
                            bool is_flat);
