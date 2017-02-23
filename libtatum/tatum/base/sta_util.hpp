#pragma once
#include <set>
#include <memory>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <vector>
#include <limits>

#include "tatum/timing_analyzers.hpp"
#include "tatum/TimingGraph.hpp"
#include "tatum/TimingConstraints.hpp"
#include "tatum/tags/TimingTags.hpp"
#include "tatum/delay_calc/FixedDelayCalculator.hpp"

namespace tatum {

float time_sec(struct timespec start, struct timespec end);

void print_histogram(const std::vector<float>& values, int nbuckets);

void print_level_histogram(const TimingGraph& tg, int nbuckets);
void print_node_fanin_histogram(const TimingGraph& tg, int nbuckets);
void print_node_fanout_histogram(const TimingGraph& tg, int nbuckets);

void print_timing_graph(std::shared_ptr<const TimingGraph> tg);
void print_levelization(std::shared_ptr<const TimingGraph> tg);

void dump_level_times(std::string fname, const TimingGraph& timing_graph, std::map<std::string,float> serial_prof_data, std::map<std::string,float> parallel_prof_data);

std::vector<NodeId> find_related_nodes(const TimingGraph& tg, const std::vector<NodeId> through_nodes, size_t max_depth=std::numeric_limits<size_t>::max());
void find_transitive_fanout_nodes(const TimingGraph& tg, std::vector<NodeId>& nodes, const NodeId node, size_t max_depth=std::numeric_limits<size_t>::max(), size_t depth=0);
void find_transitive_fanin_nodes(const TimingGraph& tg, std::vector<NodeId>& nodes, const NodeId node, size_t max_depth=std::numeric_limits<size_t>::max(), size_t depth=0);

std::string print_tag_domain_from_to(const TimingTag& tag);

/*
 * Templated function implementations
 */

template<class DelayCalc=FixedDelayCalculator>
void write_dot_file_setup(std::string filename,
                          const TimingGraph& tg,
                          const DelayCalc& delay_calc = DelayCalc(),
                          const SetupTimingAnalyzer& analyzer = SetupTimingAnalyzer(),
                          std::vector<NodeId> nodes = std::vector<NodeId>()) {

    if(tg.nodes().size() > 1000 && nodes.empty()) {
        std::cout << "Skipping setup dot file due to large timing graph size\n";
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

    if(tg.nodes().size() > 1000 && nodes.empty()) {
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

void print_setup_tags_histogram(const TimingGraph& tg, const SetupTimingAnalyzer& analyzer);
void print_hold_tags_histogram(const TimingGraph& tg, const HoldTimingAnalyzer& analyzer);

void print_setup_tags(const TimingGraph& tg, const SetupTimingAnalyzer& analyzer);
void print_hold_tags(const TimingGraph& tg, const HoldTimingAnalyzer& analyzer);

} //namepsace
