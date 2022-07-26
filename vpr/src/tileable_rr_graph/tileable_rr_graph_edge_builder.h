#ifndef TILEABLE_RR_GRAPH_EDGE_BUILDER_H
#define TILEABLE_RR_GRAPH_EDGE_BUILDER_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <vector>

/* Headers from vtrutil library */
#include "vtr_ndmatrix.h"
#include "vtr_geometry.h"

#include "physical_types.h"
#include "device_grid.h"
#include "rr_graph_obj.h"
#include "clb2clb_directs.h"
#include "rr_graph_view.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

/* begin namespace openfpga */
namespace openfpga {

void build_rr_graph_edges(RRGraphView& rr_graph,
                          const vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches,
                          const DeviceGrid& grids,
                          const vtr::Point<size_t>& device_chan_width,
                          const std::vector<t_segment_inf>& segment_inf,
                          const std::vector<vtr::Matrix<int>>& Fc_in,
                          const std::vector<vtr::Matrix<int>>& Fc_out,
                          const e_switch_block_type& sb_type, const int& Fs,
                          const e_switch_block_type& sb_subtype, const int& subFs,
                          const bool& wire_opposite_side);

void build_rr_graph_direct_connections(RRGraphView& rr_graph,
                                       const DeviceGrid& grids,
                                       const RRSwitchId& delayless_switch,
                                       const std::vector<t_direct_inf>& directs,
                                       const std::vector<t_clb_to_clb_directs>& clb_to_clb_directs);

} /* end namespace openfpga */

#endif
