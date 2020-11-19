#include "vtr_time.h"

#include "vtr_log.h"
#include "vtr_rusage.h"

namespace vtr {

int f_timer_depth = 0;

///@brief Constructor
Timer::Timer()
    : start_(clock::now())
    , initial_max_rss_(get_max_rss()) {
}

///@brief Returns the elapsed seconds since construction
float Timer::elapsed_sec() const {
    return std::chrono::duration<float>(clock::now() - start_).count();
}

///@brief Returns the maximum resident size (rss) in bytes
float Timer::max_rss_mib() const {
    return get_max_rss() / BYTE_TO_MIB;
}

///@brief Returns the change in maximum resident size in bytes
float Timer::delta_max_rss_mib() const {
    return (get_max_rss() - initial_max_rss_) / BYTE_TO_MIB;
}

///@brief Constructor
ScopedActionTimer::ScopedActionTimer(std::string action_str)
    : action_(action_str)
    , depth_(f_timer_depth++) {
}

///@brief Destructor
ScopedActionTimer::~ScopedActionTimer() {
    --f_timer_depth;
}

///@brief Sets quiet value (when true, prints the timing info)
void ScopedActionTimer::quiet(bool value) {
    quiet_ = value;
}

///@brief Returns the quiet value
bool ScopedActionTimer::quiet() const {
    return quiet_;
}

///@brief Returns the action string
std::string ScopedActionTimer::action() const {
    return action_;
}

///@brief Pads the output string with # if it is not empty
std::string ScopedActionTimer::pad() const {
    if (depth() == 0) {
        return "";
    }
    return std::string(depth(), '#') + " ";
}

///@brief Returns the depth
int ScopedActionTimer::depth() const {
    return depth_;
}

///@brief Constructor
ScopedFinishTimer::ScopedFinishTimer(std::string action_str)
    : ScopedActionTimer(action_str) {
}

///@brief Destructor
ScopedFinishTimer::~ScopedFinishTimer() {
    if (!quiet()) {
        vtr::printf_info("%s%s took %.2f seconds (max_rss %.1f MiB)\n",
                         pad().c_str(), action().c_str(), elapsed_sec(),
                         max_rss_mib());
    }
}

///@brief Constructor
ScopedStartFinishTimer::ScopedStartFinishTimer(std::string action_str)
    : ScopedActionTimer(action_str) {
    vtr::printf_info("%s%s\n", pad().c_str(), action().c_str());
}

///@brief Destructor
ScopedStartFinishTimer::~ScopedStartFinishTimer() {
    if (!quiet()) {
        vtr::printf_info("%s%s took %.2f seconds (max_rss %.1f MiB, delta_rss %+.1f MiB)\n",
                         pad().c_str(), action().c_str(), elapsed_sec(),
                         max_rss_mib(), delta_max_rss_mib());
    }
}

} // namespace vtr
