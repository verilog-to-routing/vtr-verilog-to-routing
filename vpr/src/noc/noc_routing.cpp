
#include "noc_routing.h"

NocRouting::~NocRouting(){}

void NocRouting::clear_routed_path(void) {

    routed_path.clear();
}

void NocRouting::add_link_to_routed_path(NocLinkId link_in_route) {

    routed_path.push_back(link_in_route);
}

const std::vector<NocLinkId>& NocRouting::get_routed_path(void) const {

    return routed_path;
}


