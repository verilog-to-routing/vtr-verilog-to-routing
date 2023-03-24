#include "route_constraint.h"

RouteConstraint::RouteConstraint() {
    net_name_ = std::string("");
    net_type_ = std::string("");
    route_method_ = std::string("");
    is_valid_ = false;
}

void RouteConstraint::set_net_name(std::string name) {
    net_name_ = name;
    return;
}

std::string RouteConstraint::net_name() const {
    return net_name_;
}

void RouteConstraint::set_net_type(std::string type) {
    net_type_ = type;
    return;
}

std::string RouteConstraint::net_type() const {
    return net_type_;
}

void RouteConstraint::set_route_model(std::string route_method) {
    route_method_ = route_method;
    return;
}

std::string RouteConstraint::route_model() const {
    return route_method_;
}

void RouteConstraint::set_is_valid(bool value) {
    is_valid_ = value;
}

bool RouteConstraint::is_valid() const {
    return is_valid_;
}
