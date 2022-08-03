#ifndef VPR_FAIL_ERROR_H
#define VPR_FAIL_ERROR_H
#include <memory>

#include "timing_info_fwd.h"
#include "tatum/analyzer_factory.hpp"
#include "tatum/timing_paths.hpp"
#include "timing_util.h"

/* Represents whether or not VPR should fail if timing constraints aren't met. */
void set_terminate_if_timing_fails(bool cmd_opt_terminate_if_timing_fails);

#endif
