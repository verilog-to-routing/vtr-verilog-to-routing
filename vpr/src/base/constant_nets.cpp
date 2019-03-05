#include "constant_nets.h"

#include "clustered_netlist.h"

#include "vtr_assert.h"



void process_constant_nets(ClusteredNetlist& nlist, e_constant_net_method method) {

    if (method == CONSTANT_NET_GLOBAL) {
        /*
         * Treat constant nets (e.g. gnd/vcc) as globals so they are not routed.
         * Identifying these nets as constants is more robust than the previous
         * approach (exact name match to gnd/vcc).
         *
         * Note that by not routing constant nets we are implicitly assuming that all pins
         * in the FPGA can be tied to gnd/vcc, and hence we do not need to route them.
         */
        for (ClusterNetId net : nlist.nets()) {

            if (nlist.net_is_constant(net)) {
                //Mark net as global, so that it is not routed
                VTR_LOG_WARN( "Treating constant net '%s' as global (will not be routed)\n",
                        nlist.net_name(net).c_str());
                nlist.set_net_is_global(net, true);
            }
        }
    } else {
        VTR_ASSERT(method == CONSTANT_NET_ROUTE);
         /* Treat constants the same as any other net, so they will be routed. Note that this requires
          * they have a valid driver (e.g. constant LUT).
          *
          * TODO: We should ultimately make this architecture driven (e.g. specify which
          *       pins which can be tied to gnd/vcc), and then route from those pins to
          *       deliver any constants to those primitive input pins which can not be directly
          *       tied directly to gnd/vcc.
          */
    }
}
