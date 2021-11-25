/* Defines the function used to load an rr graph written in xml format into vpr*/

#ifndef RR_GRAPH_FPGA_INTERCHANGE_H
#define RR_GRAPH_FPGA_INTERCHANGE_H

#include "rr_graph.h"
#include "device_grid.h"
#include "DeviceResources.capnp.h"
#include "LogicalNetlist.capnp.h"
#include "capnp/serialize.h"
#include "capnp/serialize-packed.h"

void build_rr_graph_fpga_interchange(const t_graph_type graph_type,
                                     const DeviceGrid& grid,
                                     const std::vector<t_segment_inf>& segment_inf,
                                     const enum e_base_cost_type base_cost_type,
                                     int* wire_to_rr_ipin_switch,
                                     bool do_check_rr_graph);

#endif /* RR_GRAPH_FPGA_INTERCHANGE_H */
