#include <iostream>
#include <iomanip>
#include <memory>
#include <set>

#include "TimingGraph.hpp"
#include "TimingTags.hpp"
#include "timing_analyzers.hpp"

#include "vpr_timing_graph_common.hpp"

int verify_analyzer(const tatum::TimingGraph& tg, const tatum::SetupTimingAnalyzer& analyzer, const VprArrReqTimes& expected_arr_req_times, const std::set<tatum::NodeId>& const_gen_fanout_nodes, const std::set<tatum::NodeId>& clock_gen_fanout_nodes);
