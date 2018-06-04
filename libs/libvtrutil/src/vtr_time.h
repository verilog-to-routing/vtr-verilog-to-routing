#ifndef VTR_TIME_H
#define VTR_TIME_H
#include <chrono>
#include <string>

namespace vtr {

    //Class for tracking time elapsed since construction
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

    //Scoped elapsed time class which prints the time elapsed for
    //the specified action when it is destructed.
    //
    //For example:
    //
    //      {
    //          vtr::ScopedFinishTimer("my_action")
    //
    //          //Do other work
    //
    //          //Will print: 'my_action took X.XX seconds'
    //      }
    class ScopedFinishTimer : public ScopedTimer {
        public:
            ScopedFinishTimer(const std::string action);
            ~ScopedFinishTimer();
        private:
            const std::string action_;
    };

    //Scoped elapsed time class which prints out the action when
    //initialized and again both the action and elapsed time
    //when destructed.
    //For example:
    //
    //      {
    //          vtr::ScopedActionTimer("my_action") //Will print: 'my_action'
    //
    //          //Do other work
    //
    //          //Will print 'my_action took X.XX seconds'
    //      }
    class ScopedActionTimer : public ScopedFinishTimer {
        public:
            ScopedActionTimer(const std::string action);
    };
}

#endif
