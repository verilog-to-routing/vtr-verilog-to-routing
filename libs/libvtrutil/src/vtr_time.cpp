#include "vtr_time.h"

#include "vtr_log.h"

namespace vtr {

ScopedTimer::ScopedTimer()
    : start_(clock::now()) {
}

float ScopedTimer::elapsed_sec() {
    return std::chrono::duration<float>(clock::now() - start_).count();
}

ScopedPrintTimer::ScopedPrintTimer(std::string action)
    : action_(action) {
    vtr::printf_info("%s\n", action_.c_str());
}

ScopedPrintTimer::~ScopedPrintTimer() {
    vtr::printf_info("%s took %.2f seconds\n", action_.c_str(), elapsed_sec());
}

} //namespace
