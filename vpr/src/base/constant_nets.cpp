#include "constant_nets.h"

#include "atom_netlist.h"
#include "atom_netlist_utils.h"
#include "clustered_netlist.h"

#include "vtr_assert.h"

void process_constant_nets(AtomNetlist& atom_nlist, const AtomLookup& atom_look_up, ClusteredNetlist& nlist, e_constant_net_method method, int verbosity) {
    if (method == CONSTANT_NET_GLOBAL) {
        /*
         * Mark nets driven by constant generators (e.g. gnd/vcc) as ignored so they are not routed.
         * Identifying these nets as constants is more robust than the previous
         * approach (exact name match to gnd/vcc).
         *
         * Note that by not routing constant nets we are implicitly assuming that all pins
         * in the FPGA can be tied to gnd/vcc, and hence we do not need to route them.
         *
         * Nets which were only inferred to be constant (e.g. the output of a primitive whose
         * inputs are all constant) are still routed, since the primitive driving them is
         * still implemented in the device.
         */
        size_t constant_net_count = 0;
        size_t inferred_constant_net_count = 0;
        for (ClusterNetId net : nlist.nets()) {
            AtomNetId atom_net_id = atom_look_up.atom_net(net);
            VTR_ASSERT(atom_net_id != AtomNetId::INVALID());

            if (is_constant_generator_net(atom_nlist, atom_net_id)) {
                // Mark net as ignored, so that it is not routed
                VTR_LOGV_WARN(verbosity > 2, "Treating constant net '%s' as ignored (will not be routed)\n",
                              nlist.net_name(net).c_str());
                nlist.set_net_is_ignored(net, true);
                atom_nlist.set_net_is_ignored(atom_net_id, true);
                ++constant_net_count;
            } else if (nlist.net_is_constant(net)) {
                VTR_LOGV(verbosity > 2, "Net '%s' was inferred to be constant (will be routed)\n",
                         nlist.net_name(net).c_str());
                ++inferred_constant_net_count;
            }
        }
        VTR_LOG_WARN("Treated %zu constant nets as ignored which will not be routed (to see net names increase packer verbosity).\n", constant_net_count);
        VTR_LOGV(inferred_constant_net_count > 0,
                 "Found %zu nets inferred to be constant which will be routed (to see net names increase packer verbosity).\n",
                 inferred_constant_net_count);
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
