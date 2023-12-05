#ifndef PATHHELPER_H
#define PATHHELPER_H

#include <vector>
#include <string>

#include "tatum/report/TimingPath.hpp"

std::string getPathsStr(const std::vector<tatum::TimingPath>& paths, const std::string& detailesLevel, bool is_flat_routing);
void calcCritPath(int numMax, const std::string& type);

#endif
