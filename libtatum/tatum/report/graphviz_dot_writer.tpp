#include "tatum/TimingGraph.hpp"
#include "tatum/base/sta_util.hpp"
#include <iostream>
#include <sstream>

namespace tatum {

constexpr size_t MAX_DOT_GRAPH_NODES = 1000;

inline std::string print_tag_domain_from_to(const TimingTag& tag) {
    std::stringstream ss;
    if(!tag.launch_clock_domain()) {
        ss << "*";
    } else {
        ss << tag.launch_clock_domain();
    }
    ss << " to ";
    if(!tag.capture_clock_domain()) {
        ss << "*";
    } else {
        ss << tag.capture_clock_domain();
    }
    return ss.str();
}

template<class DelayCalc=FixedDelayCalculator>
void write_dot_file_setup(std::string filename,
                          const TimingGraph& tg,
                          const DelayCalc& delay_calc = DelayCalc(),
                          const SetupTimingAnalyzer& analyzer = SetupTimingAnalyzer(),
                          std::vector<NodeId> nodes = std::vector<NodeId>()) {

    if(tg.nodes().size() > MAX_DOT_GRAPH_NODES && nodes.empty()) {
        std::cout << "Skipping setup dot file due to large timing graph size\n";
        return;
    }

    std::ofstream os(filename);

    //Write out a dot file of the timing graph
    os << "digraph G {" <<std::endl;
    os << "\tnode[shape=record]" <<std::endl;

    if(nodes.empty()) {
        //All nodes
        auto tg_nodes = tg.nodes();
        std::copy(tg_nodes.begin(), tg_nodes.end(), std::back_inserter(nodes));
    }

    for(const NodeId inode : nodes) {
        os << "\tnode" << size_t(inode);
        os << "[label=\"";
        os << "{" << inode << " (" << tg.node_type(inode) << ")";
        for(const auto& tags : {analyzer.setup_tags(inode), analyzer.setup_slacks(inode)}) {
            for(const TimingTag& tag : tags) {
                os << " | {";
                os << tag.type() << "\\n";
                os << print_tag_domain_from_to(tag);
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
        os <<std::endl;
    }

    //Force drawing to be levelized
    for(const LevelId ilevel : tg.levels()) {
        os << "\t{rank = same;";

        for(NodeId node_id : tg.level_nodes(ilevel)) {
            if(std::binary_search(nodes.begin(), nodes.end(), node_id)) {
                os << " node" << size_t(node_id) <<";";
            }
        }
        os << "}" <<std::endl;
    }

    //Add edges with delays annoated
    for(EdgeId edge_id : tg.edges()) {
        NodeId node_id = tg.edge_src_node(edge_id);
        NodeId sink_node_id = tg.edge_sink_node(edge_id);

        if(std::binary_search(nodes.begin(), nodes.end(), node_id)
           && std::binary_search(nodes.begin(), nodes.end(), sink_node_id)) {
            //Only draw edges to nodes in the set of nodes being printed


            std::string color = "";
            os << "\tnode" << size_t(node_id) << " -> node" << size_t(sink_node_id);
            os << " [ label=\"" << edge_id;
            if(tg.node_type(node_id) == NodeType::CPIN && tg.node_type(sink_node_id) == NodeType::SINK) {
                color = "#c45403"; //Orange-red
                os << "\\n"<< -delay_calc.setup_time(tg, edge_id) << " (-tsu)";
            } else if(tg.node_type(node_id) == NodeType::CPIN && tg.node_type(sink_node_id) == NodeType::SOURCE) {
                color = "#10c403"; //Green
                os << "\\n" << delay_calc.max_edge_delay(tg, edge_id) << " (tcq)";
            } else {
                os << "\\n" << delay_calc.max_edge_delay(tg, edge_id);
            }
            auto slacks = analyzer.setup_slacks(edge_id);
            for(const auto& tag : slacks) {
                os << "\\n" << " (slack " << print_tag_domain_from_to(tag) << ": " << tag.time().value() << ")";
            }
            if(tg.edge_disabled(edge_id)) {
                os << "\\n" << "(disabled)";
            }
            os << "\""; //end label
            if(tg.edge_disabled(edge_id)) {
                os << " style=\"dashed\"";
                os << " color=\"#aaaaaa\""; //grey
                os << " fontcolor=\"#aaaaaa\""; //grey
            } else if (!color.empty()) {
                os << " color=\"" + color + "\"";
            }
            os << "]";
            os << ";" <<std::endl;
        }
    }

    os << "}" <<std::endl;
}

template<class DelayCalc=FixedDelayCalculator>
void write_dot_file_hold(std::string filename,
                         const TimingGraph& tg,
                         const DelayCalc& delay_calc = DelayCalc(),
                         const HoldTimingAnalyzer& analyzer = TimingAnalyzer(),
                         std::vector<NodeId> nodes = std::vector<NodeId>()) {

    if(tg.nodes().size() > MAX_DOT_GRAPH_NODES && nodes.empty()) {
        std::cout << "Skipping hold dot file due to large timing graph size\n";
        return;
    }

    std::ofstream os(filename);

    //Write out a dot file of the timing graph
    os << "digraph G {" <<std::endl;
    os << "\tnode[shape=record]" <<std::endl;

    if(nodes.empty()) {
        auto tg_nodes = tg.nodes();
        std::copy(tg_nodes.begin(), tg_nodes.end(), std::back_inserter(nodes));
    }

    for(const NodeId inode : nodes) {
        os << "\tnode" << size_t(inode);
        os << "[label=\"";
        os << "{" << inode << " (" << tg.node_type(inode) << ")";
        for(const auto& tags : {analyzer.hold_tags(inode), analyzer.hold_slacks(inode)}) {
            for(const TimingTag& tag : tags) {
                os << " | {";
                os << tag.type() << "\\n";
                os << print_tag_domain_from_to(tag);
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
                os << " time: " << tag.time().value();
                os << "}";
            }
        }
        os << "}\"]";
        os <<std::endl;
    }

    //Force drawing to be levelized
    for(const LevelId ilevel : tg.levels()) {
        os << "\t{rank = same;";

        for(NodeId node_id : tg.level_nodes(ilevel)) {
            if(std::binary_search(nodes.begin(), nodes.end(), node_id)) {
                os << " node" << size_t(node_id) <<";";
            }
        }
        os << "}" <<std::endl;
    }

    //Add edges with delays annoated
    for(EdgeId edge_id : tg.edges()) {
        NodeId node_id = tg.edge_src_node(edge_id);
        NodeId sink_node_id = tg.edge_sink_node(edge_id);

        if(std::binary_search(nodes.begin(), nodes.end(), node_id)
           && std::binary_search(nodes.begin(), nodes.end(), sink_node_id)) {
            //Only draw edges to nodes in the set of nodes being printed

            os << "\tnode" << size_t(node_id) << " -> node" << size_t(sink_node_id);
            os << " [ label=\"" << edge_id;
            if(tg.node_type(node_id) == NodeType::CPIN && tg.node_type(sink_node_id) == NodeType::SINK) {
                os << "\\n" << delay_calc.hold_time(tg, edge_id) << " (thld)";
            } else if(tg.node_type(node_id) == NodeType::CPIN && tg.node_type(sink_node_id) == NodeType::SOURCE) {
                os << "\\n" << delay_calc.min_edge_delay(tg, edge_id) << " (tcq)";
            } else {
                os << "\\n" << delay_calc.min_edge_delay(tg, edge_id);
            }
            auto slacks = analyzer.hold_slacks(edge_id);
            for(const auto& tag : slacks) {
                os << "\\n" << " (slack " << print_tag_domain_from_to(tag) << ": " << tag.time().value() << ")";
            }
            if(tg.edge_disabled(edge_id)) {
                os << "\\n" << "(disabled)";
            }
            os << "\""; //end label
            if(tg.edge_disabled(edge_id)) {
                os << " style=\"dashed\"";
                os << " color=\"#aaaaaa\""; //grey
                os << " fontcolor=\"#aaaaaa\""; //grey
            }
            os << "]";
            os << ";" <<std::endl;
        }
    }

    os << "}" <<std::endl;
}

} //namespace
