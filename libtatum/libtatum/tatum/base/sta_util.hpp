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




void print_setup_tags_histogram(const TimingGraph& tg, const SetupTimingAnalyzer& analyzer);
void print_hold_tags_histogram(const TimingGraph& tg, const HoldTimingAnalyzer& analyzer);

void print_setup_tags(const TimingGraph& tg, const SetupTimingAnalyzer& analyzer);
void print_hold_tags(const TimingGraph& tg, const HoldTimingAnalyzer& analyzer);

} //namepsace
