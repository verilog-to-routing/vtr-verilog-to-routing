#ifndef CONSTANT_NETS_H
#define CONSTANT_NETS_H

#include "clustered_netlist_fwd.h"

enum e_constant_net_method {
    CONSTANT_NET_GLOBAL, //Treat constant nets as globals (i.e. not routed)
    CONSTANT_NET_ROUTE   //Treat constant nets a non-globals (i.e. routed)
};

void process_constant_nets(ClusteredNetlist& nlist, e_constant_net_method method, int verbosity);

#endif
