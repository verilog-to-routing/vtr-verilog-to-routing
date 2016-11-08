#include <fstream>
#include <cmath>

#include "output.hpp"

#include "tatum_assert.hpp"
#include "TimingGraph.hpp"

#include "TimingConstraints.hpp"
#include "TimingTags.hpp"
#include "timing_analyzers.hpp"

using tatum::NodeId;
using tatum::EdgeId;
using tatum::DomainId;
using tatum::TimingTags;
using tatum::TimingTag;
using tatum::TimingGraph;
using tatum::TimingConstraints;

void write_tags(std::ostream& os, const std::string& type, const TimingTags& tags, const NodeId node_id);

void write_timing_graph(std::ostream& os, const TimingGraph& tg) {
    os << "timing_graph:" << "\n";

    //We manually iterate to write the nodes in ascending order
    for(size_t node_idx = 0; node_idx < tg.nodes().size(); ++node_idx) {
        NodeId node_id(node_idx);

        os << " node: " << size_t(node_id) << "\n";

        os << "  type: ";
        switch(tg.node_type(node_id)) {
            case tatum::NodeType::FF_SOURCE:
            case tatum::NodeType::INPAD_SOURCE:
            case tatum::NodeType::CLOCK_SOURCE:
            case tatum::NodeType::CONSTANT_GEN_SOURCE:
                os << "SOURCE";
                break;
            case tatum::NodeType::OUTPAD_IPIN:
            case tatum::NodeType::FF_IPIN:
            case tatum::NodeType::PRIMITIVE_IPIN:
                os << "IPIN";
                break;
            case tatum::NodeType::INPAD_OPIN:
            case tatum::NodeType::FF_OPIN:
            case tatum::NodeType::PRIMITIVE_OPIN:
            case tatum::NodeType::CLOCK_OPIN:
                os << "OPIN";
                break;
            case tatum::NodeType::FF_SINK:
            case tatum::NodeType::OUTPAD_SINK:
            case tatum::NodeType::FF_CLOCK:
                os << "SINK";
                break;
            default:
                TATUM_ASSERT(false);
        }
        os << "\n";

        os << "  in_edges: ";
        for(EdgeId edge_id : tg.node_in_edges(node_id)) {
            os << size_t(edge_id) << " ";
        }
        os << "\n";

        os << "  out_edges: ";  
        for(EdgeId edge_id : tg.node_out_edges(node_id)) {
            os << size_t(edge_id) << " ";
        }
        os << "\n";

    }

    //We manually iterate to write the edges in ascending order
    for(size_t edge_idx = 0; edge_idx < tg.edges().size(); ++edge_idx) {
        EdgeId edge_id(edge_idx);

        os << " edge: " << size_t(edge_id) << "\n";
        os << "  src_node: " << size_t(tg.edge_src_node(edge_id)) << "\n";
        os << "  sink_node: " << size_t(tg.edge_sink_node(edge_id)) << "\n";
    }
    os << "\n";
}

void write_timing_constraints(std::ostream& os, const TimingConstraints& tc) {
    os << "timing_constraints:\n";    

    for(auto domain_id : tc.clock_domains()) {
        os << " type: CLOCK domain: " << size_t(domain_id) << " name: \"" << tc.clock_domain_name(domain_id) << "\"\n";
    }

    for(auto domain_id : tc.clock_domains()) {
        NodeId source_node_id = tc.clock_domain_source_node(domain_id);
        if(source_node_id) {
            os << " type: CLOCK_SOURCE node: " << size_t() << " domain: " << size_t(domain_id) << "\n";
        }
    }

    for(auto kv : tc.input_constraints()) {
        auto key = kv.first;
        auto constraint = kv.second;
        if(!isnan(constraint)) {
            os << " type: INPUT_CONSTRAINT node: " << size_t(key.node_id) << " domain: " << size_t(key.domain_id) << " constraint: " << constraint << "\n";
        }
    }

    for(auto kv : tc.output_constraints()) {
        auto key = kv.first;
        auto constraint = kv.second;
        if(!isnan(constraint)) {
            os << " type: OUTPUT_CONSTRAINT node: " << size_t(key.node_id) << " domain: " << size_t(key.domain_id) << " constraint: " << constraint << "\n";
        }
    }

    for(auto kv : tc.setup_constraints()) {
        auto key = kv.first;
        auto constraint = kv.second;
        if(!isnan(constraint)) {
            os << " type: SETUP_CONSTRAINT";
            os << " src_domain: " << size_t(key.src_domain_id);
            os << " sink_domain: " << size_t(key.sink_domain_id);
            os << " constraint: " << constraint;
            os << "\n";
        }
    }

    for(auto kv : tc.hold_constraints()) {
        auto key = kv.first;
        auto constraint = kv.second;
        if(!isnan(constraint)) {
            os << " type: HOLD_CONSTRAINT";
            os << " src_domain: " << size_t(key.src_domain_id);
            os << " sink_domain: " << size_t(key.sink_domain_id);
            os << " constraint: " << constraint;
            os << "\n";
        }
    }
    os << "\n";
}

void write_analysis_result(std::ostream& os, const TimingGraph& tg, const std::shared_ptr<tatum::TimingAnalyzer> analyzer) {
    os << "analysis_result:\n";

    auto setup_analyzer = std::dynamic_pointer_cast<tatum::SetupTimingAnalyzer>(analyzer);
    if(setup_analyzer) {
        for(size_t node_idx = 0; node_idx < tg.nodes().size(); ++node_idx) {
            NodeId node_id(node_idx);
            write_tags(os, "SETUP_DATA", setup_analyzer->get_setup_data_tags(node_id), node_id);
        }
        for(size_t node_idx = 0; node_idx < tg.nodes().size(); ++node_idx) {
            NodeId node_id(node_idx);
            write_tags(os, "SETUP_CLOCK", setup_analyzer->get_setup_clock_tags(node_id), node_id);
        }
    }
    auto hold_analyzer = std::dynamic_pointer_cast<tatum::HoldTimingAnalyzer>(analyzer);
    if(hold_analyzer) {
        for(size_t node_idx = 0; node_idx < tg.nodes().size(); ++node_idx) {
            NodeId node_id(node_idx);
            write_tags(os, "HOLD_DATA", hold_analyzer->get_hold_data_tags(node_id), node_id);
        }
        for(size_t node_idx = 0; node_idx < tg.nodes().size(); ++node_idx) {
            NodeId node_id(node_idx);
            write_tags(os, "HOLD_CLOCK", hold_analyzer->get_hold_clock_tags(node_id), node_id);
        }
    }
    os << "\n";
}

void write_tags(std::ostream& os, const std::string& type, const TimingTags& tags, const NodeId node_id) {
    for(const auto& tag : tags) {
        float arr = tag.arr_time().value();
        float req = tag.req_time().value();

        if(!isnan(arr) || !isnan(req)) {
            os << " type: " << type;
            os << " node: " << size_t(node_id);
            os << " domain: " << size_t(tag.clock_domain());

            if(!isnan(arr)) {
                os << " arr: " << arr;
            }

            if(!isnan(arr)) {
                os << " req: " << tag.req_time().value();
            }

            os << "\n";
        }
    }
}
