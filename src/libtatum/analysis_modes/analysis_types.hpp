#include "SetupTraversal.hpp"
#include "HoldTraversal.hpp"

/*
 * Useful shorthands for the supported analysis types
 *
 * Used as template parameters to specify timing analysis
 * mode for TimingAnalyzer and derived classes
 */

//Setup (max) analysis
using SetupAnalysis = SetupTraversal<>;

//Hold (min) analysis
using HoldAnalysis = HoldTraversal<>;

//Combined Setup AND Hold analysis
using SetupHoldAnalysis = HoldTraversal<SetupTraversal<>>;
