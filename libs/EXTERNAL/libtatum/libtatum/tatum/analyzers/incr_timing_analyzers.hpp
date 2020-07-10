#pragma once

/** \file
 * This file defines concrete implementations of the TimingAnalyzer interfaces.
 *
 * In particular these concrete analyzers are 'incr' (i.e. incremental) timing analyzers,
 * ever call to update_timing_impl() incrementally re-analyze the timing graph.
 */

#include "IncrSetupTimingAnalyzer.hpp"
#include "IncrHoldTimingAnalyzer.hpp"
#include "IncrSetupHoldTimingAnalyzer.hpp"
