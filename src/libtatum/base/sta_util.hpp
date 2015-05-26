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

/*
 * Templated function implementations
 */

template<class Analyzer>
void print_timing_tags_histogram(const TimingGraph& tg, const std::shared_ptr<Analyzer> analyzer) {
    const int int_width = 8;
    const int flt_width = 2;

    auto totaler = [](int total, const std::map<int,int>::value_type& kv) {
        return total + kv.second;
    };

    std::cout << "Node Data Tag Count Histogram:" << std::endl;
    std::map<int,int> data_tag_cnts;
    for(NodeId i = 0; i < tg.num_nodes(); i++) {
        data_tag_cnts[analyzer->setup_data_tags(i).num_tags()]++;
    }

    int total_data_tags = std::accumulate(data_tag_cnts.begin(), data_tag_cnts.end(), 0, totaler);
    for(const auto& kv : data_tag_cnts) {
        std::cout << "\t" << kv.first << " Tags: " << std::setw(int_width) << kv.second << " (" << std::setw(flt_width) << std::fixed << (float) kv.second / total_data_tags << ")" << std::endl;
    }

    std::cout << "Node clock Tag Count Histogram:" << std::endl;
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
void print_timing_tags(const TimingGraph& tg, const std::shared_ptr<Analyzer> analyzer) {
    std::cout << std::endl;
    std::cout << std::scientific;
    for(int level_idx = 0; level_idx < tg.num_levels(); level_idx++) {
        std::cout << "Level: " << level_idx << std::endl;
        for(NodeId node_id : tg.level(level_idx)) {
            std::cout << "Node: " << node_id << " (" << tg.node_type(node_id) << ")" << std::endl;;
            for(const TimingTag& tag : analyzer->setup_data_tags(node_id)) {
                std::cout << "\tData: ";
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
