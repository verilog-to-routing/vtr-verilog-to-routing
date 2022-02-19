#include "noc_storage.h"

// clear all the graph data structures 
NocStorage::NocStorage() {

    clear_noc();
}

// getters for the NoC

// get the outgoing links for a router in the NoC
const std::vector<NocLinkId>& NocStorage::get_noc_router_connections(NocRouterId id) const{

    return router_link_list[id];

}

int NocStorage::get_noc_router_grid_position_x(NocRouterId id) const{

    return router_storage[id].get_router_grid_position_x();
}

int NocStorage::get_noc_router_grid_position_y(NocRouterId id) const{

    return router_storage[id].get_router_grid_position_y();
}

int NocStorage::get_noc_router_id(NocRouterId id) const{

    return router_storage[id].get_router_id();
}

std::string NocStorage::get_noc_router_design_module_ref(NocRouterId id) const{

    return router_storage[id].get_router_design_module_ref();
}

NocRouterId NocStorage::get_noc_link_source_router(NocLinkId id) const{

    return link_storage[id].get_source_router();
}

NocRouterId NocStorage::get_noc_link_sink_router(NocLinkId id) const{

    return link_storage[id].get_sink_router();
}

double NocStorage::get_noc_link_bandwidth_usage(NocLinkId id) const{

    return link_storage[id].get_bandwidth_usage();
}

int NocStorage::get_noc_link_number_of_connections(NocLinkId id) const{

    return link_storage[id].get_number_of_connections();
}

void NocStorage::add_router(int id, int grid_position_x, int grid_posistion_y){

    VTR_ASSERT_MSG(!built_noc, "NoC already built, cannot modify further.");
        
    router_storage.emplace_back(id, grid_position_x, grid_posistion_y);

    /* Get the corresponding NocRouterId for the newly added router and
       add it to the conversion table.
       Since the router is added at the end of the list, the id is equivalent to the last element index.
       We build the conversion table here as it gurantees only unique routers
       in the NoC are added.
    */
    NocRouterId converted_id((int)(router_storage.size() - 1));
    router_id_conversion_table.emplace(id, converted_id);

    return;
}

void NocStorage::add_link(NocRouterId source, NocRouterId sink){

    VTR_ASSERT_MSG(!built_noc, "NoC already built, cannot modify further.");
    link_storage.emplace_back(source, sink);

    // the newly added link was added to the back of the list, so we can get the id as the last element in the list
    NocLinkId added_link_id((int)link_storage.size() - 1);
    router_link_list[source].push_back(added_link_id);

    return;
}

void NocStorage::set_noc_router_design_module_ref(NocRouterId id, std::string design_module_ref){

    router_storage[id].set_router_design_module_ref(design_module_ref);

    return;
}

void NocStorage::set_noc_link_bandwidth_usage(NocLinkId id, double bandwidth_usage){

    link_storage[id].set_bandwidth_usage(bandwidth_usage);

    return;
}

void NocStorage::set_noc_link_number_of_connections(NocLinkId id, int number_of_connections){

    link_storage[id].set_number_of_connections(number_of_connections);

    return;
}

void NocStorage::finished_building_noc(void){

    VTR_ASSERT_MSG(!built_noc, "NoC already built, cannot modify further.");
    built_noc = true;

    return;
}

void NocStorage::clear_noc(void){

    router_storage.clear();
    link_storage.clear();
    router_link_list.clear();

    built_noc = false;

    return;
}

NocRouterId NocStorage::convert_router_id(int id) const{

    std::unordered_map<int, NocRouterId>::const_iterator result = router_id_conversion_table.find(id);

    if (result == router_id_conversion_table.end())
    {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Cannot convert router with id:%d. The router was not found within the NoC.", id);
    }

    return result->second;
    
}

void NocStorage::make_room_for_noc_router_link_list(){

    VTR_ASSERT_MSG(!built_noc, "NoC already built, cannot modify further.");
    router_link_list.resize(router_storage.size());
}

