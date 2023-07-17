/* Defines the function used to load an rr graph written in xml format into vpr*/

#ifndef RR_GRAPH_READER_H
#define RR_GRAPH_READER_H

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
                  const t_graph_type graph_type,
                  const t_arch* arch,
                  t_chan_width* chan_width,
                  const enum e_base_cost_type base_cost_type,
                  const int virtual_clock_network_root_idx,
                  int* wire_to_rr_ipin_switch,
                  int* wire_to_rr_ipin_switch_between_dice,
                  const char* read_rr_graph_name,
                  std::string* read_rr_graph_filename,
                  bool read_edge_metadata,
                  bool do_check_rr_graph,
                  bool echo_enabled,
                  const char* echo_file_name,
                  bool is_flat);

#endif /* RR_GRAPH_READER_H */
