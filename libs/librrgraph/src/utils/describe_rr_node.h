#ifndef DESCRIBE_RR_NODE_H
#define DESCRIBE_RR_NODE_H

#include <string>
#include "rr_graph_view.h"
#include "device_grid.h"

//Returns a brief one-line summary of an RR node
std::string describe_rr_node(const RRGraphView& rr_graph,
                             const DeviceGrid& grid,
                             const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                             RRNodeId inode,
                             bool is_flat);

#endif