
#include "noc_storage.h"

NocStorage::NocStorage() {
    clear_noc();
}

// getters for the NoC

const std::vector<NocLinkId>& NocStorage::get_noc_router_connections(NocRouterId id) const {
    return router_link_list[id];
}

const vtr::vector<NocRouterId, NocRouter>& NocStorage::get_noc_routers(void) const {
    return router_storage;
}

const vtr::vector<NocLinkId, NocLink>& NocStorage::get_noc_links(void) const {
    return link_storage;
}

double NocStorage::get_noc_link_bandwidth(void) const {
    return noc_link_bandwidth;
}

double NocStorage::get_noc_link_latency(void) const {
    return noc_link_latency;
}

double NocStorage::get_noc_router_latency(void) const {
    return noc_router_latency;
}

const NocRouter& NocStorage::get_single_noc_router(NocRouterId id) const {
    return router_storage[id];
}

NocRouter& NocStorage::get_single_mutable_noc_router(NocRouterId id) {
    return router_storage[id];
}

// get link properties
const NocLink& NocStorage::get_single_noc_link(NocLinkId id) const {
    return link_storage[id];
}

NocLink& NocStorage::get_single_mutable_noc_link(NocLinkId id) {
    return link_storage[id];
}

// setters for the NoC

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

void NocStorage::set_noc_link_bandwidth(double link_bandwidth) {
    noc_link_bandwidth = link_bandwidth;
    return;
}

void NocStorage::set_noc_link_latency(double link_latency) {
    noc_link_latency = link_latency;
    return;
}

void NocStorage::set_noc_router_latency(double router_latency) {
    noc_router_latency = router_latency;
    return;
}

bool NocStorage::remove_link(NocRouterId src_router_id, NocRouterId sink_router_id) {

    // get all the outgoing links of the provided sourcer router
    std::vector<NocLinkId>* source_router_outgoing_links = &router_link_list[src_router_id];

    // keeps track of the position of each link in the vector of outgoing links for the source router
    int outgoing_link_index = 0;

    // keeps track of whether the link was removed or not. This status variable is used to report externally whether the link was removed or not
    bool link_removed_status = false;

    // go through each outgoing link and compare the sink router
    for (auto outgoing_link = source_router_outgoing_links->begin(); outgoing_link != source_router_outgoing_links->end(); outgoing_link++){

        // compare the sink router of this link and check if it is the link to remove
        if (link_storage[*outgoing_link].get_sink_router() == sink_router_id){
            // found the link we need to remove so we delete it here

            // first get the index of the link (this is just the link id, since the link is also the index)
            int index_of_link_to_remove = (size_t)*outgoing_link;

            // deleting link here
            link_storage.erase(link_storage.begin() + index_of_link_to_remove);

            // removing this link as an outgoing link from the source router
            source_router_outgoing_links->erase(source_router_outgoing_links->begin() + outgoing_link_index);

            // update the status variable to indicate that the link was removed
            link_removed_status = true;

            break;
        }

    }

    // if a link was removed then throw warning message
    if (!link_removed_status){
        VTR_LOG_WARN("No link could be found that has a source router with id: '%d' and sink router with id:'%d'.\n", (size_t)src_router_id, (size_t)sink_router_id);
    }

    return link_removed_status;

}

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

NocRouterId NocStorage::convert_router_id(int id) const {
    std::unordered_map<int, NocRouterId>::const_iterator result = router_id_conversion_table.find(id);

    if (result == router_id_conversion_table.end()) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Cannot convert router with id:%d. The router was not found within the NoC.", id);
    }

    return result->second;
}

void NocStorage::make_room_for_noc_router_link_list(void) {
    VTR_ASSERT_MSG(!built_noc, "NoC already built, cannot modify further.");
    router_link_list.resize(router_storage.size());
}

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

void NocStorage::echo_noc(char* file_name) const {
    FILE* fp;
    fp = vtr::fopen(file_name, "w");

    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "NoC\n");
    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "\n");

    // print the overall constraints of the NoC
    fprintf(fp, "NoC Constraints:\n");
    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "\n");
    fprintf(fp, "Maximum NoC Link Bandwidth: %f\n", noc_link_bandwidth);
    fprintf(fp, "\n");
    fprintf(fp, "NoC Link Latency: %f\n", noc_link_latency);
    fprintf(fp, "\n");
    fprintf(fp, "NoC Router Latency: %f\n", noc_router_latency);
    fprintf(fp, "\n");

    // print all the routers and their properties
    fprintf(fp, "NoC Router List:\n");
    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "\n");

    // go through each router and print its information
    for (auto router = router_storage.begin(); router != router_storage.end(); router++) {
        fprintf(fp, "Router %d:\n", router->get_router_user_id());
        // if the router tile is larger than a single grid, the position represents the bottom left corner of the tile
        fprintf(fp, "Equivalent Physical Tile Grid Position -> (%d,%d)\n", router->get_router_grid_position_x(), router->get_router_grid_position_y());
        fprintf(fp, "Router Connections ->");

        auto& router_connections = this->get_noc_router_connections(this->convert_router_id(router->get_router_user_id()));

        // go through the outgoing links of the current router and print the connecting router
        for (auto link_id = router_connections.begin(); link_id != router_connections.end(); link_id++) {
            const NocRouterId connecting_router_id = get_single_noc_link(*link_id).get_sink_router();

            fprintf(fp, " %d", get_single_noc_router(connecting_router_id).get_router_user_id());
        }

        fprintf(fp, "\n\n");
    }

    fclose(fp);

    return;
}
