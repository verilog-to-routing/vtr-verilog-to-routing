#ifndef POST_ROUTING_PB_PIN_FIXUP_H
#define POST_ROUTING_PB_PIN_FIXUP_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "vpr_context.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

/********************************************************************
 * Top-level function to synchronize a packed netlist to routing results
 * The problem comes from a mismatch between the packing and routing results
 * When there are equivalent input/output for any grids, the router will try
 * to swap the net mapping among these pins so as to achieve best 
 * routing optimization.
 * However, it will cause the packing results out-of-date as the net mapping 
 * of each grid remain untouched once packing is done.
 * This function aims to fix the mess after routing so that the net mapping
 * can be synchronized
 *
 * Note:
 *   - This function SHOULD be run ONLY when routing is finished.
 *   - This function only handles the two-stage routing results.
 *     See \see sync_netlists_to_routing_flat() for the flat routing case.
 *******************************************************************/
void sync_netlists_to_routing(const Netlist<>& net_list,
                              const DeviceContext& device_ctx,
                              AtomContext& atom_ctx,
                              ClusteringContext& clustering_ctx,
                              const PlacementContext& placement_ctx,
                              const bool& verbose);

#endif
