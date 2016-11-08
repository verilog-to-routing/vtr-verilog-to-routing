#pragma once
#include <map>
#include <vector>

#include "tatum_linear_map.hpp"
#include "tatum_range.hpp"
#include "timing_graph_fwd.hpp"

namespace tatum {

/**
 * The TimingConstraints class stores all the timing constraints applied during timing analysis.
 */
#if 0
class TimingConstraints {
    public:
        /*
         * Getters
         */
        ///Indicates whether the paths between src_domain and sink_domain should be analyzed
        ///\param src_domain The ID of the source (launch) clock domain
        ///\param sink_domain The ID of the sink (capture) clock domain
        bool should_analyze(const DomainId src_domain, const DomainId sink_domain) const;

        ///\returns The setup (max) constraint between src_domain and sink_domain
        float setup_clock_constraint(const DomainId src_domain, const DomainId sink_domain) const;

        ///\returns The hold (min) constraint between src_domain and sink_domain
        float hold_clock_constraint(const DomainId src_domain, const DomainId sink_domain) const;

        ///\returns The input delay constraint on node_id
        float input_constraint(const NodeId node_id) const;

        ///\returns The output delay constraint on node_id
        float output_constraint(const NodeId node_id) const;

        /*
         * Setters
         */
        ///Adds a setup constraint between src_domain and sink_domain with value constraint
        void add_setup_clock_constraint(const DomainId src_domain, const DomainId sink_domain, const float constraint);

        ///Adds a hold constraint between src_domain and sink_domain with value constraint
        void add_hold_clock_constraint(const DomainId src_domain, const DomainId sink_domain, const float constraint);

        ///Adds an input delay constraint on node_id with value constraint
        void add_input_constraint(const NodeId node_id, const float constraint);

        ///Adds an output delay constraint on node_id with value constraint
        void add_output_constraint(const NodeId node_id, const float constraint);

        ///Update node IDs if they have changed
        ///\param node_map A vector mapping from old to new node ids
        void remap_nodes(const tatum::util::linear_map<NodeId,NodeId>& node_map);

        ///Prints out the timing constraints for debug purposes
        void print();
    private:
        //TODO: determine better data structures for this
        //Clock constraints
        std::map<std::pair<DomainId,DomainId>,float> setup_clock_constraints_;
        std::map<std::pair<DomainId,DomainId>,float> hold_clock_constraints_;
        std::map<NodeId,float> input_constraints_;
        std::map<NodeId,float> output_constraints_;
};
#else

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

struct NodeDomain {
    NodeDomain(NodeId node, DomainId domain): node_id(node), domain_id(domain) {}

    friend bool operator<(const NodeDomain& lhs, const NodeDomain& rhs) {
        if(lhs.node_id < rhs.node_id) {
            return true;
        } else if(lhs.node_id == rhs.node_id 
                  && lhs.domain_id < rhs.domain_id) {
            return true;
        }
        return false;
    }

    NodeId node_id;
    DomainId domain_id;
};

class TimingConstraints {
    public: //Types
        typedef tatum::util::linear_map<DomainId,DomainId>::const_iterator domain_iterator;
        typedef std::map<DomainPair,float>::const_iterator clock_constraint_iterator;
        typedef std::map<NodeDomain,float>::const_iterator io_constraint_iterator;

        typedef tatum::util::Range<domain_iterator> domain_range;
        typedef tatum::util::Range<clock_constraint_iterator> clock_constraint_range;
        typedef tatum::util::Range<io_constraint_iterator> io_constraint_range;

    public: //Accessors
        //\returns A range containing all defined clock domains
        domain_range clock_domains() const;

        //\returns The name of a clock domain
        const std::string& clock_domain_name(const DomainId id) const;

        //\returns The source NodeId of the specified domain
        NodeId clock_domain_source(const DomainId id) const;

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

        clock_constraint_range setup_constraints() const;
        clock_constraint_range hold_constraints() const;
        io_constraint_range input_constraints() const;
        io_constraint_range output_constraints() const;

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

        void set_clock_domain_source(const NodeId node_id, const DomainId domain_id);

        ///Update node IDs if they have changed
        ///\param node_map A vector mapping from old to new node ids
        void remap_nodes(const tatum::util::linear_map<NodeId,NodeId>& node_map);

    private: //Data
        tatum::util::linear_map<DomainId,DomainId> domain_ids_;
        tatum::util::linear_map<DomainId,std::string> domain_names_;
        tatum::util::linear_map<DomainId,NodeId> domain_sources_;

        std::map<DomainPair,float> setup_constraints_;
        std::map<DomainPair,float> hold_constraints_;
        std::map<NodeDomain,float> input_constraints_;
        std::map<NodeDomain,float> output_constraints_;
};

#endif

} //namepsace
