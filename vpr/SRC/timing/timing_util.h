#ifndef VPR_TIMING_UTIL_H
#define VPR_TIMING_UTIL_H
#include <vector>

#include "timing_analyzers.hpp"
#include "TimingConstraints.hpp"
#include "histogram.h"
#include "PlacementDelayCalculator.hpp"
#include "timing_info_fwd.h"
#include "PathInfo.h"
#include "DomainPair.h"

double sec_to_nanosec(double seconds); 

double sec_to_mhz(double seconds);

//Returns the path delay of the longest critical timing path (i.e. across all domains)
PathInfo find_longest_critical_path_delay(const tatum::TimingConstraints& constraints, const tatum::SetupTimingAnalyzer& setup_analyzer);

//Returns the path delay of the least-slack critical timing path (i.e. across all domains)
PathInfo find_least_slack_critical_path_delay(const tatum::TimingConstraints& constraints, const tatum::SetupTimingAnalyzer& setup_analyzer);

//Returns the critical path delay between all active launch/capture clock domain pairs
std::vector<PathInfo> find_critical_path_delays(const tatum::TimingConstraints& constraints, const tatum::SetupTimingAnalyzer& setup_analyzer);

//Returns the total negative slack (setup) at all timing end-points and clock domain pairs
float find_setup_total_negative_slack(const tatum::SetupTimingAnalyzer& setup_analyzer);

//Returns the worst negative slack (setup) across all timing end-points and clock domain pairs
float find_setup_worst_negative_slack(const tatum::SetupTimingAnalyzer& setup_analyzer);

//Returns the slack at a particular node for the specified clock domains (if found), otherwise NAN
float find_node_setup_slack(const tatum::SetupTimingAnalyzer& setup_analyzer, tatum::NodeId node, tatum::DomainId launch_domain, tatum::DomainId capture_domain);

//Returns a slack histogram
std::vector<HistogramBucket> create_setup_slack_histogram(const tatum::SetupTimingAnalyzer& setup_analyzer, size_t num_bins = 10);

//Print a useful summary of timing information
void print_setup_timing_summary(const tatum::TimingConstraints& constraints, const tatum::SetupTimingAnalyzer& setup_analyzer);

//Returns the a map of domain's and thier clock fanout (i.e. logical outputs at which the clock captures)
std::map<tatum::DomainId,size_t> count_clock_fanouts(const tatum::TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& setup_analyzer);

//Prints the atom net delays to a file
void dump_atom_net_delays_tatum(std::string filename, const PlacementDelayCalculator& dc);

/*
 * Tag utilities
 */
//For comparing the values of two timing tags
struct TimingTagValueComp {
    bool operator()(const tatum::TimingTag& lhs, const tatum::TimingTag& rhs) {
        return lhs.time().value() < rhs.time().value();
    }
};

//Return the tag from the range [first,last) which has the lowest value
tatum::TimingTags::const_iterator find_minimum_tag(tatum::TimingTags::tag_range tags);

//Return the tag from the range [first,last) which has the highest value
tatum::TimingTags::const_iterator find_maximum_tag(tatum::TimingTags::tag_range tags);


/*
 * Slack and criticality calculation utilities
 */

//Return the criticality of a net's pin in the CLB netlist
float calculate_clb_net_pin_criticality(const SetupTimingInfo& timing_info, const IntraLbPbPinLookup& pb_gpin_lookup, int inet, int ipin);

//Returns the worst (maximum) criticality of the set of slack tags specified. Requires the maximum
//required time and worst slack for all domain pairs represent by the slack tags
//
// Criticality (in [0., 1.]) represents how timing-critical something is, 
// 0. is non-critical and 1. is most-critical.
//
// This returns 'relaxed per constraint' criticaly as defined in:
//
//     M. Wainberg and V. Betz, "Robust Optimization of Multiple Timing Constraints," 
//         IEEE CAD, vol. 34, no. 12, pp. 1942-1953, Dec. 2015. doi: 10.1109/TCAD.2015.2440316
//
// which handles the trade-off between different timing constraints in multi-clock circuits.
float calc_relaxed_criticality(const std::map<DomainPair,float>& domains_max_req,
                               const std::map<DomainPair,float>& domains_worst_slack,
                               const tatum::TimingTags::tag_range tags);

#endif
