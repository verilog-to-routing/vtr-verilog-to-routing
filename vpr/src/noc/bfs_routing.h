#ifndef BFS_ROUTING
#define BFS_ROUTING

#include <vector>
#include <unordered_map>

#include "noc_routing.h"


class BFSRouting : public NocRouting {

    public:

        ~BFSRouting() override;

        /**
         * @brief Finds a route that goes from the starting router in a 
         * traffic flow to the destination router. Uses the breadth first search
         * algorithm to determine the route. A route consists of a series
         * of links that should be traversed when travelling between two routers
         * within the NoC.
         * 
         * @param src_router_id The source router of a traffic flow. Identifies 
         * the starting point of the route within the NoC. This is represents
         * a unique identifier of the source router.
         * @param sink_router_id The destination router of a traffic flow.
         * Identifies the ending point of the route within the NoC.This is 
         * represents a unique identifier of the destination router.
         * @param flow_route Stores the path as a series of NoC links found by 
         * a NoC routing algorithm between two routers in a traffic flow.
         * The function will clear any
         * previously stored path and re-insert the new found path between the
         * two routers. 
         * @param noc_model A model of the NoC. This is used to traverse the
         * NoC and find a route between the two routers.
         */
        void route_flow(NocRouterId src_router_id, NocRouterId sink_router_id, std::vector<NocLinkId>& flow_route, const NocStorage& noc_model) override;






};















#endif