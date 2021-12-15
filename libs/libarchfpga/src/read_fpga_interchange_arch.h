#ifndef READ_FPGAINTERCHANGE_ARCH_FILE_H
#define READ_FPGAINTERCHANGE_ARCH_FILE_H

#include "arch_types.h"

#include "DeviceResources.capnp.h"
#include "LogicalNetlist.capnp.h"
#include "capnp/serialize.h"
#include "capnp/serialize-packed.h"
#include <fcntl.h>
#include <unistd.h>

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

#endif
