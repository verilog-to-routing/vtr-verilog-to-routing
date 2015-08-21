#pragma once
#include <map>
#include <vector>

#include "timing_graph_fwd.hpp"

/**
 * The TimingConstraints class stores all the timing constraints applied during timing analysis.
 */
class TimingConstraints {
    public:
        /*
         * Getters
         */
        ///Indicates whether the paths between src_domain and sink_domain should be analyzed
        ///\param src_domain The ID of the source (launch) clock domain
        ///\param sink_domain The ID of the sink (capture) clock domain
        bool should_analyze(DomainId src_domain, DomainId sink_domain) const;

        ///\returns The setup (max) constraint between src_domain and sink_domain
        float setup_clock_constraint(DomainId src_domain, DomainId sink_domain) const;

        ///\returns The hold (min) constraint between src_domain and sink_domain
        float hold_clock_constraint(DomainId src_domain, DomainId sink_domain) const;

        ///\returns The input delay constraint on node_id
        float input_constraint(NodeId node_id) const;

        ///\returns The output delay constraint on node_id
        float output_constraint(NodeId node_id) const;

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
        void remap_nodes(const std::vector<NodeId>& node_map);

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

