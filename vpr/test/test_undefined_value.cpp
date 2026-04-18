#include "vpr_types.h"
#include "arch_types.h"
#include "librrgraph_types.h"

// Make sure the 'unknown value' constants in VPR, LibArchFPGA and LibRRGraph are all equal to the same value (-1)
// Ideally these values should be independent of each other, but since they might be compared with each other in
// the project we enforce that they all have the same value.
static_assert(UNDEFINED == -1 && ARCH_FPGA_UNDEFINED_VAL == -1 && LIBRRGRAPH_UNDEFINED_VAL == -1, "The 'undefined value' constants in VPR, LibArchFPGA and LibRRGraph all must equal -1.");
