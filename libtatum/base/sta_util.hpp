#pragma once
#include <set>
#include <memory>
#include <iomanip>
#include <iostream>
#include <fstream>

#include "timing_analyzers.hpp"
#include "TimingGraph.hpp"
#include "TimingTags.hpp"
#include "FixedDelayCalculator.hpp"

namespace tatum {

float time_sec(struct timespec start, struct timespec end);

void print_histogram(const std::vector<float>& values, int nbuckets);

void print_level_histogram(const TimingGraph& tg, int nbuckets);
void print_node_fanin_histogram(const TimingGraph& tg, int nbuckets);
void print_node_fanout_histogram(const TimingGraph& tg, int nbuckets);

void print_timing_graph(std::shared_ptr<const TimingGraph> tg);
void print_levelization(std::shared_ptr<const TimingGraph> tg);

void dump_level_times(std::string fname, const TimingGraph& timing_graph, std::map<std::string,float> serial_prof_data, std::map<std::string,float> parallel_prof_data);

/*
 * Templated function implementations
 */

template<class DelayCalc=FixedDelayCalculator>
void write_dot_file_setup(std::string filename, 
                          const TimingGraph& tg, 
                          std::shared_ptr<const TimingAnalyzer> analyzer = std::shared_ptr<const TimingAnalyzer>(), 
                          std::shared_ptr<DelayCalc> delay_calc = std::shared_ptr<DelayCalc>()) {

    if(tg.nodes().size() > 1000) {
        std::cout << "Skipping setup dot file due to large timing graph size\n"; 
        return;
    }

    std::ofstream os(filename);

    auto setup_analyzer = std::dynamic_pointer_cast<const SetupTimingAnalyzer>(analyzer);
    if(!setup_analyzer) return;

    //Write out a dot file of the timing graph
    os << "digraph G {" <<std::endl;
    os << "\tnode[shape=record]" <<std::endl;

    for(const NodeId inode : tg.nodes()) {
        os << "\tnode" << size_t(inode);
        os << "[label=\"";
        os << "{" << inode << " (" << tg.node_type(inode) << ")";
        for(const TimingTag& tag : setup_analyzer->setup_tags(inode)) {
            os << " | {";
            os << tag.type() << "\\n";
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
            if(tag.type() == TagType::CLOCK_LAUNCH || tag.type() == TagType::CLOCK_CAPTURE || tag.type() == TagType::DATA_ARRIVAL) {
                os << " from ";
            } else {
                os << " for ";
            }
            os << tag.launch_node();
            os << "\\n";
            os << " time: " << tag.time().value();
            os << "}";
        }
        os << "}\"]";
        os <<std::endl;
    }

    //Force drawing to be levelized
    for(const LevelId ilevel : tg.levels()) {
        os << "\t{rank = same;";

        for(NodeId node_id : tg.level_nodes(ilevel)) {
            os << " node" << size_t(node_id) <<";";
        }
        os << "}" <<std::endl;
    }

    //Add edges with delays annoated
    for(const LevelId ilevel : tg.levels()) {
        for(NodeId node_id : tg.level_nodes(ilevel)) {
            for(EdgeId edge_id : tg.node_out_edges(node_id)) {

                NodeId sink_node_id = tg.edge_sink_node(edge_id);

                os << "\tnode" << size_t(node_id) << " -> node" << size_t(sink_node_id);
                if(delay_calc) {
                    if(tg.node_type(node_id) == NodeType::CPIN && tg.node_type(sink_node_id) == NodeType::SINK) {
                        os << " [ label=\"" << -delay_calc->setup_time(tg, edge_id) << " (-tsu)\" ]";
                    } else if(tg.node_type(node_id) == NodeType::CPIN && tg.node_type(sink_node_id) == NodeType::SOURCE) {
                        os << " [ label=\"" << delay_calc->max_edge_delay(tg, edge_id) << " (tcq)\" ]";
                    } else {
                        os << " [ label=\"" << delay_calc->max_edge_delay(tg, edge_id) << "\" ]";
                    }
                }
                os << ";" <<std::endl;
            }
        }
    }

    os << "}" <<std::endl;
}

template<class DelayCalc=const FixedDelayCalculator>
void write_dot_file_hold(std::string filename, 
                         const TimingGraph& tg, 
                         std::shared_ptr<const TimingAnalyzer> analyzer = std::shared_ptr<const TimingAnalyzer>(), 
                         std::shared_ptr<DelayCalc> delay_calc = std::shared_ptr<DelayCalc>()) {
    if(tg.nodes().size() > 1000) {
        std::cout << "Skipping hold dot file due to large timing graph size\n"; 
        return;
    }

    std::ofstream os(filename);

    auto hold_analyzer = std::dynamic_pointer_cast<const HoldTimingAnalyzer>(analyzer);
    if(!hold_analyzer) return;

    //Write out a dot file of the timing graph
    os << "digraph G {" <<std::endl;
    os << "\tnode[shape=record]" <<std::endl;

    //Declare nodes and annotate tags
    for(const NodeId inode : tg.nodes()) {
        os << "\tnode" << size_t(inode);
        os << "[label=\"";
        os << "{" << inode << " (" << tg.node_type(inode) << ")";
        for(const TimingTag& tag : hold_analyzer->hold_tags(inode)) {
            os << " | {";
            os << tag.type() << "\\n";
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
            if(tag.type() == TagType::CLOCK_LAUNCH || tag.type() == TagType::CLOCK_CAPTURE || tag.type() == TagType::DATA_ARRIVAL) {
                os << " from ";
            } else {
                os << " for ";
            }
            os << tag.launch_node();
            os << "\\n";
            os << " time: " << tag.time().value();
            os << "}";
        }
        os << "}\"]";
        os <<std::endl;
    }

    //Force drawing to be levelized
    for(const LevelId ilevel : tg.levels()) {
        os << "\t{rank = same;";

        for(NodeId node_id : tg.level_nodes(ilevel)) {
            os << " node" << size_t(node_id) <<";";
        }
        os << "}" <<std::endl;
    }

    //Add edges with delays annoated
    for(const LevelId ilevel : tg.levels()) {
        for(NodeId node_id : tg.level_nodes(ilevel)) {
            for(EdgeId edge_id : tg.node_out_edges(node_id)) {

                NodeId sink_node_id = tg.edge_sink_node(edge_id);

                os << "\tnode" << size_t(node_id) << " -> node" << size_t(sink_node_id);
                if(delay_calc) {
                    if(tg.node_type(node_id) == NodeType::CPIN && tg.node_type(sink_node_id) == NodeType::SINK) {
                        os << " [ label=\"" << delay_calc->hold_time(tg, edge_id) << " (thld)\" ]";
                    } else if(tg.node_type(node_id) == NodeType::CPIN && tg.node_type(sink_node_id) == NodeType::SOURCE) {
                        os << " [ label=\"" << delay_calc->min_edge_delay(tg, edge_id) << " (tcq)\" ]";
                    } else {
                        os << " [ label=\"" << delay_calc->min_edge_delay(tg, edge_id) << "\" ]";
                    }
                }
                os << ";" <<std::endl;
            }
        }
    }

    os << "}" <<std::endl;
}

void print_setup_tags_histogram(const TimingGraph& tg, const SetupTimingAnalyzer& analyzer);
void print_hold_tags_histogram(const TimingGraph& tg, const HoldTimingAnalyzer& analyzer);

void print_setup_tags(const TimingGraph& tg, const SetupTimingAnalyzer& analyzer);
void print_hold_tags(const TimingGraph& tg, const HoldTimingAnalyzer& analyzer);

} //namepsace
