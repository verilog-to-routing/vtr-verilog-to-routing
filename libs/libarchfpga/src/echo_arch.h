#ifndef ECHO_ARCH_H
#define ECHO_ARCH_H

#include "arch_types.h"

void EchoArch(const char* EchoFile,
              const std::vector<t_physical_tile_type>& PhysicalTileTypes,
              const std::vector<t_logical_block_type>& LogicalBlockTypes,
              const t_arch* arch);

#endif
