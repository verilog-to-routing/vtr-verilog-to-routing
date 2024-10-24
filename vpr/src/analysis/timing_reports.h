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

#endif
