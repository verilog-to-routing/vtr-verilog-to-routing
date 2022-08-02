#ifndef VPR_FAIL_ERROR_H
#define VPR_FAIL_ERROR_H
#include <memory>

#include "timing_info_fwd.h"
#include "tatum/analyzer_factory.hpp"
#include "tatum/timing_paths.hpp"
#include "timing_util.h"

/* Checks if slack is negative and if user wants VPR flow to fail if their desisn doesn't meet timing
 * constraints. If both conditions are true, the function adds details about the negative slack to a string
 * that will be printed when VPR throws an error.
 */
void check_if_failed_timing_constraints(double& slack, std::string slack_name, std::string prefix);

void error_if_timing_failed();

/* Represents whether or not VPR should fail if timing constraints aren't met. */
void set_terminate_if_timing_fails(bool cmd_opt_terminate_if_timing_fails);

#endif
