
#include "noc_routing_algorithm_creator.h"
#include "vpr_error.h"

std::unique_ptr<NocRouting> NocRoutingAlgorithmCreator::create_routing_algorithm(const std::string& routing_algorithm_name) {
    std::unique_ptr<NocRouting> noc_routing_algorithm;

    if (routing_algorithm_name == "xy_routing") {
        noc_routing_algorithm = std::make_unique<XYRouting>();
    } else if (routing_algorithm_name == "bfs_routing") {
        noc_routing_algorithm = std::make_unique<BFSRouting>();
    } else if (routing_algorithm_name == "west_first_routing") {
        noc_routing_algorithm = std::make_unique<WestFirstRouting>();
        std::cout << "Creating west_first algorithm" << std::endl;
    } else {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "The provided NoC routing algorithm '%s' is not supported.", routing_algorithm_name.c_str());
    }

    return noc_routing_algorithm;
}