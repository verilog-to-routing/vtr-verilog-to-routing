#ifndef VPR_TIMING_UTIL_H
#define VPR_TIMING_UTIL_H
#include <vector>

#include "timing_analyzers.hpp"
#include "TimingConstraints.hpp"
#include "histogram.h"

struct PathInfo {
    PathInfo() = default;
    PathInfo(float delay, tatum::NodeId launch_n, tatum::NodeId capture_n, tatum::DomainId launch_d, tatum::DomainId capture_d)
        : path_delay(delay)
        , launch_node(launch_n)
        , capture_node(capture_n)
        , launch_domain(launch_d)
        , capture_domain(capture_d) {}

    float path_delay = NAN;

    //The timing source and sink which launched,
    //and captured the data
    tatum::NodeId launch_node;
    tatum::NodeId capture_node;

    //The launching clock domain
    tatum::DomainId launch_domain;

    //The capture clock domain
    tatum::DomainId capture_domain;
};

//Returns the path delay of the longest critical timing path (i.e. across all domains)
PathInfo find_longest_critical_path_delay(const tatum::TimingConstraints& constraints, const tatum::SetupTimingAnalyzer& setup_analyzer);

//Returns the critical path delay between all active launch/capture clock domain pairs
std::vector<PathInfo> find_critical_path_delays(const tatum::TimingConstraints& constraints, const tatum::SetupTimingAnalyzer& setup_analyzer);

//Returns the total negative slack (setup) at all timing end-points and clock domain pairs
float find_setup_total_negative_slack(const tatum::SetupTimingAnalyzer& setup_analyzer);

//Returns the worst negative slack (setup) across all timing end-points and clock domain pairs
float find_setup_worst_negative_slack(const tatum::SetupTimingAnalyzer& setup_analyzer);

std::vector<HistogramBucket> find_setup_slack_histogram(const tatum::SetupTimingAnalyzer& setup_analyzer, size_t num_bins = 10);

#endif
