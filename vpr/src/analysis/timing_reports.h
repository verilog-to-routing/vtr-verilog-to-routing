#ifndef VPR_TIMING_REPORTS_H
#define VPR_TIMING_REPORTS_H

#include "timing_info_fwd.h"
#include "AnalysisDelayCalculator.h"
#include "vpr_types.h"

class BlkLocRegistry;

void generate_setup_timing_stats(const std::string& prefix,
                                 const SetupTimingInfo& timing_info,
                                 const AnalysisDelayCalculator& delay_calc,
                                 const t_analysis_opts& report_detail,
                                 bool is_flat,
                                 const BlkLocRegistry& blk_loc_registry);

void generate_hold_timing_stats(const std::string& prefix,
                                const HoldTimingInfo& timing_info,
                                const AnalysisDelayCalculator& delay_calc,
                                const t_analysis_opts& report_detail,
                                bool is_flat,
                                const BlkLocRegistry& blk_loc_registry);

/**
 * @brief Generates timing information for each net in atom netlist. For each net, the timing information
 * is reported in the following format:
 * netname : Fanout : source_instance <slack_on source pin> : 
 * <load pin name1> <slack on load pin name1> <net delay for this net> : 
 * <load pin name2> <slack on load pin name2> <net delay for this net> : ...
 * 
 * @param prefix The prefix for the report file to be added to filename: report_net_timing.rpt
 * @param timing_info Updated timing information
 * @param delay_calc Delay calculator
 */
void generate_net_timing_report(const std::string& prefix,
                                const SetupHoldTimingInfo& timing_info,
                                const AnalysisDelayCalculator& delay_calc);

#endif
