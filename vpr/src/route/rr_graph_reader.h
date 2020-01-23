/* Defines the function used to load an rr graph written in xml format into vpr*/

#ifndef RR_GRAPH_READER_H
#define RR_GRAPH_READER_H

#include "rr_graph.h"
#include "device_grid.h"

void load_rr_file(const t_graph_type graph_type,
                  const DeviceGrid& grid,
                  const std::vector<t_segment_inf>& segment_inf,
                  const enum e_base_cost_type base_cost_type,
                  int* wire_to_rr_ipin_switch,
                  const char* read_rr_graph_name,
                  bool read_edge_metadata,
                  bool do_check_rr_graph);

#endif /* RR_GRAPH_READER_H */
