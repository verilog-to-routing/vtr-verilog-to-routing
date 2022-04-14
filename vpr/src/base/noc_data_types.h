#ifndef NOC_DATA_TYPES_H
#define NOC_DATA_TYPES_H

#include "vtr_strong_id.h"
#include "atom_netlist_fwd.h"

#include <string>

// data types used to index the routers and links within the noc
struct noc_router_id_tag;
struct noc_link_id_tag;

typedef vtr::StrongId<noc_router_id_tag, int> NocRouterId;
typedef vtr::StrongId<noc_link_id_tag, int> NocLinkId;

/*
    Describes a traffic flow within the NoC, which is the communication between two routers. This description includes the following:
        - The module names of the source and destination routers that are communicating with each other. The module name is the name used when instantiating a router module in the HDL design.
        - The atom IDs of the router modules. By storing this, we can quickly lookup the router modules instead of identifying them by using their names.
        - The bandwidth(size) of data being transferred between the two routers.
        - The maximum allowable latency of the data transfer.

    This datastructure will be primarily used during placement to identify which routers inside the NoC(NocStorage) need to be routed to each other.This is important since the router modules can be moved around to different tiles on the FPGA device. Additionaly, once a the routing has been completed, the bandwidht and latency parameters can be used to calculate the cost of the placement of routers.
*/
struct t_noc_traffic_flow {

    // router module names
    std::string source_router_module_name;
    std::string sink_router_module_name;

    // router module atom ids
    AtomBlockId source_router_atom_id;
    AtomBlockId sink_router_atom_id;

    // in bytes/s
    double traffic_flow_bandwidth;

    // in seconds
    double max_traffic_flow_latency;

};


#endif