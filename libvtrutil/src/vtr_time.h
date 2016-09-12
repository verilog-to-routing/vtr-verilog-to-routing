#ifndef VTR_TIME_H
#define VTR_TIME_H
#include <chrono>
#include <string>

namespace vtr {
    class ScopedTimer {
        public:
            ScopedTimer();

            //No copy
            ScopedTimer(ScopedTimer&) = delete;
            ScopedTimer& operator=(ScopedTimer&) = delete;

            //No move
            ScopedTimer(ScopedTimer&&) = delete;
            ScopedTimer& operator=(ScopedTimer&&) = delete;

            float elapsed_sec();
        private:
            using clock = std::chrono::steady_clock;
            std::chrono::time_point<clock> start_;
    };

    class ScopedPrintTimer : public ScopedTimer {
        public:
            ScopedPrintTimer(const std::string action);
            ~ScopedPrintTimer();
        private:
            const std::string action_;
    };
}

#endif
