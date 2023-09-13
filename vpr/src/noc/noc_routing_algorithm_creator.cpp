
#include "noc_routing_algorithm_creator.h"
#include "vpr_error.h"

std::unique_ptr<NocRouting> NocRoutingAlgorithmCreator::create_routing_algorithm(const std::string& routing_algorithm_name, const NocStorage& noc_model,
                                                                                 const std::optional<std::reference_wrapper<const NocVirtualBlockStorage>>& noc_virtual_blocks) {
    std::unique_ptr<NocRouting> noc_routing_algorithm;

    if (routing_algorithm_name == "xy_routing") {
        noc_routing_algorithm = std::make_unique<XYRouting>(noc_model, noc_virtual_blocks);
    } else if (routing_algorithm_name == "bfs_routing") {
        noc_routing_algorithm = std::make_unique<BFSRouting>(noc_model, noc_virtual_blocks);
    } else if (routing_algorithm_name == "west_first_routing") {
        noc_routing_algorithm = std::make_unique<WestFirstRouting>(noc_model, noc_virtual_blocks);
    } else if (routing_algorithm_name == "north_last_routing") {
        noc_routing_algorithm = std::make_unique<NorthLastRouting>(noc_model, noc_virtual_blocks);
    } else if (routing_algorithm_name == "negative_first_routing") {
        noc_routing_algorithm = std::make_unique<NegativeFirstRouting>(noc_model, noc_virtual_blocks);
    } else if (routing_algorithm_name == "odd_even_routing") {
        noc_routing_algorithm = std::make_unique<OddEvenRouting>(noc_model, noc_virtual_blocks);
    } else {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "The provided NoC routing algorithm '%s' is not supported.", routing_algorithm_name.c_str());
    }

    return noc_routing_algorithm;
}
