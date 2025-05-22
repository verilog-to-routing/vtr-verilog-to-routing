#ifndef ANNOTATE_ROUTING_H
#define ANNOTATE_ROUTING_H

#include "clustered_netlist_fwd.h"
#include "rr_graph_fwd.h"
#include "vtr_vector.h"

struct AtomContext;
struct ClusteringContext;
struct DeviceContext;

/********************************************************************
 * Create a mapping between each rr_node and its mapped nets
 * based on VPR routing results.
 * - Store the net ids mapped to each routing resource nodes
 * - Mapped nodes should have valid net ids (except SOURCE and SINK nodes)
 * - Unmapped rr_node will use invalid ids
 *******************************************************************/
vtr::vector<RRNodeId, ClusterNetId> annotate_rr_node_nets(const ClusteringContext& cluster_ctx,
                                                          const DeviceContext& device_ctx,
                                                          const AtomContext& atom_ctx,
                                                          const bool& verbose);

#endif
