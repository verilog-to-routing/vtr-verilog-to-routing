#pragma once
#include <map>
#include <vector>
#include <unordered_set>

#include "timing_constraints_fwd.hpp"

#include "tatum_linear_map.hpp"
#include "tatum_range.hpp"
#include "timing_graph_fwd.hpp"

namespace tatum {


/**
 * The TimingConstraints class stores all the timing constraints applied during timing analysis.
 */
class TimingConstraints {
    public: //Types
        typedef tatum::util::linear_map<DomainId,DomainId>::const_iterator domain_iterator;
        typedef std::map<DomainPair,float>::const_iterator clock_constraint_iterator;
        typedef std::map<NodeId,IoConstraint>::const_iterator io_constraint_iterator;
        typedef std::unordered_set<NodeId>::const_iterator constant_generator_iterator;

        typedef tatum::util::Range<domain_iterator> domain_range;
        typedef tatum::util::Range<clock_constraint_iterator> clock_constraint_range;
        typedef tatum::util::Range<io_constraint_iterator> io_constraint_range;
        typedef tatum::util::Range<constant_generator_iterator> constant_generator_range;

    public: //Accessors
        //\returns A range containing all defined clock domains
        domain_range clock_domains() const;

        //\returns The name of a clock domain
        const std::string& clock_domain_name(const DomainId id) const;

        //\returns The source NodeId of the specified domain
        NodeId clock_domain_source_node(const DomainId id) const;

        bool is_virtual_clock(const DomainId) const;

        //\returns The domain of the specified node id if it is a clock source
        DomainId node_clock_domain(const NodeId id) const;

        //\returns True if the node id is a clock source
        bool node_is_clock_source(const NodeId id) const;

        //\returns True if the node id is a constant generator
        bool node_is_constant_generator(const NodeId id) const;

        //\returns A valid DomainId if a clock domain with the specified name exists, DomainId::INVALID() otherwise
        DomainId find_clock_domain(const std::string& name) const;

        ///Indicates whether the paths between src_domain and sink_domain should be analyzed
        ///\param src_domain The ID of the source (launch) clock domain
        ///\param sink_domain The ID of the sink (capture) clock domain
        bool should_analyze(const DomainId src_domain, const DomainId sink_domain) const;

        ///\returns The setup (max) constraint between src_domain and sink_domain
        float setup_constraint(const DomainId src_domain, const DomainId sink_domain) const;

        ///\returns The hold (min) constraint between src_domain and sink_domain
        float hold_constraint(const DomainId src_domain, const DomainId sink_domain) const;

        ///\returns The input delay constraint on node_id
        float input_constraint(const NodeId node_id, const DomainId domain_id) const;

        ///\returns The output delay constraint on node_id
        float output_constraint(const NodeId node_id, const DomainId domain_id) const;

        ///\returns A range of all constant generator nodes
        constant_generator_range constant_generators() const;

        //\returns A range of all setup constraints
        clock_constraint_range setup_constraints() const;

        //\returns A range of all setup constraints
        clock_constraint_range hold_constraints() const;

        //\returns A range of all input constraints
        io_constraint_range input_constraints() const;

        //\returns A range of all output constraints
        io_constraint_range output_constraints() const;

        //\returns A range of output constraints for the node id
        io_constraint_range input_constraints(const NodeId id) const;

        //\returns A range of input constraints for the node id
        io_constraint_range output_constraints(const NodeId id) const;

        ///Prints out the timing constraints for debug purposes
        void print() const;
    public: //Mutators
        ///\returns The DomainId of the clock with the specified name (will be created if it doesn not exist)
        DomainId create_clock_domain(const std::string name);

        ///Sets the setup constraint between src_domain and sink_domain with value constraint
        void set_setup_constraint(const DomainId src_domain, const DomainId sink_domain, const float constraint);

        ///Sets the hold constraint between src_domain and sink_domain with value constraint
        void set_hold_constraint(const DomainId src_domain, const DomainId sink_domain, const float constraint);

        ///Sets the input delay constraint on node_id with value constraint
        void set_input_constraint(const NodeId node_id, const DomainId domain_id, const float constraint);

        ///Sets the output delay constraint on node_id with value constraint
        void set_output_constraint(const NodeId node_id, const DomainId domain_id, const float constraint);

        ///Sets the source node for the specified clock domain
        void set_clock_domain_source(const NodeId node_id, const DomainId domain_id);

        ///Sets whether the specified node is a constant generator
        void set_constant_generator(const NodeId node_id, bool is_constant_generator=true);

        ///Update node IDs if they have changed
        ///\param node_map A vector mapping from old to new node ids
        void remap_nodes(const tatum::util::linear_map<NodeId,NodeId>& node_map);

    private:
        typedef std::map<NodeId,IoConstraint>::iterator mutable_io_constraint_iterator;
    private:
        ///\returns A valid domain id if the node is a clock source
        DomainId find_node_source_clock_domain(const NodeId node_id) const;

        io_constraint_iterator find_io_constraint(const NodeId node_id, const DomainId domain_id, const std::multimap<NodeId,IoConstraint>& io_constraints) const;
        mutable_io_constraint_iterator find_io_constraint(const NodeId node_id, const DomainId domain_id, std::multimap<NodeId,IoConstraint>& io_constraints);


    private: //Data
        tatum::util::linear_map<DomainId,DomainId> domain_ids_;
        tatum::util::linear_map<DomainId,std::string> domain_names_;
        tatum::util::linear_map<DomainId,NodeId> domain_sources_;

        std::unordered_set<NodeId> constant_generators_;

        std::map<DomainPair,float> setup_constraints_;
        std::map<DomainPair,float> hold_constraints_;
        std::multimap<NodeId,IoConstraint> input_constraints_;
        std::multimap<NodeId,IoConstraint> output_constraints_;
};

/*
 * Utility classes
 */
struct DomainPair {
    DomainPair(DomainId src, DomainId sink): src_domain_id(src), sink_domain_id(sink) {}

    friend bool operator<(const DomainPair& lhs, const DomainPair& rhs) {
        if(lhs.src_domain_id < rhs.src_domain_id) {
            return true;
        } else if(lhs.src_domain_id == rhs.src_domain_id 
                  && lhs.sink_domain_id < rhs.sink_domain_id) {
            return true;
        }
        return false;
    }

    DomainId src_domain_id;
    DomainId sink_domain_id;
};

struct IoConstraint {
    IoConstraint(DomainId domain_id, float constraint_val): domain(domain_id), constraint(constraint_val) {}

    DomainId domain;
    float constraint;
};

} //namepsace
