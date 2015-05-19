#include <iostream>

#include "assert.hpp"
#include "TimingConstraints.hpp"

using std::cout;
using std::endl;

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
    auto iter = output_constraints_.find(node_id);
    ASSERT_MSG(iter != input_constraints_.end(), "Could not find node output constraint");

    return iter->second;
}

void TimingConstraints::add_clock_constraint(const DomainId src_domain, const DomainId sink_domain, const float constraint) {
    //std::cout << "SRC: " << src_domain << " SINK: " << sink_domain << " Constraint: " << constraint << std::endl;
    auto key = std::make_pair(src_domain, sink_domain);
    auto iter = clock_constraints_.insert(std::make_pair(key, constraint));
    ASSERT_MSG(iter.second, "Attempted to insert duplicate entry");
}

void TimingConstraints::add_input_constraint(const NodeId node_id, const float constraint) {
    //std:: cout << "Node: " << node_id << " Input_Constraint: " << constraint << std::endl;
    auto iter = input_constraints_.insert(std::make_pair(node_id, constraint));
    ASSERT_MSG(iter.second, "Attempted to insert duplicate entry");
}

void TimingConstraints::add_output_constraint(const NodeId node_id, const float constraint) {
    //std::cout << "Node: " << node_id << " Output_Constraint: " << constraint << std::endl;
    auto iter = output_constraints_.insert(std::make_pair(node_id, constraint));
    ASSERT_MSG(iter.second, "Attempted to insert duplicate entry");
}

void TimingConstraints::remap_nodes(const std::vector<NodeId>& node_map) {
    std::map<NodeId,float> remapped_input_constraints;
    std::map<NodeId,float> remapped_output_constraints;

    for(auto kv : input_constraints_) {
        remapped_input_constraints[node_map[kv.first]] = kv.second;
    }
    for(auto kv : output_constraints_) {
        remapped_output_constraints[node_map[kv.first]] = kv.second;
    }

    std::swap(input_constraints_, remapped_input_constraints);
    std::swap(output_constraints_, remapped_output_constraints);
}

void TimingConstraints::print() {
    cout << "Clock Constraints" << endl;
    for(auto kv : clock_constraints_) {
        DomainId src_domain = kv.first.first;
        DomainId sink_domain = kv.first.second;
        float constraint = kv.second;
        cout << "SRC: " << src_domain;
        cout << " SINK: " << sink_domain;
        cout << " Constraint: " << constraint;
        cout << endl;
    }
    cout << "Input Constraints" << endl;
    for(auto kv : input_constraints_) {
        NodeId node_id = kv.first;
        float constraint = kv.second;
        cout << "Node: " << node_id;
        cout << " Constraint: " << constraint;
        cout << endl;
    }
    cout << "Output Constraints" << endl;
    for(auto kv : output_constraints_) {
        NodeId node_id = kv.first;
        float constraint = kv.second;
        cout << "Node: " << node_id;
        cout << " Constraint: " << constraint;
        cout << endl;
    }
}
