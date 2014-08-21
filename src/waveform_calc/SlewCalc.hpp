#ifndef SlewCalc_HPP
#define SlewCalc_HPP

#include "Waveform.hpp"
class TimingEdge;
class TimingNode;

//Abstract Slew calculators
class EdgeSlewCalc {
    public:
        virtual Slew slew(const Waveform& input, const TimingEdge& edge) = 0;
};

class NodeSlewCalc {
    public:
        virtual Slew slew(const TimingNode& node) = 0;
};

//Simple concrete slew calculators
class Unit_EdgeSlewCalc : public EdgeSlewCalc {
    public:
        explicit Unit_EdgeSlewCalc(Slew unit_slew)
            : unit_slew_(unit_slew)
            {}

        Slew slew(const Waveform& input, const TimingEdge& edge) override {
            return Slew(unit_slew_);
        }

    private:
        Slew unit_slew_;
};

class Unit_NodeSlewCalc : public NodeSlewCalc {
    public:
        explicit Unit_NodeSlewCalc(Slew unit_slew)
            : unit_slew_(unit_slew)
            {}

        Slew slew(const TimingNode& node) override {
            return Slew(unit_slew_);
        }

    private:
        Slew unit_slew_;
};

#endif
