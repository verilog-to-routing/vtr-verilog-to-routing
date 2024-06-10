#ifndef PATHHELPER_H
#define PATHHELPER_H

#ifndef NO_SERVER

#include <vector>
#include <string>
#include <memory>

#include "tatum/report/TimingPath.hpp"
#include "vpr_types.h"

namespace server {

/** 
 * @brief Structure to retain the calculation result of the critical path.
 * 
 * It contains the critical path list and the generated report as a string.
*/
struct CritPathsResult {
    /**
    * @brief Checks if the CritPathsResult contains report.
    * @return True if contains report, false otherwise.
    */
    bool is_valid() const { return !report.empty(); }

    /**
    * @brief Vector containing timing paths.
    */
    std::vector<tatum::TimingPath> paths;

    /**
    * @brief String containing the generated report.
    */
    std::string report;
};
using CritPathsResultPtr = std::shared_ptr<CritPathsResult>;

/**
* @brief Calculates the critical path.

* This function calculates the critical path based on the provided parameters.
* @param type The type of the critical path. Must be either "setup" or "hold".
* @param crit_path_num The max number of critical paths to record.
* @param details_level The level of detail for the timing report. See @ref e_timing_report_detail.
* @param is_flat_routing Indicates whether flat routing should be used.
* @return A `CritPathsResultPtr` which is a pointer to the result of the critical path calculation (see @ref CritPathsResult).
*/
CritPathsResultPtr calc_critical_path(const std::string& type, int crit_path_num, e_timing_report_detail details_level, bool is_flat_routing);

} // namespace server

#endif /* NO_SERVER */

#endif /* PATHHELPER_H */
