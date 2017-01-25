#ifndef VPR_TIMING_UTIL_H
#define VPR_TIMING_UTIL_H

#include "timing_analyzers.hpp"
#include "histogram.h"

//Returns the path delay of the most critical (i.e. lowest slack) timing path
float find_critical_path_delay(const tatum::SetupTimingAnalyzer& setup_analyzer);

//Returns the total negative slack (setup) at all timing end-points and clock domain pairs
float find_setup_total_negative_slack(const tatum::SetupTimingAnalyzer& setup_analyzer);

//Returns the worst negative slack (setup) across all timing end-points and clock domain pairs
float find_setup_worst_negative_slack(const tatum::SetupTimingAnalyzer& setup_analyzer);

std::vector<HistogramBucket> find_setup_slack_histogram(const tatum::SetupTimingAnalyzer& setup_analyzer, size_t num_bins = 10);

#endif
