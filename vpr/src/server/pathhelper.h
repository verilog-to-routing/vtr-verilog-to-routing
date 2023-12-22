#ifndef PATHHELPER_H
#define PATHHELPER_H

#include <vector>
#include <string>
#include <memory>

#include "tatum/report/TimingPath.hpp"
#include "vpr_types.h"

struct CritPathsResult {
    std::vector<tatum::TimingPath> paths;
    std::string report;
};

CritPathsResult calcCriticalPath(const std::string& type, int critPathNum, e_timing_report_detail detailsLevel, bool is_flat_routing);

#endif
