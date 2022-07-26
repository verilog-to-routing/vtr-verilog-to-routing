#ifndef TILEABLE_RR_GRAPH_GSB_H
#define TILEABLE_RR_GRAPH_GSB_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <vector>

#include "vtr_vector.h"
#include "vtr_geometry.h"

#include "physical_types.h"
#include "device_grid.h"

#include "rr_gsb.h"
#include "rr_graph_obj.h"
#include "clb2clb_directs.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

/* begin namespace openfpga */
namespace openfpga {

/************************************************************************
 * Data stuctures related to the functions 
 ***********************************************************************/
typedef std::vector<std::vector<std::vector<RRNodeId>>> t_track2track_map;
typedef std::vector<std::vector<std::vector<RRNodeId>>> t_track2pin_map;
typedef std::vector<std::vector<std::vector<RRNodeId>>> t_pin2track_map;

/************************************************************************
 * Functions 
 ***********************************************************************/
t_track2track_map build_gsb_track_to_track_map(const RRGraph& rr_graph,
                                               const RRGSB& rr_gsb,
                                               const e_switch_block_type& sb_type, 
                                               const int& Fs,
                                               const e_switch_block_type& sb_subtype, 
                                               const int& subFs,
                                               const bool& wire_opposite_side,
                                               const std::vector<t_segment_inf>& segment_inf);

RRGSB build_one_tileable_rr_gsb(const DeviceGrid& grids, 
                                const RRGraph& rr_graph,
                                const vtr::Point<size_t>& device_chan_width, 
                                const std::vector<t_segment_inf>& segment_inf,
                                const vtr::Point<size_t>& gsb_coordinate);

void build_edges_for_one_tileable_rr_gsb(RRGraph& rr_graph, 
                                         const RRGSB& rr_gsb,
                                         const t_track2pin_map& track2ipin_map,
                                         const t_pin2track_map& opin2track_map,
                                         const t_track2track_map& track2track_map,
                                         const vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches);

t_track2pin_map build_gsb_track_to_ipin_map(const RRGraph& rr_graph,
                                            const RRGSB& rr_gsb, 
                                            const DeviceGrid& grids, 
                                            const std::vector<t_segment_inf>& segment_inf, 
                                            const std::vector<vtr::Matrix<int>>& Fc_in);

t_pin2track_map build_gsb_opin_to_track_map(const RRGraph& rr_graph,
                                            const RRGSB& rr_gsb, 
                                            const DeviceGrid& grids, 
                                            const std::vector<t_segment_inf>& segment_inf, 
                                            const std::vector<vtr::Matrix<int>>& Fc_out);

void build_direct_connections_for_one_gsb(RRGraph& rr_graph,
                                          const DeviceGrid& grids,
                                          const vtr::Point<size_t>& from_grid_coordinate,
                                          const RRSwitchId& delayless_switch, 
                                          const std::vector<t_direct_inf>& directs, 
                                          const std::vector<t_clb_to_clb_directs>& clb_to_clb_directs);

} /* end namespace openfpga */

#endif

