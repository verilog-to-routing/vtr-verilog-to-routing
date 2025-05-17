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
 * @brief Generates a CSV report of timing information for each net in the atom netlist.
 * 
 * Each row in the CSV corresponds to a single net and includes:
 * - Net name
 * - Fanout count
 * - Bounding box (xmin, ymin, layer_min, xmax, ymax, layer_max)
 * - Source pin name and slack
 * - A single "sinks" field that encodes information for all sink pins
 * 
 * The "sinks" field is a semicolon-separated list of all sink pins.
 * Each sink pin is represented as a comma-separated triple:
 *   <sink_pin_name>,<sink_pin_slack>,<sink_pin_delay>
 * 
 * Example row:
 * netA,2,0,0,0,5,5,1,U1.A,0.25,"U2.B,0.12,0.5;U3.C,0.10,0.6"
 * 
 * @param prefix       Prefix for the output file name (report will be saved as <prefix>report_net_timing.csv)
 * @param timing_info  Timing analysis results (slacks)
 * @param delay_calc   Delay calculator used to extract delay between nodes
 */
void generate_net_timing_report(const std::string& prefix,
                                const SetupHoldTimingInfo& timing_info,
                                const AnalysisDelayCalculator& delay_calc);

#endif
