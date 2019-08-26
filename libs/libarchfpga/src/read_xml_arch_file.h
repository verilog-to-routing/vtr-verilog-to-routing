#ifndef READ_XML_ARCH_FILE_H
#define READ_XML_ARCH_FILE_H

#include "arch_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* special type indexes, necessary for initialization, everything afterwards
 * should use the pointers to these type indices*/

#define NUM_MODELS_IN_LIBRARY 4
#define EMPTY_TYPE_INDEX 0

/* function declarations */
void XmlReadArch(const char* ArchFile,
                 const bool timing_enabled,
                 t_arch* arch,
                 std::vector<t_physical_tile_type>& PhysicalTileTypes,
                 std::vector<t_logical_block_type>& LogicalBlockTypes);

const char* get_arch_file_name();

#ifdef __cplusplus
}
#endif

#endif
