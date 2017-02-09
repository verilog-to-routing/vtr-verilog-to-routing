#include "validate_timing_graph_constraints.hpp"
#include "TimingGraph.hpp"
#include "TimingConstraints.hpp"

#include "tatum_error.hpp"
#include "loop_detect.hpp"

namespace tatum {

bool validate_timing_graph_constraints(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints) {

    //Check that all clocks are defined as sources
    for(DomainId domain : timing_constraints.clock_domains()) {
        NodeId source_node = timing_constraints.clock_domain_source_node(domain);

        if(timing_graph.node_type(source_node) != NodeType::SOURCE) {
            std::string msg;

            msg = "Clock Domain " + std::to_string(size_t(domain)) + " (" + timing_constraints.clock_domain_name(domain) + ")"
                  " source node " + std::to_string(size_t(source_node)) + " is not a node of type SOURCE.";

            throw tatum::Error(msg);
        }
    }


    //Nothing here for now
    return true;
}

} //namespace
