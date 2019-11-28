#ifndef CHECK_RR_GRAPH_H
#define CHECK_RR_GRAPH_H
#include "physical_types.h"

void check_rr_graph(const t_graph_type graph_type,
                    const DeviceGrid& grid,
                    const std::vector<t_physical_tile_type>& types);

void check_rr_node(int inode, enum e_route_type route_type, const DeviceContext& device_ctx);

#endif
