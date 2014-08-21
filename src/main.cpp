#include <iostream>
#include <memory>

#include "make_unique.hpp"

#include "version.hpp"

#include "DelayCalc.hpp"
#include "SlewCalc.hpp"
#include "WaveformCalc.hpp"

class WaveformCalculator;
using std::unique_ptr;
using fix::make_unique;

int main(int argc, char* argv[]) {
    std::cout << "Lib STA: " << build_version << " Compiled: " << build_date << std::endl;

    //Create a waveform calculator that uses a unit delay model for both nodes and edges
    Time unit_delay(1.);
    Slew unit_slew(1.);
    unique_ptr<WaveformCalc> waveform_calc = make_unique<DelaySlew_WaveformCalc>(make_unique<Unit_NodeDelayCalc>(unit_delay),
                                                                                 make_unique<Unit_NodeSlewCalc>(unit_slew),
                                                                                 make_unique<Unit_EdgeDelayCalc>(unit_delay),
                                                                                 make_unique<Unit_EdgeSlewCalc>(unit_slew));
}

