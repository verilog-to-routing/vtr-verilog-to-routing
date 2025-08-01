#ifndef TILEABLE_RR_GRAPH_BUILDER_H
#define TILEABLE_RR_GRAPH_BUILDER_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <vector>

#include "physical_types.h"
#include "device_grid.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

void build_tileable_unidir_rr_graph(const std::vector<t_physical_tile_type>& types,
                                    const DeviceGrid& grids,
                                    const t_chan_width& chan_width,
                                    const e_switch_block_type& sb_type,
                                    const int& Fs,
                                    const e_switch_block_type& sb_subtype,
                                    const int& sub_fs,
                                    const std::vector<t_segment_inf>& segment_inf,
                                    const int& delayless_switch,
                                    const int& wire_to_arch_ipin_switch,
                                    const float R_minW_nmos,
                                    const float R_minW_pmos,
                                    const enum e_base_cost_type& base_cost_type,
                                    const std::vector<t_direct_inf>& directs,
                                    int* wire_to_rr_ipin_switch,
                                    const bool& shrink_boundary,
                                    const bool& perimeter_cb,
                                    const bool& through_channel,
                                    const bool& opin2all_sides,
                                    const bool& concat_wire,
                                    const bool& wire_opposite_side,
                                    int* Warnings);

#endif
