
#pragma once

#include <vector>
#include "rr_graph_type.h"

class RRGraphBuilder;
class RRGraphView;
class DeviceGrid;
struct t_physical_tile_type;

/**
 * @brief Builds the intra-cluster portion of the RR graph.
 *
 * Creates SOURCE/OPIN, IPIN/SINK, and internal pin-to-pin edges inside each
 * cluster, collapsing pin chains, assigning intra-tile switches, and
 * performing consistency checks on the resulting RR graph.
 */
void build_intra_cluster_rr_graph(e_graph_type graph_type,
                                  const DeviceGrid& grid,
                                  const std::vector<t_physical_tile_type>& types,
                                  const RRGraphView& rr_graph,
                                  int delayless_switch,
                                  float R_minW_nmos,
                                  float R_minW_pmos,
                                  RRGraphBuilder& rr_graph_builder,
                                  bool is_flat,
                                  bool load_rr_graph);
