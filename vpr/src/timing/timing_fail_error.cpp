#include <fstream>
#include <sstream>

#include "vtr_log.h"
#include "vtr_assert.h"
#include "vtr_math.h"

#include "globals.h"
#include "timing_util.h"
#include "timing_fail_error.h"

/* Represents whether or not VPR should fail if timing constraints aren't met. */
static bool terminate_if_timing_fails = false;

/* True if negative final slack. */
static bool failed_timing = "";

void set_terminate_if_timing_fails(bool cmd_opt_terminate_if_timing_fails) {
    terminate_if_timing_fails = cmd_opt_terminate_if_timing_fails;
}

/* Checks if slack is negative, if routing is finished, and if user wants VPR flow to fail if their
 * design doesn't meet timingconstraints. If both conditions are true, failed_timing is set to True.
 */
void check_if_failed_timing_constraints(double& slack, std::string prefix) {
    // The error should only be thrown after routing. Checking that prefix == "Final" ensures that
    // only negative slacks found after routing are considered.
    if (terminate_if_timing_fails && slack < 0 && prefix == "Final ") {
        failed_timing = true;
    }
    return;
}

/* Throws an error if slack is negative and if user wants VPR flow to fail if their design doesn't meet timing
 * constraints. Called by print_timing_stats in main.cpp.
 */
void error_if_timing_failed() {
    if (failed_timing) {
        std::string msg = "\nDesign did not meet timing constraints.\nTiming failed and terminate_if_timing_fails set -- exiting";
        VPR_FATAL_ERROR(VPR_ERROR_TIMING, msg.c_str());
    }
    return;
}
