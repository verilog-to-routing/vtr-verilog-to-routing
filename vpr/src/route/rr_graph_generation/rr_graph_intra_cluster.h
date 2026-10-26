
#pragma once

#include <optional>
#include <vector>
#include "bus_mux_route_types.h"
#include "rr_graph_fwd.h"
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
 *        graph in DeviceContext::rr_bus_muxes and rr_bus_mux_out_nodes.
 *
 * Must be called once the intra-cluster RR graph is complete and its nodes are
 * in their final order.
 */
void load_rr_bus_muxes(const RRSpatialLookup& node_lookup);

/**
 * @brief Return the bus mux and input set the rr edge from_node -> to_node implements a bit of.
 *
 * nullopt for every other edge. Looks up DeviceContext::rr_bus_mux_out_nodes, so it is cheap
 * for nodes that are not bus-mux output pins and free when the architecture has no bus muxes.
 */
std::optional<t_bus_mux_edge> find_bus_mux_edge(RRNodeId from_node, RRNodeId to_node);
