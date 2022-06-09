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
static std::string fail_timing_msg = "";

void toggle_terminate_if_timing_fails(bool cmd_opt_terminate_if_timing_fails){
	terminate_if_timing_fails = cmd_opt_terminate_if_timing_fails;
}

/* Checks if slack is negative and if user wants VPR flow to fail if their design doesn't meet timing
 * constraints. If both conditions are true, the function adds details about the negative slack to a string
 * that will be printed when VPR throws an error.
 */
void check_if_failed_timing_constraints(double& slack, std::string slack_name, std::string prefix) {
    if (terminate_if_timing_fails && slack < 0  && name == "Flow") {
        fail_timing_msg = fail_timing_msg + " " + prefix.c_str() + slack_name.c_str() + " Negative Slack: " + std::to_string(slack) + "\n";
    }
    return;
}

/* Throws an error if slack is negative and if user wants VPR flow to fail if their design doesn't meet timing
 * constraints. Called by print_timing_stats in main.cpp
 */
void error_if_timing_failed(std::string name) {
    // Every time a negative slack is found, it adds on to fail_timing_msg where it failed.
    // If fail_timing_msg is empty, then no negative slacks were found.
	// The error should only be thrown after processing finishes. Because print_timing_stats is called multiple times
	// and we only want the error to be thrown the last time its called (which is right before processing ends),
	// we check that the name parameter of the print_timing_stats that calls this function
	// matches the name parameter of the last print_timing_stats call.
    if (fail_timing_msg != "") {
        fail_timing_msg = "\nDesign did not meet timing constraints.\n"
        		"Terminating processing due to one or more negative slack and option --terminate_if_timing_fails set:\n" + fail_timing_msg;
        const char* msg = fail_timing_msg.c_str();
        VPR_FATAL_ERROR(VPR_ERROR_TIMING, msg);
    }
    return;
}
