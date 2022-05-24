#ifndef VTR_NOCROUTING_H
#define VTR_NOCROUTING_H

/**
 * The NocRouting class description header file.
 *
 * The NocRouting class describes the routing algorithm the NoC is using. This class is an interface that describes a single utility,
 * which is determining a route between two routers in the NoC. All routing algorithms should inherit this class and implement the
 * route function.
 *
 * */

#include <vector>

#include "noc_data_types.h"
#include "noc_traffic_flows.h"
#include "noc_storage.h"

class NocRouting {

    // determine the list of links that need to be traversed to travel from the source router to the sink router
    virtual void route_flow(NocRouterId src_router, NocRouterId sink_router, std::vector<NocLinkId>& route_link_list, const NocStorage& noc_model) = 0;
};

#endif
