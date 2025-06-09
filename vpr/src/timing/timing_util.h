#pragma once

#include <vector>
#include <string_view>

#include "netlist_fwd.h"
#include "tatum/TimingConstraints.hpp"

#include "histogram.h"
#include "tatum/analyzers/HoldTimingAnalyzer.hpp"
#include "tatum/analyzers/SetupTimingAnalyzer.hpp"
#include "tatum/report/TimingPath.hpp"
#include "timing_info_fwd.h"
#include "DomainPair.h"

#include "clustered_netlist_utils.h"

double sec_to_nanosec(double seconds);

double sec_to_mhz(double seconds);

/*
 * Setup-time related statistics
 */

//Returns the path delay of the longest critical timing path (i.e. across all domains)
tatum::TimingPathInfo find_longest_critical_path_delay(const tatum::TimingConstraints& constraints, const tatum::SetupTimingAnalyzer& setup_analyzer);

//Returns the path delay of the least-slack critical timing path (i.e. across all domains)
tatum::TimingPathInfo find_least_slack_critical_path_delay(const tatum::TimingConstraints& constraints, const tatum::SetupTimingAnalyzer& setup_analyzer);

//Returns the total negative slack (setup) of all timing end-points and clock domain pairs
float find_setup_total_negative_slack(const tatum::SetupTimingAnalyzer& setup_analyzer);

//Returns the worst negative slack (setup) across all timing end-points and clock domain pairs
float find_setup_worst_negative_slack(const tatum::SetupTimingAnalyzer& setup_analyzer);

//Returns the slack at a particular node for the specified clock domains (if found), otherwise NAN
float find_node_setup_slack(const tatum::SetupTimingAnalyzer& setup_analyzer, tatum::NodeId node, tatum::DomainId launch_domain, tatum::DomainId capture_domain);

//Returns a setup slack histogram
std::vector<HistogramBucket> create_setup_slack_histogram(const tatum::SetupTimingAnalyzer& setup_analyzer, size_t num_bins = 10);

//Returns a criticality histogram
std::vector<HistogramBucket> create_criticality_histogram(const Netlist<>& net_list,
                                                          const SetupTimingInfo& setup_timing,
                                                          const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                                          bool is_flat,
                                                          size_t num_bins = 10);

//Print a useful summary of timing information
void print_setup_timing_summary(const tatum::TimingConstraints& constraints,
                                const tatum::SetupTimingAnalyzer& setup_analyzer,
                                std::string_view prefix,
                                std::string_view timing_summary_filename);

/*
 * Hold-time related statistics
 */
//Returns the total negative slack (hold) of all timing end-points and clock domain pairs
float find_hold_total_negative_slack(const tatum::HoldTimingAnalyzer& hold_analyzer);

//Returns the worst negative slack (hold) across all timing end-points and clock domain pairs
float find_hold_worst_negative_slack(const tatum::HoldTimingAnalyzer& hold_analyzer);

//Returns the worst slack (hold) between the specified launch and capture clock domains
float find_hold_worst_slack(const tatum::HoldTimingAnalyzer& hold_analyzer, const tatum::DomainId launch, const tatum::DomainId capture);

//Returns a setup slack histogram
std::vector<HistogramBucket> create_hold_slack_histogram(const tatum::HoldTimingAnalyzer& hold_analyzer, size_t num_bins = 10);

//Print a useful summary of timing information
void print_hold_timing_summary(const tatum::TimingConstraints& constraints,
                               const tatum::HoldTimingAnalyzer& hold_analyzer,
                               std::string_view prefix);

float find_total_negative_slack_within_clb_blocks(const tatum::HoldTimingAnalyzer& hold_analyzer);

tatum::NodeId find_origin_node_for_hold_slack(const tatum::TimingTags::tag_range arrival_tags,
                                              const tatum::TimingTags::tag_range required_tags,
                                              float slack);

/*
 * General utilities
 */

//Returns the a map of domain's and their clock fanout (i.e. logical outputs at which the clock captures)
std::map<tatum::DomainId, size_t> count_clock_fanouts(const tatum::TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& setup_analyzer);

/*
 * Slack and criticality calculation utilities
 */

//Return the criticality of a net's pin in the CLB netlist
float calculate_clb_net_pin_criticality(const SetupTimingInfo& timing_info,
                                        const ClusteredPinAtomPinsLookup& pin_lookup,
                                        const ParentPinId clb_pin,
                                        bool is_flat);

//Return the setup slack of a net's pin in the CLB netlist
float calculate_clb_net_pin_setup_slack(const SetupTimingInfo& timing_info, const ClusteredPinAtomPinsLookup& pin_lookup, ClusterPinId clb_pin);

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
float calc_relaxed_criticality(const std::map<DomainPair, float>& domains_max_req,
                               const std::map<DomainPair, float>& domains_worst_slack,
                               const tatum::TimingTags::tag_range tags);

/*
 * Debug
 */
void print_tatum_cpds(const std::vector<tatum::TimingPathInfo>& cpds);

tatum::NodeId id_or_pin_name_to_tnode(const std::string& name_or_id);
tatum::NodeId pin_name_to_tnode(const std::string& name);

void write_setup_timing_graph_dot(const std::string& filename, const SetupTimingInfo& timing_info, tatum::NodeId debug_node = tatum::NodeId::INVALID());
void write_hold_timing_graph_dot(const std::string& filename, HoldTimingInfo& timing_info, tatum::NodeId debug_node = tatum::NodeId::INVALID());

struct TimingStats {
  private:
    void writeHuman(std::ostream& output) const;
    void writeJSON(std::ostream& output) const;
    void writeXML(std::ostream& output) const;

  public:
    TimingStats(std::string prefix, double least_slack_cpd_delay, double fmax, double setup_worst_neg_slack, double setup_total_neg_slack);

    enum OutputFormat {
        HumanReadable,
        JSON,
        XML
    };

    double least_slack_cpd_delay;
    double fmax;
    double setup_worst_neg_slack;
    double setup_total_neg_slack;
    std::string prefix;

    void write(OutputFormat fmt, std::ostream& output) const;
};

//Write a useful summary of timing information to JSON file
void write_setup_timing_summary(std::string_view timing_summary_filename, const TimingStats& stats);
