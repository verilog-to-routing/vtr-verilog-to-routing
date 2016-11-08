#include <iostream>
#include <limits>

#include "tatum_assert.hpp"
#include "TimingConstraints.hpp"

using std::cout;
using std::endl;

namespace tatum {

TimingConstraints::domain_range TimingConstraints::clock_domains() const {
    return tatum::util::make_range(domain_ids_.begin(), domain_ids_.end());
}

const std::string& TimingConstraints::clock_domain_name(const DomainId id) const {
    return domain_names_[id];
}

DomainId TimingConstraints::find_clock_domain(const std::string& name) const {
    //Linear search for name
    // We don't expect a large number of domains
    for(DomainId id : clock_domains()) {
        if(clock_domain_name(id) == name) {
            return id;
        }
    }

    //Not found
    return DomainId::INVALID();
}

bool TimingConstraints::should_analyze(const DomainId src_domain, const DomainId sink_domain) const {
    return setup_constraints_.count(std::make_pair(src_domain, sink_domain)) 
           || hold_constraints_.count(std::make_pair(src_domain, sink_domain));
}

float TimingConstraints::hold_constraint(const DomainId src_domain, const DomainId sink_domain) const {
    auto iter = hold_constraints_.find(std::make_pair(src_domain, sink_domain));
    if(iter == hold_constraints_.end()) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    return iter->second;
}
float TimingConstraints::setup_constraint(const DomainId src_domain, const DomainId sink_domain) const {
    auto iter = setup_constraints_.find(std::make_pair(src_domain, sink_domain));
    if(iter == setup_constraints_.end()) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    return iter->second;
}

float TimingConstraints::input_constraint(const NodeId node_id, const DomainId domain_id) const {
    auto key = std::make_pair(node_id, domain_id);
    auto iter = input_constraints_.find(key);
    if(iter == input_constraints_.end()) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    return iter->second;
}

float TimingConstraints::output_constraint(const NodeId node_id, const DomainId domain_id) const {
    auto key = std::make_pair(node_id, domain_id);
    auto iter = output_constraints_.find(key);
    if(iter == output_constraints_.end()) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    return iter->second;
}

DomainId TimingConstraints::create_clock_domain(const std::string name) { 
    DomainId id = find_clock_domain(name);
    if(!id) {
        //Create it
        id = DomainId(domain_ids_.size()); 
        domain_ids_.push_back(id); 
        
        domain_names_.push_back(name);

        TATUM_ASSERT(clock_domain_name(id) == name);
        TATUM_ASSERT(find_clock_domain(name) == id);
    }

    return id; 
}

void TimingConstraints::set_setup_constraint(const DomainId src_domain, const DomainId sink_domain, const float constraint) {
    std::cout << "SRC: " << src_domain << " SINK: " << sink_domain << " Constraint: " << constraint << std::endl;
    auto key = std::make_pair(src_domain, sink_domain);
    auto iter = setup_constraints_.insert(std::make_pair(key, constraint));
    TATUM_ASSERT_MSG(iter.second, "Attempted to insert duplicate setup clock constraint");
}

void TimingConstraints::set_hold_constraint(const DomainId src_domain, const DomainId sink_domain, const float constraint) {
    std::cout << "SRC: " << src_domain << " SINK: " << sink_domain << " Constraint: " << constraint << std::endl;
    auto key = std::make_pair(src_domain, sink_domain);
    auto iter = hold_constraints_.insert(std::make_pair(key, constraint));
    TATUM_ASSERT_MSG(iter.second, "Attempted to insert duplicate hold clock constraint");
}

void TimingConstraints::set_input_constraint(const NodeId node_id, const DomainId domain_id, const float constraint) {
    auto key = std::make_pair(node_id, domain_id);
    auto iter = input_constraints_.insert(std::make_pair(key, constraint));
    TATUM_ASSERT_MSG(iter.second, "Attempted to insert duplicate input delay constraint");
}

void TimingConstraints::set_output_constraint(const NodeId node_id, const DomainId domain_id, const float constraint) {
    auto key = std::make_pair(node_id, domain_id);
    auto iter = output_constraints_.insert(std::make_pair(key, constraint));
    TATUM_ASSERT_MSG(iter.second, "Attempted to insert duplicate output delay constraint");
}

void TimingConstraints::remap_nodes(const tatum::util::linear_map<NodeId,NodeId>& node_map) {
    std::map<std::pair<NodeId,DomainId>,float> remapped_input_constraints;
    std::map<std::pair<NodeId,DomainId>,float> remapped_output_constraints;

    for(auto kv : input_constraints_) {
        auto orig_key = kv.first;
        NodeId orig_node_id = orig_key.first;
        DomainId orig_domain_id = orig_key.second;
        NodeId new_node_id = node_map[orig_node_id];

        auto new_key = std::make_pair(new_node_id, orig_domain_id);
        remapped_input_constraints[new_key] = kv.second;
    }
    for(auto kv : output_constraints_) {
        auto orig_key = kv.first;
        NodeId orig_node_id = orig_key.first;
        DomainId orig_domain_id = orig_key.second;
        NodeId new_node_id = node_map[orig_node_id];

        auto new_key = std::make_pair(new_node_id, orig_domain_id);
        remapped_output_constraints[new_key] = kv.second;
    }

    std::swap(input_constraints_, remapped_input_constraints);
    std::swap(output_constraints_, remapped_output_constraints);
}

void TimingConstraints::print() const {
    cout << "Setup Clock Constraints" << endl;
    for(auto kv : setup_constraints_) {
        DomainId src_domain = kv.first.first;
        DomainId sink_domain = kv.first.second;
        float constraint = kv.second;
        cout << "SRC: " << src_domain;
        cout << " SINK: " << sink_domain;
        cout << " Constraint: " << constraint;
        cout << endl;
    }
    cout << "Hold Clock Constraints" << endl;
    for(auto kv : hold_constraints_) {
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
        NodeId node_id = kv.first.first;
        DomainId domain_id = kv.first.second;
        float constraint = kv.second;
        cout << "Node: " << node_id;
        cout << " Domain: " << domain_id;
        cout << " Constraint: " << constraint;
        cout << endl;
    }
    cout << "Output Constraints" << endl;
    for(auto kv : output_constraints_) {
        NodeId node_id = kv.first.first;
        DomainId domain_id = kv.first.second;
        float constraint = kv.second;
        cout << "Node: " << node_id;
        cout << " Domain: " << domain_id;
        cout << " Constraint: " << constraint;
        cout << endl;
    }
}

} //namepsace
