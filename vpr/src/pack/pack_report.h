#pragma once

#include <iosfwd>
#include "vpr_context.h"

void report_packing_pin_usage(std::ostream& os, const VprContext& ctx);
