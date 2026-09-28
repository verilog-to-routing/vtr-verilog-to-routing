#include "vpr_constraints.h"

UserPlaceConstraints& VprConstraints::mutable_place_constraints() {
    return placement_constraints_;
}

UserRouteConstraints& VprConstraints::mutable_route_constraints() {
    return route_constraints_;
}

const UserPlaceConstraints& VprConstraints::place_constraints() const {
    return placement_constraints_;
}

const UserRouteConstraints& VprConstraints::route_constraints() const {
    return route_constraints_;
}

UserRelativeMacros& VprConstraints::mutable_relative_macros() {
    return relative_macros_;
}

const UserRelativeMacros& VprConstraints::relative_macros() const {
    return relative_macros_;
}
