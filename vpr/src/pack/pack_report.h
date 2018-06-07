#ifndef VPR_PACK_REPORT_H
#define VPR_PACK_REPORT_H

#include <iosfwd>
#include "vpr_context.h"

void report_packing_pin_usage(std::ostream& os, const VprContext& ctx);

#endif
