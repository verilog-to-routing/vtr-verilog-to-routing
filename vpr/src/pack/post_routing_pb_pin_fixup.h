#ifndef POST_ROUTING_PB_PIN_FIXUP_H
#define POST_ROUTING_PB_PIN_FIXUP_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "vpr_context.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
void sync_netlists_to_routing(const DeviceContext& device_ctx,
                              AtomContext& atom_ctx,
                              ClusteringContext& clustering_ctx,
                              const PlacementContext& placement_ctx,
                              const RoutingContext& routing_ctx,
                              const bool& verbose);

#endif
