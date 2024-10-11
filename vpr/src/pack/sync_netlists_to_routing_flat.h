#include "netlist.h"

 /********************************************************************
 * Top-level function to synchronize packing results to routing results.
 * Flat routing invalidates the ClusteredNetlist since nets may be routed
 * inside or outside a block and changes virtually all intrablock routing.
 * This function:
 *   - rebuilds ClusteredNetlist
 *   - rebuilds all pb_routes
 *   - rebuilds pb graph <-> atom pin mapping in AtomLookup
 * taking routing results as the source of truth.
 *
 * Note:
 *   - This function SHOULD be run ONLY when routing is finished.
 *   - This function only handles the flat routing results.
 *     See \see sync_netlists_to_routing() for the two-stage case.
 *******************************************************************/
void sync_netlists_to_routing_flat(void);
