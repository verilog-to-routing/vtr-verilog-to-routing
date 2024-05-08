#ifndef PATHHELPER_H
#define PATHHELPER_H

#ifndef NO_SERVER

#include <vector>
#include <string>

#include "tatum/report/TimingPath.hpp"
#include "vpr_types.h"

namespace server {

/** 
 * @brief Structure to retain the calculation result of the critical path.
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

/**
* @brief Calculates the critical path.

* This function calculates the critical path based on the provided parameters.
* @param type The type of the critical path.
* @param crit_path_num The max number of the critical path.
* @param details_level The level of detail for the timing report.
* @param is_flat_routing Indicates whether flat routing should be used.
* @return The result of the critical path calculation. @see CritPathsResult
*/
CritPathsResult calc_critical_path(const std::string& type, int crit_path_num, e_timing_report_detail details_level, bool is_flat_routing);

} // namespace server

#endif /* NO_SERVER */

#endif /* PATHHELPER_H */
