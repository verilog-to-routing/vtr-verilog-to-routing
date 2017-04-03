#include "tatum/TimingGraph.hpp"
#include "tatum/base/sta_util.hpp"
#include <iostream>
#include <sstream>
#include <iterator>
#include <string>

namespace tatum {

constexpr size_t MAX_DOT_GRAPH_NODES = 1000;

template<class DelayCalc>
GraphvizDotWriter<DelayCalc>::GraphvizDotWriter(const TimingGraph& tg, const DelayCalc& delay_calc)
    : tg_(tg)
    , delay_calc_(delay_calc) {

    //By default dump all nodes
    auto nodes = tg_.nodes();
    nodes_to_dump_ = std::set<NodeId>(nodes.begin(), nodes.end());
}

template<class DelayCalc>
void GraphvizDotWriter<DelayCalc>::write_dot_file(std::string filename) {
    std::ofstream os(filename);
    write_dot_file(os);
}

template<class DelayCalc>
void GraphvizDotWriter<DelayCalc>::write_dot_file(std::ostream& os) {
    std::map<NodeId,std::vector<TimingTag>> node_tags;
    for(NodeId node : nodes_to_dump_) {
        node_tags[node] = std::vector<TimingTag>(); //No tags
    }

    std::map<NodeId,std::vector<TimingTag>> node_slacks;
    for(NodeId node : nodes_to_dump_) {
        node_slacks[node] = std::vector<TimingTag>(); //No slacks
    }

    write_dot_format(os, node_tags, node_slacks);
}

template<class DelayCalc>
void GraphvizDotWriter<DelayCalc>::write_dot_file(std::string filename, const TimingAnalyzer& analyzer) {
    std::ofstream os(filename);
    write_dot_file(os, analyzer);
}

template<class DelayCalc>
void GraphvizDotWriter<DelayCalc>::write_dot_file(std::ostream& os, const TimingAnalyzer& analyzer) {
    std::map<NodeId,std::vector<TimingTag>> node_tags;
    std::map<NodeId,std::vector<TimingTag>> node_slacks;
    TimingType timing_type = TimingType::UNKOWN;

    //This is ugly, but makes it transparent to the user who just passes thier analyzer
    const SetupTimingAnalyzer* setup_analyzer = dynamic_cast<const SetupTimingAnalyzer*>(&analyzer);
    const HoldTimingAnalyzer* hold_analyzer = dynamic_cast<const HoldTimingAnalyzer*>(&analyzer);

    for(NodeId node : nodes_to_dump_) {
        if(setup_analyzer) {
            auto tags = setup_analyzer->setup_tags(node);
            std::copy(tags.begin(), tags.end(), std::back_inserter(node_tags[node]));

            auto slacks = setup_analyzer->setup_slacks(node);
            std::copy(slacks.begin(), slacks.end(), std::back_inserter(node_slacks[node]));

            timing_type = TimingType::SETUP;
        } else {
            TATUM_ASSERT(hold_analyzer);

            auto tags = hold_analyzer->hold_tags(node);
            std::copy(tags.begin(), tags.end(), std::back_inserter(node_tags[node]));

            auto slacks = hold_analyzer->hold_slacks(node);
            std::copy(slacks.begin(), slacks.end(), std::back_inserter(node_slacks[node]));

            timing_type = TimingType::HOLD;
        }
    }

    write_dot_format(os, node_tags, node_slacks, timing_type);
}


template<class DelayCalc>
void GraphvizDotWriter<DelayCalc>::write_dot_format(std::ostream& os, 
                                         const std::map<NodeId,std::vector<TimingTag>>& node_tags,
                                         const std::map<NodeId,std::vector<TimingTag>>& node_slacks,
                                         TimingType timing_type) {
    TATUM_ASSERT(timing_type == TimingType::SETUP || timing_type == TimingType::HOLD);

    os << "digraph G {" << std::endl;
    os << "\tnode[shape=record]" << std::endl;

    for(const NodeId node: nodes_to_dump_) {
        auto tag_iter = node_tags.find(node);
        TATUM_ASSERT(tag_iter != node_tags.end());

        auto slack_iter = node_slacks.find(node);
        TATUM_ASSERT(slack_iter != node_slacks.end());

        write_dot_node(os, node, tag_iter->second, slack_iter->second);
    }

    for(const LevelId level : tg_.levels()) {
        write_dot_level(os, level);
    }

    for(const EdgeId edge : tg_.edges()) {
        write_dot_edge(os, edge, timing_type);
    }

    os << "}" << std::endl;
}

template<class DelayCalc>
void GraphvizDotWriter<DelayCalc>::write_dot_node(std::ostream& os, 
                                       const NodeId node, 
                                       const std::vector<TimingTag>& tags, 
                                       const std::vector<TimingTag>& slacks) {
    os << "\t";
    os << node_name(node);
    os << "[label=\"";
    os << "{" << node << " (" << tg_.node_type(node) << ")";

    for(const auto& tag_set : {tags, slacks}) {
        for(const auto& tag : tag_set) {
            os << " | {";
            os << tag.type() << "\\n";
            tag_domain_from_to(os, tag);
            if(tag.origin_node()) {
                if(tag.type() == TagType::CLOCK_LAUNCH || tag.type() == TagType::CLOCK_CAPTURE || tag.type() == TagType::DATA_ARRIVAL) {
                    os << " from ";
                } else {
                    os << " for ";
                }
                os << tag.origin_node();
            } else {
                os << " [Origin] ";
            }
            os << "\\n";
            os << "time: " << tag.time().value();
            os << "}";
        }
    }
    os << "}\"]";
    os << std::endl;
}

template<class DelayCalc>
void GraphvizDotWriter<DelayCalc>::write_dot_level(std::ostream& os, const LevelId level) {
    os << "\t{rank = same; ";
    for(const NodeId node : tg_.level_nodes(level)) {
        if(nodes_to_dump_.count(node)) {
            os << node_name(node) <<"; ";
        }
    }
    os << "}" << std::endl;
}

template<class DelayCalc>
void GraphvizDotWriter<DelayCalc>::write_dot_edge(std::ostream& os, const EdgeId edge, const TimingType timing_type) {
    NodeId src_node = tg_.edge_src_node(edge);
    NodeId sink_node = tg_.edge_sink_node(edge);

    if(nodes_to_dump_.count(src_node) && nodes_to_dump_.count(sink_node)) {
        //Only draw edges to nodes in the set of nodes being printed

        EdgeType edge_type = tg_.edge_type(edge);

        std::string color = "";
        os << "\t" << node_name(src_node) << " -> " << node_name(sink_node);
        os << " [ label=\"" << edge;
        if(edge_type == EdgeType::PRIMITIVE_CLOCK_CAPTURE) {
            color = CLOCK_CAPTURE_EDGE_COLOR;
            if (timing_type == TimingType::SETUP) {
                os << "\\n"<< -delay_calc_.setup_time(tg_, edge) << " (-tsu)";
            } else {
                //Hold
                TATUM_ASSERT(timing_type == TimingType::HOLD);
                os << "\\n"<< delay_calc_.hold_time(tg_, edge) << " (thld)";
            }
        } else if(edge_type == EdgeType::PRIMITIVE_CLOCK_LAUNCH) {
            color = CLOCK_LAUNCH_EDGE_COLOR;
            os << "\\n" << delay_calc_.max_edge_delay(tg_, edge) << " (tcq)";
        } else {
            os << "\\n" << delay_calc_.max_edge_delay(tg_, edge);
        }

        if(tg_.edge_disabled(edge)) {
            os << "\\n" << "(disabled)";
        }

        os << "\""; //end label
        if(tg_.edge_disabled(edge)) {
            os << " style=\"dashed\"";
            os << " color=\"" << DISABLED_EDGE_COLOR << "\"";
            os << " fontcolor=\"" << DISABLED_EDGE_COLOR << "\"";
        } else if (!color.empty()) {
            os << " color=\"" + color + "\"";
        }
        os << "]";
        os << ";" <<std::endl;
    }
}

template<class DelayCalc>
void GraphvizDotWriter<DelayCalc>::tag_domain_from_to(std::ostream& os, const TimingTag& tag) {
    if(!tag.launch_clock_domain()) {
        os << "*";
    } else {
        os << tag.launch_clock_domain();
    }
    os << " to ";
    if(!tag.capture_clock_domain()) {
        os << "*";
    } else {
        os << tag.capture_clock_domain();
    }
}

template<class DelayCalc>
std::string GraphvizDotWriter<DelayCalc>::node_name(const NodeId node) {
    return "node" + std::to_string(size_t(node));
}

} //namespace
