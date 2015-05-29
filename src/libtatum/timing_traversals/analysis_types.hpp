#include "SetupTraversal.hpp"
#include "HoldTraversal.hpp"

/*
 * Useful shorthands for the supported analysis types
 *
 * Used as template parameters to specify timing analysis
 * mode for TimingAnalyzer and derived classes
 */

//Setup (max) analysis
//typedef SetupTraversal<> SetupAnalysis;
template<class DelayCalc>
using SetupAnalysis = SetupTraversal<DelayCalc>;

//Hold (min) analysis
//typedef HoldTraversal<> HoldAnalysis;
template<class DelayCalc>
using HoldAnalysis = HoldTraversal<DelayCalc>;

//Combined Setup AND Hold analysis
template<class DelayCalc>
using SetupHoldAnalysis = HoldTraversal<DelayCalc, SetupTraversal<DelayCalc>>;
