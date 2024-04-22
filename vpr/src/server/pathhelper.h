#ifndef PATHHELPER_H
#define PATHHELPER_H

#include <vector>
#include <string>

#include "tatum/report/TimingPath.hpp"
#include "vpr_types.h"

namespace server {

/** 
 * @brief Structure to retain the calculation result of the critical path.
 * 
 * It contains the critical path list and the generated report as a string.
*/
struct CritPathsResult {
    bool is_valid() const { return !report.empty(); }
    std::vector<tatum::TimingPath> paths;
    std::string report;
};

/** 
 * @brief Helper function to calculate critical path timing report with specified parameters.
 */
CritPathsResult calc_critical_path(const std::string& type, int crit_path_num, e_timing_report_detail details_level, bool is_flat_routing);

} // namespace server

#endif /* PATHHELPER_H */
