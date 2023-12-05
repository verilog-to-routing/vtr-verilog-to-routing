#include "pathhelper.h"
#include "globals.h"
#include "vpr_net_pins_matrix.h"
#include "VprTimingGraphResolver.h"
#include "tatum/TimingReporter.hpp"

#include "draw_types.h"
#include "draw_global.h"

#include <sstream>
#include <cassert>

std::string getPathsStr(const std::vector<tatum::TimingPath>& paths, const std::string& detailsLevel, bool is_flat_routing) 
{
    // shortcuts
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& timing_ctx = g_vpr_ctx.timing();
    // shortcuts
    
    // build deps
    NetPinsMatrix<float> net_delay = make_net_pins_matrix<float>(atom_ctx.nlist);
    auto analysis_delay_calc = std::make_shared<AnalysisDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, net_delay, is_flat_routing);
    
    VprTimingGraphResolver resolver(atom_ctx.nlist, atom_ctx.lookup, *timing_ctx.graph, *analysis_delay_calc.get(), is_flat_routing);
    e_timing_report_detail detailesLevelEnum = e_timing_report_detail::NETLIST;
    if (detailsLevel == "netlist") {
        detailesLevelEnum = e_timing_report_detail::NETLIST;
    } else if (detailsLevel == "aggregated") {
        detailesLevelEnum = e_timing_report_detail::AGGREGATED;
    } else if (detailsLevel == "detailed") {
        detailesLevelEnum = e_timing_report_detail::DETAILED_ROUTING;
    } else if (detailsLevel == "debug") {
        detailesLevelEnum = e_timing_report_detail::DEBUG;
    } else {
        std::cerr << "unhandled option" << detailsLevel << std::endl;
    }
    resolver.set_detail_level(detailesLevelEnum);
    //

    std::stringstream ss;
    tatum::TimingReporter timing_reporter(resolver, *timing_ctx.graph, *timing_ctx.constraints);
    timing_reporter.report_timing(ss, paths);

    return ss.str();
}

std::vector<tatum::TimingPath> calcSetupCritPaths(int numMax)
{
    tatum::TimingPathCollector path_collector;

    t_draw_state* draw_state = get_draw_state_vars();
    auto& timing_ctx = g_vpr_ctx.timing();

    return path_collector.collect_worst_setup_timing_paths(
        *timing_ctx.graph,
        *(draw_state->setup_timing_info->setup_analyzer()), numMax);
}

std::vector<tatum::TimingPath> calcHoldCritPaths(int numMax, const std::shared_ptr<SetupHoldTimingInfo>& hold_timing_info)
{
    tatum::TimingPathCollector path_collector;

    auto& timing_ctx = g_vpr_ctx.timing();

    return path_collector.collect_worst_hold_timing_paths(
        *timing_ctx.graph,
        *(hold_timing_info->hold_analyzer()), numMax);
}