#ifndef ANNOTATE_ROUTING_H
#define ANNOTATE_ROUTING_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "vpr_context.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

void annotate_rr_node_nets(const DeviceContext& device_ctx,
                           const ClusteringContext& clustering_ctx,
                           RoutingContext& routing_ctx,
                           const bool& verbose);

#endif
