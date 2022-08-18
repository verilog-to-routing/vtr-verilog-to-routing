#include <fstream>
#include <sstream>

#include "vtr_log.h"
#include "vtr_assert.h"
#include "vtr_math.h"

#include "globals.h"
#include "timing_util.h"
#include "timing_fail_error.h"

/* Sets terminate_if_timing_fails in the timing context if the user has indicated
 * terminate_if_timing_fails option should be on.
 */
void set_terminate_if_timing_fails(bool cmd_opt_terminate_if_timing_fails) {
    auto& timing_ctx = g_vpr_ctx.mutable_timing();
    timing_ctx.terminate_if_timing_fails = cmd_opt_terminate_if_timing_fails;
}
