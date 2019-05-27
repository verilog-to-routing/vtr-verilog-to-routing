#include "vtr_time.h"

#include "vtr_log.h"
#include "vtr_rusage.h"

namespace vtr {

int f_timer_depth = 0;

Timer::Timer()
    : start_(clock::now())
    , initial_max_rss_(get_max_rss()) {
}

float Timer::elapsed_sec() const {
    return std::chrono::duration<float>(clock::now() - start_).count();
}

float Timer::max_rss_mib() const {
    return get_max_rss() / BYTE_TO_MIB;
}

float Timer::delta_max_rss_mib() const {
    return (get_max_rss() - initial_max_rss_) / BYTE_TO_MIB;
}

ScopedActionTimer::ScopedActionTimer(std::string action_str)
    : action_(action_str)
    , depth_(f_timer_depth++) {
}

ScopedActionTimer::~ScopedActionTimer() {
    --f_timer_depth;
}

void ScopedActionTimer::quiet(bool value) {
    quiet_ = value;
}

bool ScopedActionTimer::quiet() const {
    return quiet_;
}

std::string ScopedActionTimer::action() const {
    return action_;
}

std::string ScopedActionTimer::pad() const {
    if (depth() == 0) {
        return "";
    }
    return std::string(depth(), '#') + " ";
}

int ScopedActionTimer::depth() const {
    return depth_;
}

ScopedFinishTimer::ScopedFinishTimer(std::string action_str)
    : ScopedActionTimer(action_str) {
}

ScopedFinishTimer::~ScopedFinishTimer() {
    if (!quiet()) {
        vtr::printf_info("%s%s took %.2f seconds (max_rss %.1f MiB)\n",
                         pad().c_str(), action().c_str(), elapsed_sec(),
                         max_rss_mib());
    }
}

ScopedStartFinishTimer::ScopedStartFinishTimer(std::string action_str)
    : ScopedActionTimer(action_str) {
    vtr::printf_info("%s%s\n", pad().c_str(), action().c_str());
}

ScopedStartFinishTimer::~ScopedStartFinishTimer() {
    if (!quiet()) {
        vtr::printf_info("%s%s took %.2f seconds (max_rss %.1f MiB, delta_rss %+.1f MiB)\n",
                         pad().c_str(), action().c_str(), elapsed_sec(),
                         max_rss_mib(), delta_max_rss_mib());
    }
}

} // namespace vtr
