#pragma once

#include <vector>
#include "physical_types.h"

void EchoArch(const char* EchoFile,
              const std::vector<t_physical_tile_type>& PhysicalTileTypes,
              const std::vector<t_logical_block_type>& LogicalBlockTypes,
              const t_arch* arch);
