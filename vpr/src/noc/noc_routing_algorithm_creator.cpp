
#include "noc_routing_algorithm_creator.h"
#include "xy_routing.h"
#include "bfs_routing.h"
#include "west_first_routing.h"
#include "north_last_routing.h"
#include "negative_first_routing.h"
#include "odd_even_routing.h"
#include "vpr_error.h"


std::unique_ptr<NocRouting> NocRoutingAlgorithmCreator::create_routing_algorithm(const std::string& routing_algorithm_name) {
    std::unique_ptr<NocRouting> noc_routing_algorithm;

    if (routing_algorithm_name == "xy_routing") {
        noc_routing_algorithm = std::make_unique<XYRouting>();
    } else if (routing_algorithm_name == "bfs_routing") {
        noc_routing_algorithm = std::make_unique<BFSRouting>();
    } else if (routing_algorithm_name == "west_first_routing") {
        noc_routing_algorithm = std::make_unique<WestFirstRouting>();
    } else if (routing_algorithm_name == "north_last_routing") {
        noc_routing_algorithm = std::make_unique<NorthLastRouting>();
    } else if (routing_algorithm_name == "negative_first_routing") {
        noc_routing_algorithm = std::make_unique<NegativeFirstRouting>();
    } else if (routing_algorithm_name == "odd_even_routing") {
        noc_routing_algorithm = std::make_unique<OddEvenRouting>();
    } else {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "The provided NoC routing algorithm '%s' is not supported.", routing_algorithm_name.c_str());
    }

    return noc_routing_algorithm;
}