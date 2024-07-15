#include "user_route_constraints.h"

void UserRouteConstraints::add_route_constraint(std::string net_name, RoutingScheme route_scheme) {
    route_constraints_.insert({net_name, route_scheme});
}

const std::pair<std::string, RoutingScheme> UserRouteConstraints::get_route_constraint_by_idx(std::size_t idx) const {
    RoutingScheme route_scheme;

    // throw an error if the index is out of range
    if ((route_constraints_.size() == 0) || (idx > route_constraints_.size() - 1)) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "in get_route_constraint_by_idx: index %u is out of range. The unordered map for route constraints has a size of %u\n",
                        idx, route_constraints_.size());
    }

    auto it = route_constraints_.begin();
    std::advance(it, idx);
    return *it;
}

bool UserRouteConstraints::has_routing_constraint(std::string net_name) const {
    // Check if there's an exact match for the net name
    auto const& rc_itr = route_constraints_.find(net_name);
    if (rc_itr != route_constraints_.end()) {
        return true;
    }

    // Check for wildcard matches
    for (const auto& route_constraint : route_constraints_) {
        if (std::regex_match(net_name, std::regex(route_constraint.first))) {
            return true;
        }
    }

    // Return false if no match is found
    return false;
}

const RoutingScheme UserRouteConstraints::get_route_scheme_by_net_name(std::string net_name) const {
    if (has_routing_constraint(net_name) == false) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "in get_route_scheme_by_net_name: no routing constraints exist for net name %s \n",
                        net_name.c_str());
    }

    RoutingScheme route_scheme;
    auto const& rs_itr = route_constraints_.find(net_name);
    if (rs_itr == route_constraints_.end()) {
        // Check for wildcard matches
        for (auto constraint : route_constraints_) {
            if (std::regex_match(net_name, std::regex(constraint.first))) {
                route_scheme = constraint.second;
                break;
            }
        }
    } else {
        route_scheme = rs_itr->second;
    }
    return route_scheme;
}

e_clock_modeling UserRouteConstraints::get_route_model_by_net_name(std::string net_name) const {
    RoutingScheme route_scheme = get_route_scheme_by_net_name(net_name);
    return route_scheme.route_model();
}

const std::string UserRouteConstraints::get_routing_network_name_by_net_name(std::string net_name) const {
    RoutingScheme route_scheme = get_route_scheme_by_net_name(net_name);
    return route_scheme.network_name();
}

int UserRouteConstraints::get_num_route_constraints(void) const {
    return route_constraints_.size();
}