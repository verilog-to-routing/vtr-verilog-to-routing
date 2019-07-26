#include "vtr_log.h"

#include "timing_info.h"
#include "concrete_timing_info.h"

void warn_unconstrained(std::shared_ptr<const tatum::TimingAnalyzer> analyzer) {
    if (analyzer->num_unconstrained_startpoints() > 0) {
        VTR_LOG_WARN("%zu timing startpoints were not constrained during timing analysis\n",
                     analyzer->num_unconstrained_startpoints());
    }
    if (analyzer->num_unconstrained_endpoints() > 0) {
        VTR_LOG_WARN("%zu timing endpoints were not constrained during timing analysis\n",
                     analyzer->num_unconstrained_endpoints());
    }
}
