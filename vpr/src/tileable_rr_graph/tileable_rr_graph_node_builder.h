#ifndef TILEABLE_RR_GRAPH_NODE_BUILDER_H
#define TILEABLE_RR_GRAPH_NODE_BUILDER_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
/* Headers from vtrutil library */
#include "vtr_geometry.h"

/* Headers from readarch library */
#include "physical_types.h"

/* Headers from vpr library */
#include "rr_graph_obj.h"
#include "device_grid.h"
#include "rr_graph_view.h"
#include "rr_graph_builder.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

/* begin namespace openfpga */
namespace openfpga {

void alloc_tileable_rr_graph_nodes(RRGraphBuilder& rr_graph_builder,
                                   vtr::vector<RRNodeId, RRSwitchId>& driver_switches, 
                                   const DeviceGrid& grids,
                                   const vtr::Point<size_t>& chan_width,
                                   const std::vector<t_segment_inf>& segment_infs,
                                   const bool& through_channel);

void create_tileable_rr_graph_nodes(RRGraphView& rr_graph,
                                    RRGraphBuilder& rr_graph_builder,
                                    vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches, 
                                    std::map<RRNodeId, std::vector<size_t>>& rr_node_track_ids,
                                    const DeviceGrid& grids, 
                                    const vtr::Point<size_t>& chan_width, 
                                    const std::vector<t_segment_inf>& segment_infs,
                                    const RRSwitchId& wire_to_ipin_switch,
                                    const RRSwitchId& delayless_switch,
                                    const bool& through_channel);


} /* end namespace openfpga */

#endif
