
#include "PlacerTimingCosts.h"


PlacerTimingCosts::PlacerTimingCosts(const ClusteredNetlist& nlist) {
    auto nets = nlist.nets();

    net_start_indicies_.resize(nets.size());

    // Walk through the netlist to determine how many connections there are.
    size_t iconn = 0;
    for (ClusterNetId net : nets) {
        // The placer always skips 'ignored' nets, so they don't affect timing
        // costs, so we also skip them here
        if (nlist.net_is_ignored(net)) {
            net_start_indicies_[net] = OPEN;
            continue;
        }

        // Save the starting index of the current net's connections.
        // We use a -1 offset, since sinks indexed from [1..num_net_pins-1]
        // (there is no timing cost associated with net drivers)
        net_start_indicies_[net] = iconn - 1;

        // Reserve space for all this net's connections
        iconn += nlist.net_sinks(net).size();
    }

    const size_t num_connections = iconn;

    // Determine how many binary tree levels we need to have a leaf for each connection cost
    size_t ilevel = 0;
    while (num_nodes_in_level(ilevel) < num_connections) {
        ++ilevel;
    }
    num_levels_ = ilevel + 1;

    size_t num_leaves = num_nodes_in_level(ilevel);
    size_t num_nodes_in_previous_level = num_nodes_in_level(ilevel - 1);

    VTR_ASSERT_MSG(num_leaves >= num_connections, "Need at least as many leaves as connections");
    VTR_ASSERT_MSG(num_connections == 0 || num_nodes_in_previous_level < num_connections,
                   "Level before should have fewer nodes than connections (to ensure using the smallest binary tree)");

    // We don't need to store all possible leaves if we have fewer connections (i.e. bottom-right of tree is empty)
    size_t last_level_unused_nodes = num_nodes_in_level(ilevel) - num_connections;
    size_t num_nodes = num_nodes_up_to_level(ilevel) - last_level_unused_nodes;

    // Reserve space for connection costs and intermediate node values
    connection_costs_ = std::vector<double>(num_nodes, std::numeric_limits<double>::quiet_NaN());

    // The net start indices we calculated earlier didn't account for intermediate binary tree nodes
    // Shift the start indices after the intermediate nodes
    size_t num_intermediate_nodes = num_nodes_up_to_level(ilevel - 1);
    for (ClusterNetId net : nets) {
        if (nlist.net_is_ignored(net)) continue;

        net_start_indicies_[net] = net_start_indicies_[net] + num_intermediate_nodes;
    }
}
