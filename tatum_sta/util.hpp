#pragma once
#include <set>
#include <vector>

#include "timing_graph_fwd.hpp"
#include "timing_constraints_fwd.hpp"
#include "vpr_timing_graph_common.hpp"

float relative_error(float A, float B);

std::set<tatum::NodeId> identify_constant_gen_fanout(const tatum::TimingGraph& tg);
std::set<tatum::NodeId> identify_clock_gen_fanout(const tatum::TimingGraph& tg);

void rebuild_timing_graph(tatum::TimingGraph& tg, tatum::TimingConstraints& tc, const VprFfInfo& ff_info, std::vector<float>& edge_delays, VprArrReqTimes& arr_req_times);

void add_ff_clock_to_source_sink_edges(tatum::TimingGraph& timing_graph, const VprFfInfo& ff_info, std::vector<float>& edge_delays);
