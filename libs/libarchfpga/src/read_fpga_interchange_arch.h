#pragma once

#include <vector>
#include "physical_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* special type indexes, necessary for initialization, everything afterwards
 * should use the pointers to these type indices */

#define NUM_MODELS_IN_LIBRARY 4
#define EMPTY_TYPE_INDEX 0

/* function declaration */
void FPGAInterchangeReadArch(const char* FPGAInterchangeDeviceFile,
                             const bool timing_enabled,
                             t_arch* arch,
                             std::vector<t_physical_tile_type>& PhysicalTileTypes,
                             std::vector<t_logical_block_type>& LogicalBlockTypes);

#ifdef __cplusplus
}
#endif
