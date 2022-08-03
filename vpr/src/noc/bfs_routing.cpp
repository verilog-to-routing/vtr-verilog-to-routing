#

#include "bfs_routing.h"


BFSRouting::~BFSRouting(){}

void BFSRouting::route_flow(NocRouterId src_router_id, NocRouterId sink_router_id, std::vector<NocLinkId>& flow_route, const NocStorage& noc_model){
    /**
     * Keeps track of which routers have been reached already
     * while traversing the NoC. This variable will help determine
     * cases where a route could not be found and the algorithm is
     * stuck going back and forth between routers it has already
     * visited.
     * 
     */
    std::unordered_set<NocRouterId> visited_routers;

    /*
        As the routing goes through the NoC, each router visited has a
        correspondinglink that was used to reach the router. This
        datastructure stores the link that was used to visit each router in 
        the NoC.
        Once the destination router is reached, we need a way 
    */
   std::unordered_map<NocRouterId, NocLinkId> router_parent_link;











}