
#pragma once

#include <vector>
#include "rr_graph_type.h"

class RRGraphBuilder;
class RRGraphView;
class DeviceGrid;
struct t_physical_tile_type;

void build_intra_cluster_rr_graph(e_graph_type graph_type,
                                  const DeviceGrid& grid,
                                  const std::vector<t_physical_tile_type>& types,
                                  const RRGraphView& rr_graph,
                                  const int delayless_switch,
                                  float R_minW_nmos,
                                  float R_minW_pmos,
                                  RRGraphBuilder& rr_graph_builder,
                                  bool is_flat,
                                  bool load_rr_graph);
