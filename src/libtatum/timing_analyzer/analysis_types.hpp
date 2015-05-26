#include "SetupTraversal.hpp"
#include "HoldTraversal.hpp"

/*
 * Useful shorthands for the supported analysis types
 */

//Setup (max) analysis
typedef SetupTraversal<> SetupAnalysis;

//Hold (min) analysis
typedef HoldTraversal<> HoldAnalysis;

//Combined Setup AND Hold analysis
typedef HoldTraversal<SetupTraversal<>> SetupHoldAnalysis;
