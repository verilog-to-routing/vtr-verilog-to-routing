#ifndef ANNOTATE_ROUTING_H
#define ANNOTATE_ROUTING_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "vpr_context.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

vtr::vector<RRNodeId, ParentNetId> annotate_rr_node_nets(const Netlist<>& net_list,
                                                         const DeviceContext& device_ctx,
                                                         const RoutingContext& routing_ctx,
                                                         const bool& verbose,
                                                         bool is_flat);

#endif
