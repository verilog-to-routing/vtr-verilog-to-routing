#include "tatum/base/validate_timing_graph_constraints.hpp"
#include "tatum/TimingGraph.hpp"
#include "tatum/TimingConstraints.hpp"

#include "tatum/error.hpp"

namespace tatum {

bool validate_timing_graph_constraints(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints) {

    //Check that all clocks are defined as source nodes
    for(DomainId domain : timing_constraints.clock_domains()) {
        NodeId source_node = timing_constraints.clock_domain_source_node(domain);

        if(source_node) { //Virtual (i.e. IO) clocks may not have sources
            if(timing_graph.node_type(source_node) != NodeType::SOURCE) {
                std::string msg;

                msg = "Clock Domain " + std::to_string(size_t(domain)) + " (" + timing_constraints.clock_domain_name(domain) + ")"
                      " source node " + std::to_string(size_t(source_node)) + " is not a node of type SOURCE.";

                throw tatum::Error(msg, source_node);
            }
        }
    }

    //Check that any OPIN nodes with no incoming edges are constant generators
    for(NodeId node : timing_graph.nodes()) {
        if(timing_graph.node_type(node) == NodeType::OPIN && timing_graph.node_in_edges(node).size() == 0) {
            if(!timing_constraints.node_is_constant_generator(node)) {
                std::string msg;

                msg = "Timing Graph node " + std::to_string(size_t(node)) + " is an OPIN with no incoming edges, but is not marked as a constant generator";

                throw tatum::Error(msg, node);
            }
        }
    }

    //Nothing here for now
    return true;
}

} //namespace
