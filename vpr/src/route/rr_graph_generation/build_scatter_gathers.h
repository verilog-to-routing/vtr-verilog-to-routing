#pragma once

#include "scatter_gather_types.h"
#include "rr_types.h"
#include "rr_graph_type.h"
#include "switchblock_scatter_gather_common_utils.h"

#include <vector>

// forward declaration
namespace vtr {
class RngContainer;
}

/// Identifies a specific channel location in the device grid.
struct t_chan_loc {
    t_physical_tile_loc location; ///< Physical grid location of the channel
    e_rr_type chan_type;          ///< Type of routing channel (e.g., CHANX, CHANY)
    e_side side;                  ///< Side of the reference switch block the channel lies on
};

/// Represents a wire candidate for scatter/gather connections.
struct t_sg_candidate {
    t_chan_loc chan_loc;                 ///< Channel location (coordinates, type, side) where the wire lies.
    t_wire_switchpoint wire_switchpoint; ///< Wire index and its valid switchpoint
};

/// Represents a scatter/gather bottleneck connection between two locations.
/// This data structure is used in RR graph generation to model a node that is
/// driven by wires at a gather location and drives wires at a scatter location.
struct t_bottleneck_link {
    t_physical_tile_loc gather_loc;  ///< Source switchblock location.
    t_physical_tile_loc scatter_loc; ///< Destination switchblock location.
    int arch_wire_switch;            ///< The switch (mux) used to drive the bottleneck wire.
    int parallel_segment_index;
    e_rr_type chan_type;
    std::vector<t_sg_candidate> gather_fanin_connections;   ///< Wires driving the bottleneck link at  `gather_loc`
    std::vector<t_sg_candidate> scatter_fanout_connections; ///< Wires driven by the bottleneck link at `scatter_loc`
};

/**
 * @brief Builds scatter/gather bottleneck connections across the device grid.
 *
 * For each scatter/gather pattern, this routine finds candidate gather and scatter
 * wires, evaluates connection limits from user-specified formulas, and creates
 * bottleneck links between the corresponding locations. Both intra-die and inter-die
 * (3D) connections are handled.
 *
 * @param scatter_gather_patterns List of scatter/gather connection patterns.
 * @param inter_cluster_rr Flags indicating whether each layer has inter-cluster routing resources.
 * @param segment_inf_x, segment_inf_y, segment_inf_z Wire segment type information along each axis.
 * @param chan_details_x Channel details for horizontal routing channels.
 * @param chan_details_y Channel details for vertical routing channels.
 * @param nodes_per_chan Channel width data.
 * @param rng Random number generator used to shuffle wire candidates.
 * @param interdie_3d_links Output: matrix storing inter-die (3D) bottleneck links.
 * @return Vector of non-3d bottleneck links.
 */
std::vector<t_bottleneck_link> alloc_and_load_scatter_gather_connections(const std::vector<t_scatter_gather_pattern>& scatter_gather_patterns,
                                                                         const std::vector<bool>& inter_cluster_rr,
                                                                         const std::vector<t_segment_inf>& segment_inf_x,
                                                                         const std::vector<t_segment_inf>& segment_inf_y,
                                                                         const std::vector<t_segment_inf>& segment_inf_z,
                                                                         const t_chan_details& chan_details_x,
                                                                         const t_chan_details& chan_details_y,
                                                                         const t_chan_width& nodes_per_chan,
                                                                         vtr::RngContainer& rng,
                                                                         vtr::NdMatrix<std::vector<t_bottleneck_link>, 2>& interdie_3d_links);
