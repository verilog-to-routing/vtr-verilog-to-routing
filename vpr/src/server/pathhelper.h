#ifndef PATHHELPER_H
#define PATHHELPER_H

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
    bool isValid() const { return !report.empty(); }
    std::vector<tatum::TimingPath> paths;
    std::string report;
};

/** 
 * @brief Unified helper function to calculate the critical path with specified parameters.
 */
CritPathsResult calcCriticalPath(const std::string& type, int critPathNum, e_timing_report_detail detailsLevel, bool is_flat_routing, bool usePathElementSeparator);

} // namespace server

#endif // PATHHELPER_H
