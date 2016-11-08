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

NodeId TimingConstraints::clock_domain_source_node(const DomainId id) const {
    return domain_sources_[id];
}

DomainId TimingConstraints::node_clock_domain(const NodeId id) const {
    //This is currenlty a linear search through all clock sources and
    //I/O constraints, could be made more efficient but it is only called
    //rarely (i.e. during pre-traversals)

    //Is it a clock source?
    DomainId source_domain = find_node_source_clock_domain(id);
    if(source_domain) return source_domain;

    //Does it have an input constarint?
    for(auto kv : input_constraints()) {
        auto key = kv.first;

        //TODO: Assumes a single clock per node
        if(key.node_id == id) return key.domain_id;
    }

    //Does it have an output constraint?
    for(auto kv : output_constraints()) {
        auto key = kv.first;

        //TODO: Assumes a single clock per node
        if(key.node_id == id) return key.domain_id;
    }

    //None found
    return DomainId::INVALID();
}

bool TimingConstraints::node_is_clock_source(const NodeId id) const {
    //Returns a DomainId which converts to true if valid
    return bool(find_node_source_clock_domain(id));
}

DomainId TimingConstraints::find_node_source_clock_domain(const NodeId node_id) const {
    //We don't expect many clocks, so the linear search should be fine
    for(auto domain_id : clock_domains()) {
        if(clock_domain_source_node(domain_id) == node_id) {
            return domain_id;
        }
    }

    return DomainId::INVALID();
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
    return setup_constraints_.count(DomainPair(src_domain, sink_domain)) 
           || hold_constraints_.count(DomainPair(src_domain, sink_domain));
}

float TimingConstraints::hold_constraint(const DomainId src_domain, const DomainId sink_domain) const {
    auto iter = hold_constraints_.find(DomainPair(src_domain, sink_domain));
    if(iter == hold_constraints_.end()) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    return iter->second;
}
float TimingConstraints::setup_constraint(const DomainId src_domain, const DomainId sink_domain) const {
    auto iter = setup_constraints_.find(DomainPair(src_domain, sink_domain));
    if(iter == setup_constraints_.end()) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    return iter->second;
}

float TimingConstraints::input_constraint(const NodeId node_id, const DomainId domain_id) const {
    auto key = NodeDomain(node_id, domain_id);
    auto iter = input_constraints_.find(key);
    if(iter == input_constraints_.end()) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    return iter->second;
}

float TimingConstraints::output_constraint(const NodeId node_id, const DomainId domain_id) const {
    auto key = NodeDomain(node_id, domain_id);
    auto iter = output_constraints_.find(key);
    if(iter == output_constraints_.end()) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    return iter->second;
}

TimingConstraints::clock_constraint_range TimingConstraints::setup_constraints() const {
    return tatum::util::make_range(setup_constraints_.begin(), setup_constraints_.end());
}

TimingConstraints::clock_constraint_range TimingConstraints::hold_constraints() const {
    return tatum::util::make_range(hold_constraints_.begin(), hold_constraints_.end());
}

TimingConstraints::io_constraint_range TimingConstraints::input_constraints() const {
    return tatum::util::make_range(input_constraints_.begin(), input_constraints_.end());
}

TimingConstraints::io_constraint_range TimingConstraints::output_constraints() const {
    return tatum::util::make_range(output_constraints_.begin(), output_constraints_.end());
}

DomainId TimingConstraints::create_clock_domain(const std::string name) { 
    DomainId id = find_clock_domain(name);
    if(!id) {
        //Create it
        id = DomainId(domain_ids_.size()); 
        domain_ids_.push_back(id); 
        
        domain_names_.push_back(name);
        domain_sources_.emplace_back(NodeId::INVALID());

        TATUM_ASSERT(clock_domain_name(id) == name);
        TATUM_ASSERT(find_clock_domain(name) == id);
    }

    return id; 
}

void TimingConstraints::set_setup_constraint(const DomainId src_domain, const DomainId sink_domain, const float constraint) {
    std::cout << "SRC: " << src_domain << " SINK: " << sink_domain << " Constraint: " << constraint << std::endl;
    auto key = DomainPair(src_domain, sink_domain);
    auto iter = setup_constraints_.insert(std::make_pair(key, constraint));
    TATUM_ASSERT_MSG(iter.second, "Attempted to insert duplicate setup clock constraint");
}

void TimingConstraints::set_hold_constraint(const DomainId src_domain, const DomainId sink_domain, const float constraint) {
    std::cout << "SRC: " << src_domain << " SINK: " << sink_domain << " Constraint: " << constraint << std::endl;
    auto key = DomainPair(src_domain, sink_domain);
    auto iter = hold_constraints_.insert(std::make_pair(key, constraint));
    TATUM_ASSERT_MSG(iter.second, "Attempted to insert duplicate hold clock constraint");
}

void TimingConstraints::set_input_constraint(const NodeId node_id, const DomainId domain_id, const float constraint) {
    auto key = NodeDomain(node_id, domain_id);
    auto iter = input_constraints_.insert(std::make_pair(key, constraint));
    TATUM_ASSERT_MSG(iter.second, "Attempted to insert duplicate input delay constraint");
}

void TimingConstraints::set_output_constraint(const NodeId node_id, const DomainId domain_id, const float constraint) {
    auto key = NodeDomain(node_id, domain_id);
    auto iter = output_constraints_.insert(std::make_pair(key, constraint));
    TATUM_ASSERT_MSG(iter.second, "Attempted to insert duplicate output delay constraint");
}

void TimingConstraints::set_clock_domain_source(const NodeId node_id, const DomainId domain_id) {
    domain_sources_[domain_id] = node_id;
}

void TimingConstraints::remap_nodes(const tatum::util::linear_map<NodeId,NodeId>& node_map) {
    std::map<NodeDomain,float> remapped_input_constraints;
    std::map<NodeDomain,float> remapped_output_constraints;
    tatum::util::linear_map<DomainId,NodeId> remapped_domain_sources(domain_sources_.size());

    for(auto kv : input_constraints_) {
        auto orig_key = kv.first;
        NodeId new_node_id = node_map[orig_key.node_id];

        auto new_key = NodeDomain(new_node_id, orig_key.domain_id);
        remapped_input_constraints[new_key] = kv.second;
    }
    for(auto kv : output_constraints_) {
        auto orig_key = kv.first;
        NodeId new_node_id = node_map[orig_key.node_id];

        auto new_key = NodeDomain(new_node_id, orig_key.domain_id);
        remapped_output_constraints[new_key] = kv.second;
    }

    for(size_t domain_idx = 0; domain_idx < domain_sources_.size(); ++domain_idx) {
        DomainId domain_id(domain_idx);

        NodeId old_node_id = domain_sources_[domain_id];
        if(old_node_id) {
            remapped_domain_sources[domain_id] = node_map[old_node_id];
        }
    }

    std::swap(input_constraints_, remapped_input_constraints);
    std::swap(output_constraints_, remapped_output_constraints);
    std::swap(domain_sources_, remapped_domain_sources);
}

void TimingConstraints::print() const {
    cout << "Setup Clock Constraints" << endl;
    for(auto kv : setup_constraints_) {
        auto key = kv.first;
        float constraint = kv.second;
        cout << "SRC: " << key.src_domain_id;
        cout << " SINK: " << key.sink_domain_id;
        cout << " Constraint: " << constraint;
        cout << endl;
    }
    cout << "Hold Clock Constraints" << endl;
    for(auto kv : hold_constraints_) {
        auto key = kv.first;
        float constraint = kv.second;
        cout << "SRC: " << key.src_domain_id;
        cout << " SINK: " << key.sink_domain_id;
        cout << " Constraint: " << constraint;
        cout << endl;
    }
    cout << "Input Constraints" << endl;
    for(auto kv : input_constraints_) {
        auto key = kv.first;
        float constraint = kv.second;
        cout << "Node: " << key.node_id;
        cout << " Domain: " << key.domain_id;
        cout << " Constraint: " << constraint;
        cout << endl;
    }
    cout << "Output Constraints" << endl;
    for(auto kv : output_constraints_) {
        auto key = kv.first;
        float constraint = kv.second;
        cout << "Node: " << key.node_id;
        cout << " Domain: " << key.domain_id;
        cout << " Constraint: " << constraint;
        cout << endl;
    }
}

} //namepsace
