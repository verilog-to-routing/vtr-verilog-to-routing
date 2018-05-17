#ifndef VPR_TIMING_REPORTS_H
#define VPR_TIMING_REPORTS_H

#include "timing_info_fwd.h"
#include "AnalysisDelayCalculator.h"
#include "vpr_types.h"

void generate_setup_timing_stats(const SetupTimingInfo& timing_info, const AnalysisDelayCalculator& delay_calc, e_timing_report_detail report_detail);
void generate_hold_timing_stats(const HoldTimingInfo& timing_info, const AnalysisDelayCalculator& delay_calc, e_timing_report_detail report_detail);

#endif
