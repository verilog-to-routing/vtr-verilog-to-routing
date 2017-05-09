#include "timing_reports.h"

#include "vtr_log.h"

#include "tatum/TimingReporter.hpp"

#include "vpr_types.h"
#include "globals.h"

#include "timing_info.h"
#include "timing_util.h"

#include "VprTimingGraphNameResolver.h"

void generate_timing_stats(const SetupTimingInfo& timing_info) {
#ifdef ENABLE_CLASSIC_VPR_STA
    vtr::printf("\n");
    vtr::printf("New Timing Stats\n");
    vtr::printf("================\n");
#endif
    vtr::printf("\n");
    print_setup_timing_summary(*g_ctx.timing_constraints, *timing_info.setup_analyzer());

    VprTimingGraphNameResolver name_resolver(g_ctx.atom_nl, g_ctx.atom_lookup);
    tatum::TimingReporter timing_reporter(name_resolver, *g_ctx.timing_graph, *g_ctx.timing_constraints);

    timing_reporter.report_timing_setup("report_timing.setup.rpt", *timing_info.setup_analyzer());
    timing_reporter.report_unconstrained_endpoints_setup("report_unconstrained_timing_endpoints.rpt", *timing_info.setup_analyzer());
}
