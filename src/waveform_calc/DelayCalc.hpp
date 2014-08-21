#ifndef DelayCalc_HPP
#define DelayCalc_HPP

#include "Time.hpp"

class Waveform;
class TimingEdge;
class TimingNode;

//Abstract delay calculators
class EdgeDelayCalc {
    public:
        virtual Time delay(const Waveform& input, const TimingEdge& edge) = 0;
};

class NodeDelayCalc {
    public:
        virtual Time delay(const TimingNode& node) = 0;
};

//Simple concrete delay calculators
class Unit_EdgeDelayCalc : public EdgeDelayCalc {
    public:
        explicit Unit_EdgeDelayCalc(Time unit_delay)
            : unit_delay_(unit_delay)
            {}

        Time delay(const Waveform& input, const TimingEdge& edge) override {
            return Time(unit_delay_);
        }

    private:
        Time unit_delay_;
};

class Unit_NodeDelayCalc : public NodeDelayCalc{
    public:
        explicit Unit_NodeDelayCalc(Time unit_delay)
            : unit_delay_(unit_delay)
            {}

        Time delay(const TimingNode& node) override {
            return Time(unit_delay_);
        }

    private:
        Time unit_delay_;
};

#endif
