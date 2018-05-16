#ifndef VPR_TIMING_REPORTS_H
#define VPR_TIMING_REPORTS_H

#include "timing_info_fwd.h"

void generate_setup_timing_stats(const SetupTimingInfo& timing_info, bool detailed_reports);
void generate_hold_timing_stats(const HoldTimingInfo& timing_info, bool detailed_reports);

#endif
