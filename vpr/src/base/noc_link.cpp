/*
 * The NocLink defines a connection between two routers in the NoC.
 * The NocLink contains the following information:
 * - The source router and destination router the link connects
 * - The number of seperate traffic flows that use the link. By 
 * flows, this is when a router in the NoC transmits information 
 * to another router in the NoC and the link was used within the 
 * communication path between both routers.
 * - The bandwidth usage of the link. When a link is used within a 
 * traffic flow, each link in the communication path needs to
 * support a predefined bandwidth of the flow. Every time a link
 * is used in a flow, its bandwidth usage increases based on the 
 * bandwidth needed by this link. This is useful to track as it
 * can indicate when a link is bein overused (the bandwidth usage
 * exceeds the links supported capability).   
 * There are also a number of functions defined here that can be used
 * to access the above information or modify them.
 */

#include "noc_link.h"

// constructor
NocLink::NocLink(NocRouterId source, NocRouterId sink)
    : source_router(source)
    , sink_router(sink) {
    // initialize variables
    bandwidth_usage = 0.0;
    number_of_connections = 0;
}

// getters
NocRouterId NocLink::get_source_router(void) const {
    return source_router;
}

NocRouterId NocLink::get_sink_router(void) const {
    return sink_router;
}

double NocLink::get_bandwidth_usage(void) const {
    return bandwidth_usage;
}

int NocLink::get_number_of_connections(void) const {
    return number_of_connections;
}

//setters
void NocLink::set_bandwidth_usage(double new_bandwidth_usage) {
    bandwidth_usage = new_bandwidth_usage;
}

void NocLink::set_number_of_connections(int new_number_of_connections) {
    number_of_connections = new_number_of_connections;
}