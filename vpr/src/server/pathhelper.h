#ifndef PATHHELPER_H
#define PATHHELPER_H

#include <vector>
#include <string>
#include <memory>

#include "tatum/report/TimingPath.hpp"

class SetupHoldTimingInfo;

std::string getPathsStr(const std::vector<tatum::TimingPath>& paths, const std::string& detailesLevel, bool is_flat_routing);

std::vector<tatum::TimingPath> calcSetupCritPaths(int numMax);
std::vector<tatum::TimingPath> calcHoldCritPaths(int numMax, const std::shared_ptr<SetupHoldTimingInfo>& hold_timing_info);

#endif
