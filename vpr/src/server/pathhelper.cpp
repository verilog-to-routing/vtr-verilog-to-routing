#ifndef NO_SERVER

#include "pathhelper.h"
#include "commconstants.h"

#include "globals.h"
#include "VprTimingGraphResolver.h"
#include "tatum/TimingReporter.hpp"
#include "RoutingDelayCalculator.h"
#include "timing_info.h"

#include <sstream>

namespace server {

/** 
 * @brief helper function to collect crit parser metadata.
 * This data is used on parser side to properly extract arrival path elements from the timing report.
 */
static void collect_crit_path_metadata(std::stringstream& ss, const std::vector<tatum::TimingPath>& paths) {
    ss << "#RPT METADATA:\n";
    ss << "path_index/clock_launch_path_elements_num/arrival_path_elements_num\n";
    std::size_t counter = 0;
    for (const tatum::TimingPath& path : paths) {
        std::size_t offset_index = path.clock_launch_path().elements().size();
        std::size_t selectable_items = path.data_arrival_path().elements().size();
        ss << counter << "/" << offset_index << "/" << selectable_items << "\n";
        counter++;
    }
}

/** 
 * @brief Helper function to calculate critical path timing report with specified parameters.
 */
CritPathsResultPtr calc_critical_path(const std::string& report_type, int crit_path_num, e_timing_report_detail details_level, bool is_flat_routing) {
    // shortcuts
    const std::shared_ptr<SetupHoldTimingInfo>& timing_info = g_vpr_ctx.server().timing_info;
    const std::shared_ptr<RoutingDelayCalculator>& routing_delay_calc = g_vpr_ctx.server().routing_delay_calc;

    auto& timing_ctx = g_vpr_ctx.timing();
    auto& atom_ctx = g_vpr_ctx.atom();
    //

    t_analysis_opts analysis_opts;
    analysis_opts.timing_report_detail = details_level;
    analysis_opts.timing_report_npaths = crit_path_num;

    VprTimingGraphResolver resolver(atom_ctx.nlist, atom_ctx.lookup, *timing_ctx.graph, *routing_delay_calc, is_flat_routing);
    resolver.set_detail_level(analysis_opts.timing_report_detail);

    tatum::TimingReporter timing_reporter(resolver, *timing_ctx.graph, *timing_ctx.constraints);

    CritPathsResultPtr result = std::make_shared<CritPathsResult>();

    std::stringstream ss;
    if (report_type == comm::KEY_SETUP_PATH_LIST) {
        timing_reporter.report_timing_setup(result->paths, ss, *timing_info->setup_analyzer(), analysis_opts.timing_report_npaths);
    } else if (report_type == comm::KEY_HOLD_PATH_LIST) {
        timing_reporter.report_timing_hold(result->paths, ss, *timing_info->hold_analyzer(), analysis_opts.timing_report_npaths);
    }

    if (!result->paths.empty()) {
        collect_crit_path_metadata(ss, result->paths);
        result->report = ss.str();
    }

    return result;
}

} // namespace server

#endif /* NO_SERVER */
