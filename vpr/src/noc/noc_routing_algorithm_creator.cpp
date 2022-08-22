
#include "noc_routing_algorithm_creator.h"
#include "vpr_error.h"

NocRouting* NocRoutingAlgorithmCreator::create_routing_algorithm(std::string routing_algorithm_name) {
    NocRouting* noc_routing_algorithm = nullptr;

    if (routing_algorithm_name == "xy_routing") {
        noc_routing_algorithm = new XYRouting();
    } else if (routing_algorithm_name == "bfs_routing") {
        noc_routing_algorithm = new BFSRouting();
    } else {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "The provided NoC routing algorithm '%s' is not supported.", routing_algorithm_name.c_str());
    }

    return noc_routing_algorithm;
}