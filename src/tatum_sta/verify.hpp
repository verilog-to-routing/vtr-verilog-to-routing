#include <iostream>
#include <iomanip>
#include <memory>
#include <set>

#include "TimingGraph.hpp"
#include "TimingTags.hpp"
#include "timing_analyzer_interfaces.hpp"

#include "vpr_timing_graph_common.hpp"

int verify_analyzer(const TimingGraph& tg, const SetupTimingAnalyzer& analyzer, const VprArrReqTimes& expected_arr_req_times, const std::set<NodeId>& const_gen_fanout_nodes, const std::set<NodeId>& clock_gen_fanout_nodes);
