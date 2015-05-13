#include <iostream>

#include "assert.hpp"
#include "TimingConstraints.hpp"

float TimingConstraints::clock_constraint(DomainId src_domain, DomainId sink_domain) const {
    auto iter = clock_constraints_.find(std::make_pair(src_domain, sink_domain));
    ASSERT_MSG(iter != clock_constraints_.end(), "Could not find clock constraint");

    return iter->second;
}

float TimingConstraints::input_constraint(NodeId node_id) const {
    auto iter = input_constraints_.find(node_id);
    ASSERT_MSG(iter != input_constraints_.end(), "Could not find node input constraint");

    return iter->second;
}

float TimingConstraints::output_constraint(NodeId node_id) const {
    auto iter = input_constraints_.find(node_id);
    ASSERT_MSG(iter != input_constraints_.end(), "Could not find node output constraint");

    return iter->second;
}

void TimingConstraints::add_clock_constraint(const DomainId src_domain, const DomainId sink_domain, const float constraint) {
    std::cout << "SRC: " << src_domain << " SINK: " << sink_domain << " Constraint: " << constraint << std::endl;
}

void TimingConstraints::add_input_constraint(const NodeId node_id, const float constraint) {
    std:: cout << "Node: " << node_id << " Input_Constraint: " << constraint << std::endl;
}

void TimingConstraints::add_output_constraint(const NodeId node_id, const float constraint) {
    std::cout << "Node: " << node_id << " Output_Constraint: " << constraint << std::endl;
}

