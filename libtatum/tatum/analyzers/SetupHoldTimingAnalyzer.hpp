#pragma once
#include "SetupTimingAnalyzer.hpp"
#include "HoldTimingAnalyzer.hpp"

namespace tatum {

/**
 * SetupHoldTimingAnalyzer represents an abstract interface for all timing analyzers
 * performing combined setup and hold (i.e. simultaneous long and short-path) analysis.
 *
 * A combined analysis tends to be more efficient than performing two seperate analysies
 * (provided both setup and hold data are truly required).
 *
 * It implements both the SetupTimingAnalyzer and HoldTimingAnalyzer interfaces.
 */
class SetupHoldTimingAnalyzer : public SetupTimingAnalyzer, public HoldTimingAnalyzer {
    //Empty (all behaviour inherited)
    //
    // Note that SetupTiminganalyzer and HoldTimingAnalyzer used virtual inheritance, so
    // there is no ambiguity when inheriting from both (there will be only one base class
    // instance).
};


} //namepsace
