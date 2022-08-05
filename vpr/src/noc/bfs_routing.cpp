#

#include "bfs_routing.h"


BFSRouting::~BFSRouting(){}

void BFSRouting::route_flow(NocRouterId src_router_id, NocRouterId sink_router_id, std::vector<NocLinkId>& flow_route, const NocStorage& noc_model){
    
    const NocRouter src_router = noc_model.get_single_noc_router(src_router_id);
    const NocRouter sink_router = noc_model.get_single_noc_router(sink_router_id);

    // destroy any previously stored route
    flow_route.clear();
    

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
        corresponding link that was used to reach the router. This
        datastructure stores the link that was used to visit each router in 
        the NoC.
        Once the sink router has been found. This datastructure can be used to
        trace the path back to the source router.
    */
   std::unordered_map<NocRouterId, NocLinkId> router_parent_link;

   std::queue<NocRouterId> routers_to_process;

   // indicates whether the routing algorithm found the destination router. If the destination
   // router was not found then this means no route exists between the source and destination router.
   bool found_sink_router = false; 

   // Start by processing the source router
   routers_to_process.push(src_router_id);
   visited_routers.insert(src_router_id);

   // Explore the NoC from the starting router and try to find a path to the destination router
   // we finish searching when there are no more routers to process or we found the destination router
   while (!routers_to_process.empty()){

        // get the next router to process
        NocRouterId processing_router = routers_to_process.front();
        routers_to_process.pop();

        // get the links leaving the currenly processing router (these represent possible paths to explore in the NoC)
        const std::vector<NocLinkId> outgoing_links = noc_model.get_noc_router_connections(processing_router);
        
        // go through the outgoing links of the current router and process the sink routers connected to these links
        for (auto link : outgoing_links) {

            // get the router connected to the current outgoing link
            NocRouterId connected_router  = noc_model.get_single_noc_link(link).get_sink_router();

            // check if we already explored the router we are visiting
            if (visited_routers.find(connected_router) == visited_routers.end()){
                // if we are here then this is the first time visiting this router //
                // mark this router as visited and add sned the router to be processed
                visited_routers.insert(connected_router);
                routers_to_process.push(connected_router);

                // set the parent link of this router (link used to visit the router) as the current outgoing link
                router_parent_link.insert(std::pair<NocRouterId, NocLinkId>(connected_router, link));

                // check if the router visited is the destination router
                if (connected_router == sink_router_id){
                    // mark that the destination router has been found
                    found_sink_router = true;
                    // no need to process any more routers
                    break;
                }
            }
        }
        
        // If the destination router was found there is no need to keep exploring the NoC
        if (found_sink_router){
            break;
        }
    }

    // check if the above search found a valid path from the source to the destination
    // router.
    if (found_sink_router){
        // a lgeal path was found, so construct it
        generate_route(sink_router_id, flow_route, noc_model, router_parent_link);
    }
    else{
        // a path was not found so throw an error to the user
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "No route could be found from starting router with id:'%d' and the destination router with id:'%d' using the XY-Routing algorithm.",src_router.get_router_user_id(), sink_router.get_router_user_id());
    }

    return;

}

void generate_route(NocRouterId start_router_id, std::vector<NocLinkId>& flow_route, const NocStorage& noc_model, const std::unordered_map<NocRouterId, NocLinkId>& router_parent_link){

    // The intermediate router being visited while tracing the path back from the destination router to the src router in the flow.
    // Initially this is set to the router at the end of the path (start_router)
    NocRouterId curr_intermediate_router = start_router_id;

    // get a reference to the beginning of the vector that stores the detgermined route of the flow
    // need this since each link we find in the path will be added to the beginning of the vector
    auto route_beginning = flow_route.begin();

    // get the parent link of the start router
    std::unordered_map<NocRouterId, NocLinkId>::const_iterator curr_intermediate_router_parent_link = router_parent_link.find(curr_intermediate_router);

    // keeop tracking baackwards from each router in the path until a router doesn't have a parent link (this means we reached the starting router in the path)
    while (curr_intermediate_router_parent_link != router_parent_link.end()){
        // add the parent link to the path. Since we are tracing backwards we need to store the links in fron of the last link.
        flow_route.emplace(route_beginning, curr_intermediate_router_parent_link->second);
        
        // now move to the next intermediate router in the path. This will be the source router of the parent link
        curr_intermediate_router = noc_model.get_single_noc_link(curr_intermediate_router_parent_link->second).get_source_router();
        // now get the parent of the router we moved to
        curr_intermediate_router_parent_link = router_parent_link.find(curr_intermediate_router);
    }   

    return;
    
}