#ifndef TATUM_TIMING_PATHS_HPP
#define TATUM_TIMING_PATHS_HPP
#include <vector>

#include "tatum/timing_analyzers.hpp"
#include "tatum/TimingConstraintsFwd.hpp"
#include "tatum/TimingGraphFwd.hpp"
#include "tatum/report/TimingPath.hpp"

namespace tatum {

std::vector<TimingPathInfo> find_critical_paths(const TimingGraph& timing_graph, 
                                                const TimingConstraints& timing_constraints, 
                                                const SetupTimingAnalyzer& setup_analyzer);

TimingPath trace_setup_path(const TimingGraph& timing_graph, 
                            const SetupTimingAnalyzer& setup_analyzer,
                            const DomainId launch_domain,
                            const DomainId capture_domain,
                            const NodeId sink_node);

TimingPath trace_hold_path(const TimingGraph& timing_graph, 
                           const HoldTimingAnalyzer& hold_analyzer,
                           const DomainId launch_domain,
                           const DomainId capture_domain,
                           const NodeId sink_node);

} //namespace
#endif
