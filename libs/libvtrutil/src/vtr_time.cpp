#include "vtr_time.h"

#include "vtr_log.h"
#include "vtr_rusage.h"

namespace vtr {

ScopedTimer::ScopedTimer()
    : start_(clock::now())
    , initial_max_rss_(get_max_rss()) {
}

float ScopedTimer::elapsed_sec() const {
    return std::chrono::duration<float>(clock::now() - start_).count();
}

float ScopedTimer::max_rss_mib() const {
    return get_max_rss() / BYTE_TO_MIB;
}

float ScopedTimer::delta_max_rss_mib() const {
    return (get_max_rss() - initial_max_rss_) / BYTE_TO_MIB;
}


ScopedFinishTimer::ScopedFinishTimer(std::string action_str)
    : action_(action_str) {
}

ScopedFinishTimer::~ScopedFinishTimer() {
    if (!quiet()) {
        vtr::printf_info("%s took %.2f seconds, peak_rss %.1f MiB (%+.1f MiB)\n",
                         action().c_str(), elapsed_sec(),
                         max_rss_mib(), delta_max_rss_mib());
    }
}

void ScopedFinishTimer::quiet(bool value) {
    quiet_ = value; 
}

bool ScopedFinishTimer::quiet() const {
    return quiet_;
}

std::string ScopedFinishTimer::action() const {
    return action_;
}

ScopedActionTimer::ScopedActionTimer(std::string action_str)
    : ScopedFinishTimer(action_str) {
    vtr::printf_info("%s\n", action().c_str());
}

} //namespace
