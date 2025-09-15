#pragma once
/* Defines the function used to load an rr graph written in xml format into vpr*/

#include "rr_graph_type.h"
#include "rr_graph_cost.h"
#include "rr_graph_builder.h"
#include "rr_graph_view.h"
#include "rr_graph_fwd.h"
#include "rr_node.h"
#include "device_grid.h"
#include "physical_types.h"


void load_rr_file(RRGraphBuilder* rr_graph_builder,
                  RRGraphView* rr_graph,
                  const std::vector<t_physical_tile_type>& physical_tile_types,
                  const std::vector<t_segment_inf>& segment_inf,
                  vtr::vector<RRIndexedDataId, t_rr_indexed_data>* rr_indexed_data,
                  std::vector<t_rr_rc_data>* rr_rc_data,
                  const DeviceGrid& grid,
                  const std::vector<t_arch_switch_inf>& arch_switch_inf,
                  e_graph_type graph_type,
                  const t_arch* arch,
                  t_chan_width* chan_width,
                  const e_base_cost_type base_cost_type,
                  RRSwitchId* wire_to_rr_ipin_switch,
                  int* wire_to_rr_ipin_switch_between_dice,
                  const char* read_rr_graph_name,
                  std::string* loaded_rr_graph_filename,
                  bool read_edge_metadata,
                  bool do_check_rr_graph,
                  bool echo_enabled,
                  const char* echo_file_name,
                  bool is_flat);

/**
 * @brief Reads a text file where the intrinsic delay of edges are overridden.
 *
 * @details This function tries to find a switch with the overridden delay. If such a
 *          switch exists, the edge will point to it as its corresponding switch.
 *          Otherwise, a new switch is created so that the edge can point to a valid switch.
 *          The architecture file allows the user to specify nominal switch delays, but delays
 *          may vary for the same switch type across the device. To represent switch delays
 *          more accurately, the user can specify multiple switch types in the architecture file
 *          and restrict each one to a region or a single location. Alternatively, the user can
 *          use this file to override edge delays.
 *
 * @param filename The text file to be ingested by this function.
 * @param rr_graph_builder Used to add switches and override switch IDs for edges.
 * @param rr_graph Provides read only access to RR graph.
 */
void load_rr_edge_delay_overrides(std::string_view filename,
                                  RRGraphBuilder& rr_graph_builder,
                                  const RRGraphView& rr_graph);
