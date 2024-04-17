#include "pathhelper.h"
#include "globals.h"
#include "vpr_net_pins_matrix.h"
#include "VprTimingGraphResolver.h"
#include "tatum/TimingReporter.hpp"

#include "draw_types.h"
#include "draw_global.h"
#include "net_delay.h"
#include "concrete_timing_info.h"
#include "commconstants.h"

#include "timing_info_fwd.h"
#include "AnalysisDelayCalculator.h"
#include "vpr_types.h"

#include <sstream>

namespace server {

/** 
 * @brief helper function to collect crit parser metadata.
 * This data is used on parser side to properly extract arrival path elements from the timing report.
 */
static void collect_crit_path_metadata(std::stringstream& ss, const std::vector<tatum::TimingPath>& paths)
{
    ss << "#RPT METADATA:\n";
    ss << "path_index/clock_launch_path_elements_num/arrival_path_elements_num\n";
    std::size_t counter = 0;
    for (const tatum::TimingPath& path: paths) {
        std::size_t offset_index = path.clock_launch_path().elements().size();
        std::size_t selectable_items = path.data_arrival_path().elements().size();
        ss << counter << "/" << offset_index << "/" << selectable_items << "\n";
        counter++;
    }
}

/** 
 * @brief helper function to generate critical path timing report with specified parameters.
 */
static CritPathsResult generate_timing_report(const SetupHoldTimingInfo& timing_info,
                                              const AnalysisDelayCalculator& delay_calc,
                                              const t_analysis_opts& analysis_opts,
                                              const std::string& report_type,
                                              bool is_flat) {
    auto& timing_ctx = g_vpr_ctx.timing();
    auto& atom_ctx = g_vpr_ctx.atom();

    VprTimingGraphResolver resolver(atom_ctx.nlist, atom_ctx.lookup, *timing_ctx.graph, delay_calc, is_flat);
    resolver.set_detail_level(analysis_opts.timing_report_detail);

    tatum::TimingReporter timing_reporter(resolver, *timing_ctx.graph, *timing_ctx.constraints);

    std::vector<tatum::TimingPath> paths;
    std::stringstream ss;
    if (report_type == comm::KEY_SETUP_PATH_LIST) {
        timing_reporter.report_timing_setup(paths, ss, *timing_info.setup_analyzer(), analysis_opts.timing_report_npaths);
    } else if (report_type == comm::KEY_HOLD_PATH_LIST) {
        timing_reporter.report_timing_hold(paths, ss, *timing_info.hold_analyzer(), analysis_opts.timing_report_npaths);
    }

    if (!paths.empty()) {
        collect_crit_path_metadata(ss, paths);
        return CritPathsResult{paths, ss.str()};
    } else {
        return CritPathsResult{std::vector<tatum::TimingPath>(), ""};
    }
}

/** 
 * @brief Helper function to calculate critical path timing report with specified parameters.
 */
CritPathsResult calcCriticalPath(const std::string& report_type, int critPathNum, e_timing_report_detail detailsLevel, bool is_flat_routing) 
{
    // shortcuts
    auto& atom_ctx = g_vpr_ctx.atom();

    //Load the net delays
    const Netlist<>& net_list = is_flat_routing ? (const Netlist<>&)g_vpr_ctx.atom().nlist : (const Netlist<>&)g_vpr_ctx.clustering().clb_nlist;

    NetPinsMatrix<float> net_delay = make_net_pins_matrix<float>(net_list);
    load_net_delay_from_routing(net_list,
                                net_delay);

    //Do final timing analysis
    auto analysis_delay_calc = std::make_shared<AnalysisDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, net_delay, is_flat_routing);
    
    e_timing_update_type timing_update_type = e_timing_update_type::AUTO;     // FULL, INCREMENTAL, AUTO
    std::unique_ptr<SetupHoldTimingInfo> timing_info = make_setup_hold_timing_info(analysis_delay_calc, timing_update_type);
    timing_info->update();

    t_analysis_opts analysis_opt;
    analysis_opt.timing_report_detail = detailsLevel;
    analysis_opt.timing_report_npaths = critPathNum;

    return generate_timing_report(*timing_info, *analysis_delay_calc, analysis_opt, report_type, is_flat_routing);
}

} // namespace server
