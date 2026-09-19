
#pragma once

#include <vector>
#include "rr_graph_type.h"

class RRGraphBuilder;
class RRGraphView;
class RRSpatialLookup;
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
                                  bool load_rr_graph,
                                  bool device_model_warnings);

/**
 * @brief Records the bus-based muxes (<mux bus="true">) of the intra-cluster RR
 *        graph in RoutingContext::rr_bus_muxes and rr_bus_mux_out_nodes.
 *
 * Must be called once the intra-cluster RR graph is complete and its nodes are
 * in their final order.
 */
void load_rr_bus_muxes(const RRSpatialLookup& node_lookup);
