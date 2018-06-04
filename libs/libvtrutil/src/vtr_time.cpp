#include "vtr_time.h"

#include "vtr_log.h"

namespace vtr {

ScopedTimer::ScopedTimer()
    : start_(clock::now()) {
}

float ScopedTimer::elapsed_sec() {
    return std::chrono::duration<float>(clock::now() - start_).count();
}

ScopedFinishTimer::ScopedFinishTimer(std::string action)
    : action_(action) {
}

ScopedFinishTimer::~ScopedFinishTimer() {
    vtr::printf_info("%s took %.2f seconds\n", action_.c_str(), elapsed_sec());
}

ScopedActionTimer::ScopedActionTimer(std::string action)
    : ScopedFinishTimer(action) {
    vtr::printf_info("%s\n", action.c_str());
}

} //namespace
