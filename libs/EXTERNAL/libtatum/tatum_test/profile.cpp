#include <iostream>
#include <cmath>
#include <chrono>
#include "profile.hpp"

#ifdef TATUM_STA_PROFILE_VTUNE
#include "ittnotify.h"
#endif

#ifdef TATUM_STA_PROFILE_CALLGRIND
#include <valgrind/callgrind.h>
#endif


typedef std::chrono::duration<double> dsec;
typedef std::chrono::high_resolution_clock Clock;

std::map<std::string,std::vector<double>> profile(size_t num_iterations, std::shared_ptr<tatum::TimingAnalyzer> serial_analyzer) {
    //To selectively profile using callgrind:
    //  valgrind --tool=callgrind --collect-atstart=no --instr-atstart=no --cache-sim=yes --cacheuse=yes ./command

    std::map<std::string,std::vector<double>> prof_data;

#ifdef TATUM_STA_PROFILE_CALLGRIND
    CALLGRIND_START_INSTRUMENTATION;
#endif

    for(size_t i = 0; i < num_iterations; i++) {
        //Analyze

#ifdef TATUM_STA_PROFILE_VTUNE
        __itt_resume();
#endif

#ifdef TATUM_STA_PROFILE_CALLGRIND
        CALLGRIND_TOGGLE_COLLECT;
#endif

        serial_analyzer->update_timing();

#ifdef TATUM_STA_PROFILE_CALLGRIND
        CALLGRIND_TOGGLE_COLLECT;
#endif

#ifdef TATUM_STA_PROFILE_VTUNE
        __itt_pause();
#endif

        for(auto key : {"arrival_pre_traversal_sec", "arrival_traversal_sec", "required_pre_traversal_sec", "required_traversal_sec", "reset_sec", "update_slack_sec", "analysis_sec"}) {
            prof_data[key].push_back(serial_analyzer->get_profiling_data(key));
        }


        std::cout << ".";
        std::cout.flush();
    }

#ifdef TATUM_STA_PROFILE_CALLGRIND
    CALLGRIND_STOP_INSTRUMENTATION;
#endif

    return prof_data;
}
