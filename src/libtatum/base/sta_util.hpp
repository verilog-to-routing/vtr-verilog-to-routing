#pragma once
#include <set>
#include <memory>

#include "TimingGraph.hpp"
#include "TimingAnalyzer.hpp"

float time_sec(struct timespec start, struct timespec end);

void print_histogram(const std::vector<float>& values, int nbuckets);

float relative_error(float A, float B);
void print_level_histogram(const TimingGraph& tg, int nbuckets);
void print_node_fanin_histogram(const TimingGraph& tg, int nbuckets);
void print_node_fanout_histogram(const TimingGraph& tg, int nbuckets);

void print_timing_graph(const TimingGraph& tg);
void print_levelization(const TimingGraph& tg);

std::set<NodeId> identify_constant_gen_fanout(const TimingGraph& tg);
std::set<NodeId> identify_clock_gen_fanout(const TimingGraph& tg);

void add_ff_clock_to_source_sink_edges(TimingGraph& timing_graph, const std::vector<BlockId> node_logical_blocks, std::vector<float>& edge_delays);
void dump_level_times(std::string fname, const TimingGraph& timing_graph, std::map<std::string,float> serial_prof_data, std::map<std::string,float> parallel_prof_data);

/*
 * Templated function implementations
 */

template<class Analyzer>
void write_dot_file_setup(std::ostream& os, const TimingGraph& tg, const std::shared_ptr<Analyzer>& analyzer) {
    //Write out a dot file of the timing graph
    os << "digraph G {" <<std::endl;
    os << "\tnode[shape=record]" <<std::endl;

    for(int inode = 0; inode < tg.num_nodes(); inode++) {
        os << "\tnode" << inode;
        os << "[label=\"";
        os << "{#" << inode << " (" << tg.node_type(inode) << ")";
        const TimingTags& data_tags = analyzer->setup_data_tags(inode);
        if(data_tags.num_tags() > 0) {
            for(const TimingTag& tag : data_tags) {
                os << " | {";
                os << "DATA - clk: " << tag.clock_domain();
                os << " launch: " << tag.launch_node();
                os << "\\n";
                os << " arr: " << tag.arr_time().value();
                os << " req: " << tag.req_time().value();
                os << "}";
            }
        }
        const TimingTags& clock_tags = analyzer->setup_clock_tags(inode);
        if(clock_tags.num_tags() > 0) {
            for(const TimingTag& tag : clock_tags) {
                os << " | {";
                os << "CLOCK - clk: " << tag.clock_domain();
                os << " launch: " << tag.launch_node();
                os << "\\n";
                os << " arr: " << tag.arr_time().value();
                os << " req: " << tag.req_time().value();
                os << "}";
            }
        }
        os << "}\"]";
        os <<std::endl;
    }

    //Force drawing to be levelized
    for(int ilevel = 0; ilevel < tg.num_levels(); ilevel++) {
        os << "\t{rank = same;";

        for(NodeId node_id : tg.level(ilevel)) {
            os << " node" << node_id <<";";
        }
        os << "}" <<std::endl;
    }

    //Add edges with delays annoated
    auto delay_calc = analyzer->delay_calculator();

    for(int ilevel = 0; ilevel < tg.num_levels(); ilevel++) {
        for(NodeId node_id : tg.level(ilevel)) {
            for(int edge_idx = 0; edge_idx < tg.num_node_out_edges(node_id); edge_idx++) {
                EdgeId edge_id = tg.node_out_edge(node_id, edge_idx);

                NodeId sink_node_id = tg.edge_sink_node(edge_id);

                os << "\tnode" << node_id << " -> node" << sink_node_id;
                os << " [ label=\"" << delay_calc.max_edge_delay(tg, edge_id) << "\" ]";
                os << ";" <<std::endl;
            }
        }
    }

    os << "}" <<std::endl;
}

template<class Analyzer>
void write_dot_file_hold(std::ostream& os, const TimingGraph& tg, const std::shared_ptr<Analyzer> analyzer) {
    //Write out a dot file of the timing graph
    os << "digraph G {" <<std::endl;
    os << "\tnode[shape=record]" <<std::endl;

    //Declare nodes and annotate tags
    for(int inode = 0; inode < tg.num_nodes(); inode++) {
        os << "\tnode" << inode;
        os << "[label=\"";
        os << "{#" << inode << " (" << tg.node_type(inode) << ")";
        const TimingTags& data_tags = analyzer->hold_data_tags(inode);
        if(data_tags.num_tags() > 0) {
            for(const TimingTag& tag : data_tags) {
                os << " | {";
                os << "DATA - clk: " << tag.clock_domain();
                os << " launch: " << tag.launch_node();
                os << "\\n";
                os << " arr: " << tag.arr_time().value();
                os << " req: " << tag.req_time().value();
                os << "}";
            }
        }
        const TimingTags& clock_tags = analyzer->hold_clock_tags(inode);
        if(clock_tags.num_tags() > 0) {
            for(const TimingTag& tag : clock_tags) {
                os << " | {";
                os << "CLOCK - clk: " << tag.clock_domain();
                os << " launch: " << tag.launch_node();
                os << "\\n";
                os << " arr: " << tag.arr_time().value();
                os << " req: " << tag.req_time().value();
                os << "}";
            }
        }
        os << "}\"]";
        os <<std::endl;
    }

    //Force drawing to be levelized
    for(int ilevel = 0; ilevel < tg.num_levels(); ilevel++) {
        os << "\t{rank = same;";

        for(NodeId node_id : tg.level(ilevel)) {
            os << " node" << node_id <<";";
        }
        os << "}" <<std::endl;
    }

    //Add edges with delays annoated
    auto delay_calc = analyzer->delay_calculator();

    for(int ilevel = 0; ilevel < tg.num_levels(); ilevel++) {
        for(NodeId node_id : tg.level(ilevel)) {
            for(int edge_idx = 0; edge_idx < tg.num_node_out_edges(node_id); edge_idx++) {
                EdgeId edge_id = tg.node_out_edge(node_id, edge_idx);

                NodeId sink_node_id = tg.edge_sink_node(edge_id);

                os << "\tnode" << node_id << " -> node" << sink_node_id;
                os << " [ label=\"" << delay_calc.min_edge_delay(tg, edge_id) << "\" ]";
                os << ";" <<std::endl;
            }
        }
    }

    os << "}" <<std::endl;
}

template<class Analyzer>
void print_setup_tags_histogram(const TimingGraph& tg, const std::shared_ptr<Analyzer> analyzer) {
    const int int_width = 8;
    const int flt_width = 2;

    auto totaler = [](int total, const std::map<int,int>::value_type& kv) {
        return total + kv.second;
    };

    std::cout << "Node Data Setup Tag Count Histogram:" << std::endl;
    std::map<int,int> data_tag_cnts;
    for(NodeId i = 0; i < tg.num_nodes(); i++) {
        data_tag_cnts[analyzer->setup_data_tags(i).num_tags()]++;
    }

    int total_data_tags = std::accumulate(data_tag_cnts.begin(), data_tag_cnts.end(), 0, totaler);
    for(const auto& kv : data_tag_cnts) {
        std::cout << "\t" << kv.first << " Tags: " << std::setw(int_width) << kv.second << " (" << std::setw(flt_width) << std::fixed << (float) kv.second / total_data_tags << ")" << std::endl;
    }

    std::cout << "Node Clock Setup Tag Count Histogram:" << std::endl;
    std::map<int,int> clock_tag_cnts;
    for(NodeId i = 0; i < tg.num_nodes(); i++) {
        clock_tag_cnts[analyzer->setup_clock_tags(i).num_tags()]++;
    }

    int total_clock_tags = std::accumulate(clock_tag_cnts.begin(), clock_tag_cnts.end(), 0, totaler);
    for(const auto& kv : clock_tag_cnts) {
        std::cout << "\t" << kv.first << " Tags: " << std::setw(int_width) << kv.second << " (" << std::setw(flt_width) << std::fixed << (float) kv.second / total_clock_tags << ")" << std::endl;
    }
}

template<class Analyzer>
void print_hold_tags_histogram(const TimingGraph& tg, const std::shared_ptr<Analyzer> analyzer) {
    const int int_width = 8;
    const int flt_width = 2;

    auto totaler = [](int total, const std::map<int,int>::value_type& kv) {
        return total + kv.second;
    };

    std::cout << "Node Data Hold Tag Count Histogram:" << std::endl;
    std::map<int,int> data_tag_cnts;
    for(NodeId i = 0; i < tg.num_nodes(); i++) {
        data_tag_cnts[analyzer->hold_data_tags(i).num_tags()]++;
    }

    int total_data_tags = std::accumulate(data_tag_cnts.begin(), data_tag_cnts.end(), 0, totaler);
    for(const auto& kv : data_tag_cnts) {
        std::cout << "\t" << kv.first << " Tags: " << std::setw(int_width) << kv.second << " (" << std::setw(flt_width) << std::fixed << (float) kv.second / total_data_tags << ")" << std::endl;
    }

    std::cout << "Node Clock Hold Tag Count Histogram:" << std::endl;
    std::map<int,int> clock_tag_cnts;
    for(NodeId i = 0; i < tg.num_nodes(); i++) {
        clock_tag_cnts[analyzer->hold_clock_tags(i).num_tags()]++;
    }

    int total_clock_tags = std::accumulate(clock_tag_cnts.begin(), clock_tag_cnts.end(), 0, totaler);
    for(const auto& kv : clock_tag_cnts) {
        std::cout << "\t" << kv.first << " Tags: " << std::setw(int_width) << kv.second << " (" << std::setw(flt_width) << std::fixed << (float) kv.second / total_clock_tags << ")" << std::endl;
    }
}

template<class Analyzer>
void print_setup_tags(const TimingGraph& tg, const std::shared_ptr<Analyzer> analyzer) {
    std::cout << std::endl;
    std::cout << "Setup Tags:" << std::endl;
    std::cout << std::scientific;
    for(int level_idx = 0; level_idx < tg.num_levels(); level_idx++) {
        std::cout << "Level: " << level_idx << std::endl;
        for(NodeId node_id : tg.level(level_idx)) {
            std::cout << "Node: " << node_id << " (" << tg.node_type(node_id) << ")" << std::endl;;
            for(const TimingTag& tag : analyzer->setup_data_tags(node_id)) {
                std::cout << "\tData : ";
                std::cout << "  clk: " << tag.clock_domain();
                std::cout << "  Arr: " << tag.arr_time().value();
                std::cout << "  Req: " << tag.req_time().value();
                std::cout << std::endl;
            }
            for(const TimingTag& tag : analyzer->setup_clock_tags(node_id)) {
                std::cout << "\tClock: ";
                std::cout << "  clk: " << tag.clock_domain();
                std::cout << "  Arr: " << tag.arr_time().value();
                std::cout << "  Req: " << tag.req_time().value();
                std::cout << std::endl;
            }
        }
    }
    std::cout << std::endl;
}

template<class Analyzer>
void print_hold_tags(const TimingGraph& tg, const std::shared_ptr<Analyzer> analyzer) {
    std::cout << std::endl;
    std::cout << "Hold Tags:" << std::endl;
    std::cout << std::scientific;
    for(int level_idx = 0; level_idx < tg.num_levels(); level_idx++) {
        std::cout << "Level: " << level_idx << std::endl;
        for(NodeId node_id : tg.level(level_idx)) {
            std::cout << "Node: " << node_id << " (" << tg.node_type(node_id) << ")" << std::endl;;
            for(const TimingTag& tag : analyzer->hold_data_tags(node_id)) {
                std::cout << "\tData : ";
                std::cout << "  clk: " << tag.clock_domain();
                std::cout << "  Arr: " << tag.arr_time().value();
                std::cout << "  Req: " << tag.req_time().value();
                std::cout << std::endl;
            }
            for(const TimingTag& tag : analyzer->hold_clock_tags(node_id)) {
                std::cout << "\tClock: ";
                std::cout << "  clk: " << tag.clock_domain();
                std::cout << "  Arr: " << tag.arr_time().value();
                std::cout << "  Req: " << tag.req_time().value();
                std::cout << std::endl;
            }
        }
    }
    std::cout << std::endl;
}

