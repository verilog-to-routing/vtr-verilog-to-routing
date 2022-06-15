/*
 * The NocStorage defines the embedded NoC in the FPGA device.
 * The NocStorage contains the following information:
 * - A list of all the routers in the NoC (NocRouter objects)
 * - A list of all the links in the NoC (NocLink objects)
 * - A connections list for each router. This is simply a list
 * of outgoing links of each router.   
 * There are also a number of functions defined here that can be used
 * to access the above information or modify them.
 */

#include "noc_storage.h"

// clear all the graph data structures
NocStorage::NocStorage() {
    clear_noc();
}

// getters for the NoC

// get the outgoing links for a router in the NoC
const std::vector<NocLinkId>& NocStorage::get_noc_router_connections(NocRouterId id) const {
    return router_link_list[id];
}

// get the list of all routers in the NoC
const vtr::vector<NocRouterId, NocRouter>& NocStorage::get_noc_routers(void) const {
    return router_storage;
}

// get the list of all links in the NoC
const vtr::vector<NocLinkId, NocLink>& NocStorage::get_noc_links(void) const {
    return link_storage;
}

// get router properties
const NocRouter& NocStorage::get_noc_router(NocRouterId id) const {
    return router_storage[id];
}

NocRouter& NocStorage::get_mutable_noc_router(NocRouterId id) {
    return router_storage[id];
}


/*int NocStorage::get_noc_router_grid_position_x(NocRouterId id) const {
    return router_storage[id].get_router_grid_position_x();
}

int NocStorage::get_noc_router_grid_position_y(NocRouterId id) const {
    return router_storage[id].get_router_grid_position_y();
}

int NocStorage::get_noc_router_id(NocRouterId id) const {
    return router_storage[id].get_router_id();
}

std::string NocStorage::get_noc_router_design_module_ref(NocRouterId id) const {
    return router_storage[id].get_router_design_module_ref();
}*/

// get link properties
const NocLink& NocStorage::get_noc_link(NocLinkId id) const {
    link_storage[id];
}

NocLink& NocStorage::get_mutable_noc_link(NocLinkId id) {
    return link_storage[id];
}

/*NocRouterId NocStorage::get_noc_link_source_router(NocLinkId id) const {
    return link_storage[id].get_source_router();
}

NocRouterId NocStorage::get_noc_link_sink_router(NocLinkId id) const {
    return link_storage[id].get_sink_router();
}

double NocStorage::get_noc_link_bandwidth_usage(NocLinkId id) const {
    return link_storage[id].get_bandwidth_usage();
}

int NocStorage::get_noc_link_number_of_connections(NocLinkId id) const {
    return link_storage[id].get_number_of_connections();
}*/

void NocStorage::add_router(int id, int grid_position_x, int grid_posistion_y) {
    VTR_ASSERT_MSG(!built_noc, "NoC already built, cannot modify further.");

    router_storage.emplace_back(id, grid_position_x, grid_posistion_y);

    /* Get the corresponding NocRouterId for the newly added router and
     * add it to the conversion table.
     * Since the router is added at the end of the list, the id is equivalent to the last element index.
     * We build the conversion table here as it gurantees only unique routers
     * in the NoC are added.
     */
    NocRouterId converted_id((int)(router_storage.size() - 1));
    router_id_conversion_table.emplace(id, converted_id);

    return;
}

void NocStorage::add_link(NocRouterId source, NocRouterId sink) {
    VTR_ASSERT_MSG(!built_noc, "NoC already built, cannot modify further.");
    link_storage.emplace_back(source, sink);

    // the newly added link was added to the back of the list, so we can get the id as the last element in the list
    NocLinkId added_link_id((int)link_storage.size() - 1);
    router_link_list[source].push_back(added_link_id);

    return;
}

/*// set router properties
void NocStorage::set_noc_router_design_module_ref(NocRouterId id, std::string design_module_ref) {
    router_storage[id].set_router_design_module_ref(design_module_ref);

    return;
}

// set link properties
void NocStorage::set_noc_link_bandwidth_usage(NocLinkId id, double bandwidth_usage) {
    link_storage[id].set_bandwidth_usage(bandwidth_usage);

    return;
}

void NocStorage::set_noc_link_number_of_connections(NocLinkId id, int number_of_connections) {
    link_storage[id].set_number_of_connections(number_of_connections);

    return;
}*/

// assert the flag that indicates whether the noc was built or not
// Once this function is called, the NoC cannot be modified further.
// THis is useful as it ensured the NoC is only modified initially when it is built
void NocStorage::finished_building_noc(void) {
    VTR_ASSERT_MSG(!built_noc, "NoC already built, cannot modify further.");
    built_noc = true;

    return;
}

void NocStorage::clear_noc(void) {
    router_storage.clear();
    link_storage.clear();
    router_link_list.clear();

    built_noc = false;

    return;
}

/*
 * The router IDs provided by the user cannot be used to
 * identify the routers inside NocStorage. So, the router ids
 * are converted to a NocRouterId that can be used to quickly identify
 * a router. When given a router id, it can be converted to the corresponding
 * NocRouterId here.
 *
 */
NocRouterId NocStorage::convert_router_id(int id) const {
    std::unordered_map<int, NocRouterId>::const_iterator result = router_id_conversion_table.find(id);

    if (result == router_id_conversion_table.end()) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Cannot convert router with id:%d. The router was not found within the NoC.", id);
    }

    return result->second;
}

void NocStorage::make_room_for_noc_router_link_list() {
    VTR_ASSERT_MSG(!built_noc, "NoC already built, cannot modify further.");
    router_link_list.resize(router_storage.size());
}

/*
 * Two links are considered parallel when the source router of one link
 * is the destination router of the second line. And when the desitnantion
 * router of one link is the source router of the other link. Sometimes
 * it is usful to get the parallel link of a given link. SO, when given
 * a link id, the parallel link id is returned here.
 */
NocLinkId NocStorage::get_parallel_link(NocLinkId current_link) const {
    // get the current source and sink router
    NocRouterId curr_source_router = link_storage[current_link].get_source_router();
    NocRouterId curr_sink_router = link_storage[current_link].get_sink_router();

    // get the link list of the sink router
    const std::vector<NocLinkId>* sink_router_links = &(router_link_list[curr_sink_router]);

    NocLinkId parallel_link = INVALID_LINK_ID;

    // go through the links of the sink router and the link that has the current source router as the sink router of the link is the parallel link we are looking for
    for (auto link = sink_router_links->begin(); link != sink_router_links->end(); link++) {
        if (link_storage[*link].get_sink_router() == curr_source_router) {
            parallel_link = *link;
            break;
        }
    }

    return parallel_link;
}
