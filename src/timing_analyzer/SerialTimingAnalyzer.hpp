#ifndef SerialTimingAnalyzer_hpp
#define SerialTimingAnalyzer_hpp
#include "TimingAnalyzer.hpp"

class SerialTimingAnalyzer : public TimingAnalyzer {
    public: 
        void update_timing(TimingGraph& timing_graph);
};

#endif
