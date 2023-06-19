#include "packer_constraint.h"

PackerConstraint::PackerConstraint() {
    net_name_ = std::string("");
    pin_name_ = std::string("");
    is_valid_ = false;
}

void PackerConstraint::set_net_name(std::string name) {
    net_name_ = name;
    return;
}

std::string PackerConstraint::net_name() const {
    return net_name_;
}

void PackerConstraint::set_pin_name(std::string name) {
    pin_name_ = name;
    return;
}

std::string PackerConstraint::pin_name() const {
    return net_name_;
}

void PackerConstraint::set_is_valid(bool value) {
    is_valid_ = value;
}

bool PackerConstraint::is_valid() const {
    return is_valid_;
}
