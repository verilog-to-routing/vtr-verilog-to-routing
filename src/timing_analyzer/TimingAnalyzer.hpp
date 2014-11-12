#ifndef TimingAnalyzer_hpp
#define TimingAnalyzer_hpp

class TimingGraph; //Forward declaration

class TimingAnalyzer {
    public:
        virtual void update_timing(TimingGraph& timing_graph) = 0;
};

#endif
