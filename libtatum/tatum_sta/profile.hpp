#ifndef TATUM_PROFILE
#define TATUM_PROFILE
#include <map>
#include <string>
#include <vector>
#include <memory>

#include "TimingAnalyzer.hpp"

std::map<std::string,std::vector<double>> profile(size_t num_iterations, std::shared_ptr<tatum::TimingAnalyzer> serial_analyzer);

#endif
