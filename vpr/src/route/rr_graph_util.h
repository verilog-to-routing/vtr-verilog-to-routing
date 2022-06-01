#ifndef RR_GRAPH_UTIL_H
#define RR_GRAPH_UTIL_H

#include "vpr_types.h"
#include "clustered_netlist_utils.h"

int seg_index_of_cblock(t_rr_type from_rr_type, int to_node);

int seg_index_of_sblock(int from_node, int to_node);

bool is_intra_blk_node(RRNodeId node_id);

int get_net_list_pin_rr_node_num(const AtomLookup& atom_lookup,
                                 const ClusteredPinAtomPinsLookup& cluster_pin_lookup,
                                 const Netlist<>& net_list,
                                 const ClusteredNetlist& cluster_net_list,
                                 const vtr::vector<ClusterNetId, std::vector<int>>& cluster_net_terminals,
                                 const ParentNetId net_id,
                                 int pin_index,
                                 bool is_flat);

std::vector<RRNodeId> get_nodes_on_tile(const RRSpatialLookup& rr_spatial_lookup,
                                        t_rr_type node_type,
                                        int root_x,
                                        int root_y);

bool is_node_on_tile(t_rr_type node_type,
                     int root_x,
                     int root_y,
                     int node_ptc);

// This function generates and returns a vector indexed by RRNodeId
// containing a list of fan-in edges for each node.
vtr::vector<RRNodeId, std::vector<RREdgeId>> get_fan_in_list();

#endif
