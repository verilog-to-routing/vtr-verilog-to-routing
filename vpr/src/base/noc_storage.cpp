#include "noc_storage.h"

// clear all the graph data structures 
NocStorage::NocStorage() {

    clear_noc();
}

// destructor
NocStorage::~NocStorage(){
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

    VTR_ASSERT(!built_noc);
    router_storage.emplace_back(id, grid_position_x, grid_posistion_y);

    return;
}

void NocStorage::add_link(NocRouterId source, NocRouterId sink){

    VTR_ASSERT(!built_noc);
    link_storage.emplace_back(source, sink);

    return;
}

void NocStorage::add_noc_router_link(NocRouterId router_id, NocLinkId link_id){

    VTR_ASSERT(!built_noc);
    router_link_list[router_id].push_back(link_id);

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

    VTR_ASSERT(!built_noc);
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

