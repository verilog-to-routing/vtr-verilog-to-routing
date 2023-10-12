#pragma once

#include "netlist_fwd.h"
#include "route_tree_fwd.h"
#include "vpr_types.h"

/** A net decomposed by routing a connection through the partitioning
 * cutline and dividing the bounding box into two. When routing, the connection
 * router will receive a smaller-than-usual bounding box and will have to
 * filter the existing routing spatially. */
class VirtualNet {
  public:
    /** The net in question. */
    ParentNetId net_id;
    /** Clipped bounding box. This is needed to enable decomposing a net multiple times.
     * Otherwise we would need a history of side types and cutlines to compute the bbox. */
    t_bb clipped_bb;
    /** Times decomposed -- don't decompose vnets too deeply or
     * it disturbs net ordering when it's eventually disabled & creates a runtime bump. */
    int times_decomposed = 0;
};
