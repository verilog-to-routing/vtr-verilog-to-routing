#include "timing_reports.h"

#include "vtr_log.h"

#include "tatum/TimingReporter.hpp"

#include "vpr_types.h"
#include "globals.h"

#include "timing_info.h"
#include "timing_util.h"

#include "VprTimingGraphResolver.h"

void generate_setup_timing_stats(const std::string& prefix, const SetupTimingInfo& timing_info, const AnalysisDelayCalculator& delay_calc, const t_analysis_opts& analysis_opts) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& timing_ctx = g_vpr_ctx.timing();
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

    print_setup_timing_summary(*timing_ctx.constraints, *timing_info.setup_analyzer());

    IntraLbPbPinLookup pb_gpin_lookup(device_ctx.block_types, device_ctx.num_block_types);
    ClusteredPinAtomPinsLookup pin_lookup(atom_ctx.nlist, cluster_ctx.clb_nlist, pb_gpin_lookup);
    VprTimingGraphResolver resolver(
        atom_ctx.nlist, atom_ctx.lookup,
        cluster_ctx.clb_nlist,
        pin_lookup,
        *timing_ctx.graph, delay_calc,
        route_ctx.net_rr_terminals);
    resolver.set_detail_level(analysis_opts.timing_report_detail);

    tatum::TimingReporter timing_reporter(resolver, *timing_ctx.graph, *timing_ctx.constraints);

    timing_reporter.report_timing_setup(prefix + "report_timing.setup.rpt", *timing_info.setup_analyzer(), analysis_opts.timing_report_npaths);

    if (analysis_opts.timing_report_skew) {
        timing_reporter.report_skew_setup(prefix + "report_skew.setup.rpt", *timing_info.setup_analyzer(), analysis_opts.timing_report_npaths);
    }

    timing_reporter.report_unconstrained_setup(prefix + "report_unconstrained_timing.setup.rpt", *timing_info.setup_analyzer());
}

void generate_hold_timing_stats(const std::string& prefix, const HoldTimingInfo& timing_info, const AnalysisDelayCalculator& delay_calc, const t_analysis_opts& analysis_opts) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& timing_ctx = g_vpr_ctx.timing();
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

    print_hold_timing_summary(*timing_ctx.constraints, *timing_info.hold_analyzer());

    IntraLbPbPinLookup pb_gpin_lookup(device_ctx.block_types, device_ctx.num_block_types);
    ClusteredPinAtomPinsLookup pin_lookup(atom_ctx.nlist, cluster_ctx.clb_nlist, pb_gpin_lookup);
    VprTimingGraphResolver resolver(
        atom_ctx.nlist, atom_ctx.lookup,
        cluster_ctx.clb_nlist,
        pin_lookup,
        *timing_ctx.graph, delay_calc,
        route_ctx.net_rr_terminals);
    resolver.set_detail_level(analysis_opts.timing_report_detail);

    tatum::TimingReporter timing_reporter(resolver, *timing_ctx.graph, *timing_ctx.constraints);

    timing_reporter.report_timing_hold(prefix + "report_timing.hold.rpt", *timing_info.hold_analyzer(), analysis_opts.timing_report_npaths);

    if (analysis_opts.timing_report_skew) {
        timing_reporter.report_skew_hold(prefix + "report_skew.hold.rpt", *timing_info.hold_analyzer(), analysis_opts.timing_report_npaths);
    }

    timing_reporter.report_unconstrained_hold(prefix + "report_unconstrained_timing.hold.rpt", *timing_info.hold_analyzer());
}
