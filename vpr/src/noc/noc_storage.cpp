
#include "noc_storage.h"
#include "vtr_assert.h"
#include "vpr_error.h"


#include <algorithm>

NocStorage::NocStorage() {
    clear_noc();
}

// getters for the NoC
const std::vector<NocLinkId>& NocStorage::get_noc_router_outgoing_links(NocRouterId id) const {
    return router_outgoing_links_list[id];
}

const std::vector<NocLinkId>& NocStorage::get_noc_router_incoming_links(NocRouterId id) const {
    return router_incoming_links_list[id];
}

const vtr::vector<NocRouterId, NocRouter>& NocStorage::get_noc_routers() const {
    return router_storage;
}

int NocStorage::get_number_of_noc_routers() const {
    return router_storage.size();
}

const vtr::vector<NocLinkId, NocLink>& NocStorage::get_noc_links() const {
    return link_storage;
}

vtr::vector<NocLinkId, NocLink>& NocStorage::get_mutable_noc_links() {
    return link_storage;
}

int NocStorage::get_number_of_noc_links() const {
    return link_storage.size();
}

double NocStorage::get_noc_link_latency() const {
    return noc_link_latency;
}

double NocStorage::get_noc_router_latency() const {
    return noc_router_latency;
}

bool NocStorage::get_detailed_router_latency() const {
    return detailed_router_latency_;
}

bool NocStorage::get_detailed_link_latency() const {
    return detailed_link_latency_;
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

NocLinkId NocStorage::get_single_noc_link_id(NocRouterId src_router, NocRouterId dst_router) const {
    NocLinkId link_id = NocLinkId::INVALID();

    for (const auto& link : link_storage) {
        if (link.get_source_router() == src_router && link.get_sink_router() == dst_router) {
            link_id = link.get_link_id();
            break;
        }
    }

    return link_id;
}

NocLink& NocStorage::get_single_mutable_noc_link(NocLinkId id) {
    return link_storage[id];
}

NocRouterId NocStorage::get_router_at_grid_location(const t_pl_loc& hard_router_location) const {
    // get the key to identify the corresponding hard router block at the provided grid location
    int router_key = generate_router_key_from_grid_location(hard_router_location.x,
                                                            hard_router_location.y,
                                                            hard_router_location.layer);

    // get the hard router block id at the given grid location
    auto hard_router_block = grid_location_to_router_id.find(router_key);
    // verify whether a router hard block exists at this location
    VTR_ASSERT(hard_router_block != grid_location_to_router_id.end());

    return hard_router_block->second;
}

// setters for the NoC

void NocStorage::add_router(int id,
                            int grid_position_x, int grid_posistion_y, int layer_position,
                            double latency) {
    VTR_ASSERT_MSG(!built_noc, "NoC already built, cannot modify further.");

    router_storage.emplace_back(id, grid_position_x, grid_posistion_y, layer_position, latency);

    /* Get the corresponding NocRouterId for the newly added router and
     * add it to the conversion table.
     * Since the router is added at the end of the list, the id is equivalent to the last element index.
     * We build the conversion table here as it guarantees only unique routers
     * in the NoC are added.
     */
    NocRouterId converted_id((int)(router_storage.size() - 1));
    router_id_conversion_table.emplace(id, converted_id);

    /* need to associate the current router with its grid position */
    // get the key to identify the current router
    int router_key = generate_router_key_from_grid_location(grid_position_x, grid_posistion_y, layer_position);
    grid_location_to_router_id.insert(std::pair<int, NocRouterId>(router_key, converted_id));
}

void NocStorage::add_link(NocRouterId source, NocRouterId sink, double bandwidth, double latency) {
    VTR_ASSERT_MSG(!built_noc, "NoC already built, cannot modify further.");

    // the new link will be added to the back of the list,
    // so we can use the total number of links added so far as id
    NocLinkId added_link_id((int)link_storage.size());

    link_storage.emplace_back(added_link_id, source, sink, bandwidth, latency);

    router_outgoing_links_list[source].push_back(added_link_id);
    router_incoming_links_list[sink].push_back(added_link_id);
}

void NocStorage::set_noc_link_bandwidth(double link_bandwidth) {
    // Iterate over all links and set their bandwidth
    for (auto& link : link_storage) {
        link.set_bandwidth(link_bandwidth);
    }
}

void NocStorage::set_noc_link_latency(double link_latency) {
    noc_link_latency = link_latency;
}

void NocStorage::set_noc_router_latency(double router_latency) {
    noc_router_latency = router_latency;
}

void NocStorage::set_device_grid_width(int grid_width) {
    device_grid_width = grid_width;
}

void NocStorage::set_device_grid_spec(int grid_width, int grid_height) {
    device_grid_width = grid_width;
    layer_num_grid_locs = grid_width * grid_height;
}

bool NocStorage::remove_link(NocRouterId src_router_id, NocRouterId sink_router_id) {
    // This status variable is used to report externally whether the link was removed or not
    bool link_removed_status = false;

    // check if the src router for the link to remove exists (within the id ranges). Otherwise, there is no point looking for the link
    if ((size_t)src_router_id < router_storage.size()) {
        // get all the outgoing links of the provided source router
        std::vector<NocLinkId>& source_router_outgoing_links = router_outgoing_links_list[src_router_id];
        std::vector<NocLinkId>& sink_router_incoming_links = router_incoming_links_list[sink_router_id];

        const NocLinkId link_to_be_removed_id = get_single_noc_link_id(src_router_id, sink_router_id);
        link_removed_status = (link_to_be_removed_id != NocLinkId::INVALID());

        auto it = std::remove(source_router_outgoing_links.begin(),
                              source_router_outgoing_links.end(),
                              link_to_be_removed_id);

        if (it == source_router_outgoing_links.end()) {
            VTR_LOG_WARN("No link could be found among outgoing links of source router with id(%d) "
                "that that connects to the sink router with id (%d).\n",
                (size_t)src_router_id,
                (size_t)sink_router_id);
        }

        source_router_outgoing_links.erase(it, source_router_outgoing_links.end());

        it = std::remove(sink_router_incoming_links.begin(),
                         sink_router_incoming_links.end(),
                         link_to_be_removed_id);

        if (it == sink_router_incoming_links.end()) {
            VTR_LOG_WARN("No link could be found among incoming links of sink router with id(%d) "
                "that that connects to the source router with id (%d).\n",
                (size_t)sink_router_id,
                (size_t)src_router_id);
        }

        sink_router_incoming_links.erase(it, sink_router_incoming_links.end());

        link_storage[link_to_be_removed_id].set_source_router(NocRouterId::INVALID());
        link_storage[link_to_be_removed_id].set_sink_router(NocRouterId::INVALID());
        link_storage[link_to_be_removed_id].set_bandwidth_usage(-1);

    }

    // if a link was not removed then throw warning message
    if (!link_removed_status) {
        VTR_LOG_WARN("No link could be found that has a source router with id: '%d' and sink router with id:'%d'.\n",
                     (size_t)src_router_id,
                     (size_t)sink_router_id);
    }

    return link_removed_status;
}

void NocStorage::finished_building_noc() {
    VTR_ASSERT_MSG(!built_noc, "NoC already built, cannot modify further.");
    built_noc = true;

    returnable_noc_link_const_refs_.reserve(link_storage.size());

    /* We go through all NoC routers in the router_storage and check if there are any
     * two consecutive NoC routers whose latency is different. If such two routers are
     * found, we set detailed_router_latency_ to True.
     *
     * The values of detailed_link_latency_ and detailed_link_bandwidth_ are determined
     * in a similar way.
     */

    auto router_latency_it = std::adjacent_find(router_storage.begin(), router_storage.end(),
                                                [](const NocRouter& a, const NocRouter& b) {
                                                    return a.get_latency() != b.get_latency();
                                                });
    detailed_router_latency_ = (router_latency_it != router_storage.end());

    auto link_latency_it = std::adjacent_find(link_storage.begin(), link_storage.end(),
                                              [](const NocLink& a, const NocLink& b) {
                                                  return a.get_latency() != b.get_latency();
                                              });
    detailed_link_latency_ = (link_latency_it != link_storage.end());
}

void NocStorage::clear_noc() {
    router_storage.clear();
    link_storage.clear();
    router_outgoing_links_list.clear();
    router_incoming_links_list.clear();
    grid_location_to_router_id.clear();

    built_noc = false;
}

NocRouterId NocStorage::convert_router_id(int id) const {
    auto result = router_id_conversion_table.find(id);

    if (result == router_id_conversion_table.end()) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Cannot convert router with id:%d. The router was not found within the NoC.", id);
    }

    return result->second;
}

int NocStorage::convert_router_id(NocRouterId id) const {
    for (auto [user_id, router_id] : router_id_conversion_table) {
        if (router_id == id) {
            return user_id;
        }
    }

    VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Cannot convert router with id:%d. The router was not found within the NoC.", id);
}

void NocStorage::make_room_for_noc_router_link_list() {
    VTR_ASSERT_MSG(!built_noc, "NoC already built, cannot modify further.");
    router_outgoing_links_list.resize(router_storage.size());
    router_incoming_links_list.resize(router_storage.size());
}

NocLinkId NocStorage::get_parallel_link(NocLinkId current_link) const {
    // get the current source and sink router
    NocRouterId curr_source_router = link_storage[current_link].get_source_router();
    NocRouterId curr_sink_router = link_storage[current_link].get_sink_router();

    // get the link list of the sink router
    const std::vector<NocLinkId>& sink_router_links = router_outgoing_links_list[curr_sink_router];

    NocLinkId parallel_link = NocLinkId::INVALID();

    // go through the links of the sink router and the link that has the current source router as the sink router of the link is the parallel link we are looking for
    for (auto sink_router_link : sink_router_links) {
        if (link_storage[sink_router_link].get_sink_router() == curr_source_router) {
            parallel_link = sink_router_link;
            break;
        }
    }

    return parallel_link;
}

int NocStorage::generate_router_key_from_grid_location(int grid_position_x, int grid_position_y, int layer_position) const {
    // calculate the key value
    return (layer_num_grid_locs * layer_position + device_grid_width * grid_position_y + grid_position_x);
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
    fprintf(fp, "NoC Link Latency: %f\n", noc_link_latency);
    fprintf(fp, "\n");
    fprintf(fp, "NoC Router Latency: %f\n", noc_router_latency);
    fprintf(fp, "\n");

    // print all the routers and their properties
    fprintf(fp, "NoC Router List:\n");
    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "\n");

    // go through each router and print its information
    for (const auto& router : router_storage) {
        fprintf(fp, "Router %d:\n", router.get_router_user_id());
        // if the router tile is larger than a single grid, the position represents the bottom left corner of the tile
        fprintf(fp, "Equivalent Physical Tile Grid Position -> (%d,%d)\n", router.get_router_grid_position_x(), router.get_router_grid_position_y());
        fprintf(fp, "Router Connections (destination router id, link bandwidth, link latency) ->");

        auto& router_connections = this->get_noc_router_outgoing_links(this->convert_router_id(router.get_router_user_id()));

        // go through the outgoing links of the current router and print the connecting router
        for (auto router_connection : router_connections) {
            const auto& link = get_single_noc_link(router_connection);
            const NocRouterId connecting_router_id = link.get_sink_router();

            fprintf(fp, " (%d, %g, %g)",
                    get_single_noc_router(connecting_router_id).get_router_user_id(),
                    link.get_bandwidth(),
                    link.get_latency());
        }

        fprintf(fp, "\n\n");
    }

    fclose(fp);
}
