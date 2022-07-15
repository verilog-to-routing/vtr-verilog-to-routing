#ifndef RR_GRAPH_UTIL_H
#define RR_GRAPH_UTIL_H

#include "vpr_types.h"
#include "rr_graph_view.h"

int seg_index_of_cblock(t_rr_type from_rr_type, int to_node);

int seg_index_of_sblock(int from_node, int to_node);

bool is_node_on_tile(t_rr_type node_type,
                     int root_x,
                     int root_y,
                     int node_ptc);

int get_rr_node_max_ptc (const RRGraphView& rr_graph_view,
                        RRNodeId node_id,
                        bool is_flat);

RRNodeId get_pin_rr_node_id(const RRSpatialLookup& rr_spatial_lookup,
                            t_physical_tile_type_ptr physical_tile,
                            const int i,
                            const int j,
                            int pin_physical_num);

RRNodeId get_class_rr_node_id(const RRSpatialLookup& rr_spatial_lookup,
                              t_physical_tile_type_ptr physical_tile,
                              const int i,
                              const int j,
                              int class_physical_num);

// This function generates and returns a vector indexed by RRNodeId
// containing a list of fan-in edges for each node.
vtr::vector<RRNodeId, std::vector<RREdgeId>> get_fan_in_list();

#endif
