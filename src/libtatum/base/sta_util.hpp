#pragma once

#include "TimingGraph.hpp"
#include "SerialTimingAnalyzer.hpp"

float time_sec(struct timespec start, struct timespec end);

void print_histogram(const std::vector<float>& values, int nbuckets);

float relative_error(float A, float B);
void print_level_histogram(const TimingGraph& tg, int nbuckets);
void print_node_fanin_histogram(const TimingGraph& tg, int nbuckets);
void print_node_fanout_histogram(const TimingGraph& tg, int nbuckets);
void print_timing_graph(const TimingGraph& tg);

void print_timing_tags_histogram(const TimingGraph& tg, SerialTimingAnalyzer& analyzer, int nbuckets);
